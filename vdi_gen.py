#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import header_gen

# size in bytes for argument types
def GetTypeSize(t, dicts):
	callbackDict = dicts["callbackDict"]
	typedefDict = dicts["typedefDict"]
	structDict = dicts["structDict"]

	t = t.replace("*", "")
	if t[0] == 'c':
		t = t[1:]

	if t in callbackDict:
		return 4
	elif t in structDict:
		# Always pointers
		return 4
	elif t in typedefDict:
		td = typedefDict[t]
		t = td.attrib.get("type")
		return GetTypeSize(t, dicts)

	if t == "int8_t" or t == "uint8_t":
		return 1
	elif t == "int16_t" or t == "uint16_t":
		return 2
	elif t == "int32_t" or t == "uint32_t":
		return 4
	return 0	# void

def MakeVDIInlineCode(code):
	code = code.replace("contrl", "vdiparblk.contrl")
	code = code.replace("intin", "vdiparblk.intin")
	code = code.replace("ptsin", "vdiparblk.ptsin")
	code = code.replace("intout", "vdiparblk.intout")
	code = code.replace("ptsout", "vdiparblk.ptsout")
	return code

def AddStringToArray(tmpVals, src, str):
	if src:
		arr = "out"
	else:
		arr = "in"
	tmpVals["s_" + arr] = tmpVals["s_" + arr] + str

# size is in words
def UpdateVDIIndex(tmpVals, src, dst, idx, size):
	arr = src
	if not arr:
		arr = dst
	carr = "c_" + arr
	if not carr in tmpVals or tmpVals[carr] < 0:
		# Array is of undeterminable size, so some features is impossible:
		# Automatically updated idx.
		# Reserving memory and setting untouched to zero.
		# Sequences is impossible.
		return
	start = idx
	end = idx + size
	tmpVals[carr] = end
	if 'out' in arr:
		# Only need to reserve and mask memory for input
		return
	if 'pts' in arr:
		# always reserve even words for ptsin
		end = (end + 1) & ~1
	if tmpVals["r_" + arr] < end:
		# Add unused positions to mask
		ml = end - tmpVals["r_" + arr]
		tmpVals["m_" + arr] += "_" * ml
		tmpVals["r_" + arr] = end
	# mask used words
	useStr = "*" * size
	tmpVals["m_" + arr] = tmpVals["m_" + arr][: start] + useStr + tmpVals["m_" + arr][end:]

#	name = name or argument
#	type = the arguments type.
#	seq = arg or sequence tag.
#	prevSeq = None if seq = arg tag. Previous arg or sequence tag if seq = sequence tag.
def HandleVDIArg(fncName, name, type, seq, prevSeq, tmpVals, tabs, dicts, seqIdx):
	# src is where the data is taken from, and arg is destination: src->arg
	src = seq.attrib.get("src")
	# dst is where the data is going to, and arg is source: arg->dst
	dst = seq.attrib.get("dst")
	# words is number of words that should be copied. Can be missing (empty)
	words = seq.attrib.get("words")
	# longs is number of long words that should be copied. Can be missing (empty)
	longs = seq.attrib.get("longs")
	# isPtr is True if the arg is a pointer. If False, the arg is a constant or callback.
	isPtr = '*' in type
	# idx is the index to use with the src or dst.
	idx = seq.attrib.get("idx")
	# If name is missing, then value is a constant that should be used.
	value = seq.attrib.get("value")
	# Set to True if we can copy using longs.
	isLongs = False
	# Default data length to copy is one word.
	count = 1

	if idx:
		idx = int(idx)
	else:
		idx = -1

	if prevSeq is not None:
		psrc = prevSeq.attrib.get("src")
		pdst = prevSeq.attrib.get("dst")
		if psrc and not src:
			# If src was used in previous sequence, then it must be used in this.
			print ("Inconsequent src/dst usage for: " + fncName)
			raise ValueError
		if pdst and not dst:
			# If dst was used in previous sequence, then it must be used in this.
			print ("Inconsequent src/dst usage for: " + fncName)
			raise ValueError

	if name:
		if value:
			print ("Cannot use both name and value for: " + fncName)
			raise ValueError
	else:
		name = "const_value"
		if not value:
			print ("Missing name or value for:" + fncName)
			raise ValueError

	# Size in bytes for the type when we need to know. Size = 1 means typecasting to word
	typeSize = GetTypeSize(type, dicts);

	if seqIdx < 0:
		print ("Cannot calculate the sequence index for: " + fncName)
		raise ValueError

	# Verify data and calculate expected defaults
	if src and not dst:
		# src->arg
		# In this case, arg must always be a pointer
		if idx < 0:
			idx = tmpVals["c_" + src]
		if not isPtr:
			print ("Arg: " + name + "must be a pointer in: " + fncName)
			raise ValueError
		if longs:
			isLongs =True
			if isinstance(longs, int) or longs.isnumeric():
				if int(longs) == 0:
					# Special case.
					# Size is always two words, and it is the pointer itself that should be copied.
					count = 1
					isPtr = False	# Use it as a constant and not ptr
				else:
					# absolute numerical size
					count = longs
			elif longs == "all":
				# Copy all of the remaining data. Make it into an inline code.
				count = "vdiparblk.contrl[2]"	# ptsout
				if idx >= 0:
					if (idx & 1) != 0:
						# Cannot use longs for an uneven number of words...
						isLongs = False
						count = "(" + count + " * 2) - " + str(idx)
					else:
						count += " - " + str(int(idx / 2))
			else:
				# longs is a functional code to be inserted in program code.
				count = MakeVDIInlineCode(longs)
		elif words:
			isLongs =False
			if isinstance(words, int) or words.isnumeric():
				# absolute numerical size
				count = words
			elif words == "all":
				# Copy all of the remaining data. Make it into an inline code.
				count = "vdiparblk.contrl[4]"	# intout
				if idx >= 0:
					count += " - " + str(idx)
			else:
				# words is a functional code to be inserted in program code.
				count = MakeVDIInlineCode(words)
		else:
			# The assumption here is to copy one word
			isLongs =False
			count = 1
	elif dst and not src:
		# arg->dst
		# In this case, arg can be a constant, callback, pointer or non pointer.
		if idx < 0:
			idx = tmpVals["c_" + dst]
		if isPtr:
			if longs:
				isLongs =True
				if isinstance(longs, int) or longs.isnumeric():
					if int(longs) == 0:
						# Special case.
						# Size is always two words, and it is the pointer itself that should be copied.
						count = 1
						isPtr = False	# Use it as a constant and not ptr
					else:
						# absolute numerical size
						count = longs
				else:
					# longs is a functional code to be inserted in program code.
					count = MakeVDIInlineCode(longs)
			elif words:
				isLongs =False
				if isinstance(words, int) or words.isnumeric():
					# absolute numerical size
					count = words
				elif words == "strlen":
					count = -1	# Size is determined by zero ended string
				else:
					# words is a functional code to be inserted in program code.
					count = MakeVDIInlineCode(words)
			else:
				# The assumption here is to copy one word
				isLongs =False
				count = 1
		else:
			# longs or words if existing must be set to 1
			if longs:
				isLongs =True
				if (isinstance(longs, int) or longs.isnumeric()) and int(longs) == 1:
					count = 1
				else:
					print ("Longs can only be set to 1: " + fncName)
					raise ValueError
			elif words:
				isLongs =False
				if (isinstance(words, int) or words.isnumeric()) and int(words) == 1:
					count = 1
				else:
					print ("Words can only be set to 1: " + fncName)
					raise ValueError
			else:
				# Need to figure out size from the type.
				if typeSize > 2:
					isLongs =True
				else:
					isLongs =False
				count = 1
	elif not dst and not src:
		# Special case. This argument is not stored, but may be used by inlining code.
		# Basically we do nothing and just return
		return
	else:
		# must have just one of src or dst
		print ("Ill formatted arg in: " + fncName)
		raise ValueError
	
	if idx < 0:
		print ("Impossible to determine idx for: " + fncName)
		raise ValueError

	if isLongs and typeSize == 1 and int(longs) != 0:
		print ("Cannot cast from byte to long: " + fncName)
		raise ValueError

	if value:
		argString = value
		if count != 1 or isLongs or src:
			print ("Value must be one word in size and src cannot be used: " + fncName)
			raise ValueError
	else:
		argString = MakeArgString(name, type, isPtr, seqIdx, dicts)
	ptrString = MakePtrString(src, dst, idx)

	# Determine what to do with the data now when we have the defaults
	if (isinstance(count, int) or count.isnumeric()) and int(count) > 0:
		# Copy a compile time known number of data
		count = int(count)
		# This will auto update the idx and also calculate a new seqIdx
		if not isLongs and (count & 1) == 0:
			# even number of words, we can convert that to longs
			isLongs = True
			count = int(count / 2)

		if value:
			seqString = "*" + ptrString + " = " + argString + ";"
		else:
			seqString = MakeCopy(argString, ptrString, src, isLongs, typeSize, count)
	
		seqIdx = seqIdx + count
		idxupd = count
		if isLongs:
			seqIdx = seqIdx + count
			idxupd += count
			
		UpdateVDIIndex(tmpVals, src, dst, idx, idxupd)
	else:
		# Copy a compile time unknown number of data
		# This will turn off auto update of idx and invalidate any further use of seqIdx
		# We still need to add the run time length to the contrl array if we use ptsin or intin.
		if count == -1:
			if typeSize != 1:
				print ("type is expected to be of byte size: " + fncName)
				raise ValueError
			# Copy and cast zero ended string data
			seqString = MakeStrlenCopy(argString, ptrString, dicts)
			if dst is not None:
				tmpVals["rl_" + dst] = tmpVals["rl_" + dst] + " + _str_len"
		else:
			# Copy an inlined code number of data
			# Lets create a local variable to that and use that.
			varName = "_" + name + "_len"
			if idx > 0:
				varName += "_" + str(idx)
			lclVar = MakeLocalVar(varName, count, dicts)
			AddStringToArray(tmpVals, src, tabs + lclVar + "\n")
			# Lets use the local variable instead of count
			if dst is not None:
				tmpVals["rl_" + dst] = tmpVals["rl_" + dst] + " + " + varName
			seqString = MakeCopy(argString, ptrString, src, isLongs, typeSize, varName)
		seqIdx = -1

	AddStringToArray(tmpVals, src, tabs + seqString + "\n")

	return seqIdx

def MakeLocalVar(varName, count, dicts):
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	count = count.replace(" - 0", "")
	lclVar = wordType + " " + varName + " = " + count + ";"
	return lclVar

def MakeStrlenCopy(argString, ptrString, dicts):
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	seqString = wordType + " _str_len = vdi_zero_ended_string_to_words(" + argString + ", " + ptrString + ");"
	return seqString

def MakeCopy(argString, ptrString, isSrc, isLongs, typeSize, count):
	if not isinstance(count, int) and count.isnumeric():
		count = int(count)

	if not isLongs and typeSize == 2 and count == 1:
		return MakeNiceWordCopy(argString, ptrString, isSrc)

	if isLongs:
		seqString = "VDI_COPY_LONG"
	else:
		if typeSize != 1:
			seqString = "VDI_COPY_WORD"
		else:
			if isSrc:
				seqString = "VDI_CAST_TO_BYTE"
			else:
				seqString = "VDI_CAST_FROM_BYTE"
	if not isinstance(count, int) or count > 1:
		# Multiple values copy
		seqString += "S"

	if isSrc:
		# ptrString is source
		seqString += "(" + ptrString + ", " + argString
	else:
		# ptrString is destination
		seqString += "(" + argString + ", " + ptrString

	if not isinstance(count, int) or count > 1:
		# Multiple values copy
		seqString += ", " + str(count)

	seqString += ");"

	return seqString

def MakeNiceWordCopy(argString, ptrString, isSrc):
	# When only copying one word, we can do a cleaner syntax
	if argString[0] == '&':
		argString = argString[1:]
	else:
		argString = "*" + argString

	if ptrString[0] == '&':
		ptrString = ptrString[1:]

	if isSrc:
		# ptrString is source
		seqString = argString + " = " + ptrString + ";"
	else:
		# ptrString is destination
		seqString = ptrString + " = " + argString + ";"
	return seqString

# Should always return an address
def MakeArgString(name, type, isPtr, seqIdx, dicts):
	[_, typeName, _, _, _] = header_gen.GetTypeName(type, dicts)
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	if isPtr:
		# pointer, construct an address.
		if seqIdx > 0:
			# seqIdx is always word position.
			if wordType == typeName:
				tstr = "&" + name + "[" + str(seqIdx) + "]"
			else:
				# Need to cast
				tstr = "&((" + wordType + "*)" + name + ")[" + str(seqIdx) + "]"
		else:
			tstr = name
	else:
		# const value
		tstr = "&" + name
	return tstr

def MakePtrString(src, dst, idx):
	# The array pointers are always word arrays.
	if idx < 0:
		idx = 0
	if src:
		tstr = "&vdiparblk." + src + "[" + str(idx) + "]"
	else:
		tstr = "&vdiparblk." + dst + "[" + str(idx) + "]"
	return tstr

def CodeVDIFunction(iname, ff, dicts, options):
	name = ff.attrib.get("name")
	id = ff.attrib.get("id")		# VDI function
	subid = ff.attrib.get("subid")	# VDI sub function
	if not subid:
		subid = 0
	grpid = ff.attrib.get("grpid")	# Trap num (always 2 for vdi)
	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError
	
	ret = "void"
	retidx = ""
	retsrc = ""
	retcode = ""
	retIsCode = False
	retIsSrcIdx = False
	r = ff.find("return")
	if r is not None:
		ret = r.attrib.get("type")
		retidx = r.attrib.get("idx")
		retsrc = r.attrib.get("src")
		retcode = r.attrib.get("code")
	if ret == "void":
		if (retcode or retidx or retsrc):
			# void return do not need any attributes
			print ("Ill formatted return for: " + name + "\n")
			raise ValueError
	else:
		if retcode:
			if (retidx or retsrc):
				# retcode cannot exist at the same time as retsrc or retidx
				print ("Ill formatted return for: " + name + "\n")
				raise ValueError
			else:
				retIsCode = True
		else:
			if not (retidx and retsrc):
				# retsrc and retidx are complementary.
				print ("Ill formatted return for: " + name + "\n")
				raise ValueError
			else:
				retIsSrcIdx = True

	with open("gen/" + name + ".c", "w") as f:
		f.write('#include "' + iname + '.h"\n\n')

		header_gen.WriteType(f, "", ret, dicts)
		f.write(" " + name + "(")

		first = True
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			t = a.attrib.get("type")
			if n:
				if not first:
					f.write(", ")
				first = False
				header_gen.WriteType(f, n, t, dicts)
		if first:
			f.write("void")
		f.write(")\n{\n")

		# c_ = current index
		# s_ = generated code string
		# r_ = number of reserved words
		# m_ = _ or * used as word mask for unused/used. Unused should be set to 0. Only for input
		# s_contrl is not used and instead stored in s_intin and s_intout depending on src or dst.
		# rl_ = number of reserved words (intin) and longs (pstin) stated in local variables added together.
		tmpVals = {
			"c_intin": 0,
			"c_intout": 0,
			"c_ptsin": 0,
			"c_ptsout": 0,
			"s_in": "",
			"s_out": "",
			"r_intin": 0,
			"r_ptsin": 0,
			"m_intin": "",
			"m_ptsin": "",
			"rl_intin": "",
			"rl_ptsin": "",
		}
		if retIsSrcIdx:
			if retsrc == "intout" and retidx == "0":
				ts = GetTypeSize(ret, dicts)
				tmpVals["c_intout"] = (ts + 1) >> 1	# even words

		for a in ff.findall('reserve'):
			dst = a.attrib.get("dst")
			if dst is None:
				print ("Reserve can unly be dst: " + name + "\n")
				raise ValueError

			words = a.attrib.get("words")
			longs = a.attrib.get("longs")
			size = 0
			if words is not None:
				size = int(words)
			elif longs is not None:
				size = int(longs) * 2
			tmpVals["r_" + dst] = size
			tmpVals["m_" + dst] = "_" * size

		tabs = '\t'
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			t = a.attrib.get("type")
			tabs = HandleTestBegin(a, tmpVals, tabs)
			seqIdx = 0
			if a.find('sequence') is None:
				seqIdx = HandleVDIArg(name, n, t, a, None, tmpVals, tabs, dicts, seqIdx)
			prevSeq = a
			for s in a.findall('sequence'):
				seqIdx = HandleVDIArg(name, n, t, s, prevSeq, tmpVals, tabs, dicts, seqIdx)
				prevSeq = s
			tabs = HandleTestEnd(a, tmpVals, tabs)
		
		if "fast_vdi" in options:
			f.write("// FAST_VDI_STUFF!\n")
		else:
			HandleUntouched(tmpVals, "intin")
			HandleUntouched(tmpVals, "ptsin")
			f.write(tmpVals["s_in"])
		f.write(tabs + "vdiparblk.contrl[0] = " + str(id) + ";\n")

		ptsins = int(tmpVals["r_ptsin"] / 2)
		sptsins = str(ptsins) + tmpVals["rl_ptsin"]
		sptsins = sptsins.replace("0 + ", "")
		f.write(tabs + "vdiparblk.contrl[1] = " + sptsins + ";\n")

		intins = int(tmpVals["r_intin"])
		sintins = str(intins) + tmpVals["rl_intin"]
		sintins = sintins.replace("0 + ", "")
		f.write(tabs + "vdiparblk.contrl[3] = " + sintins + ";\n")

		f.write(tabs + "vdiparblk.contrl[5] = " + str(subid) + ";\n")
		f.write("\tvdi_call();\n")

		if "fast_vdi" not in options:
			f.write(tmpVals["s_out"])

		if retIsSrcIdx:
			f.write("\t")
			header_gen.WriteType(f, "result", ret, dicts)
			if ret == "int16_t" or ret == "uint16_t":
				f.write(" = vdiparblk." + retsrc + "[" + retidx + "];\n")
			elif ret == "int8_t" or ret == "uint8_t":
				f.write(" = (" + ret + ")vdiparblk." + retsrc + "[" + retidx + "];\n")
			else:
				# Assume 4 byte size
				f.write(";\n\t")
				f.write("VDI_COPY_LONG(&(vdiparblk." + retsrc + "[" + retidx + "]), &result);\n")

		if "fast_vdi" not in options:
			f.write("// FAST_VDI_STUFF!\n")

		if ret != "void":
			if retIsCode:
				f.write("\treturn ")
				rcode = MakeVDIInlineCode(retcode)
				f.write(rcode)
				f.write(";\n")
			else:
				f.write("\treturn result;\n")
		f.write("}\n\n")

def HandleUntouched(tmpVals, arr):
	indices = [i for i, x in enumerate(tmpVals["m_" + arr]) if x == "_"]
	
	tstr = ""
	for idx in indices:
		# Position is untouched, so we need to clear it.
		tstr += "\t" + "vdiparblk." + arr + "[" + str(idx) + "] = 0;\n"
	AddStringToArray(tmpVals, None, tstr)

def HandleTestBegin(a, tmpVals, tabs):
	tst = a.attrib.get("test")
	if tst:
		src = a.attrib.get("src")
		code = MakeVDIInlineCode(tst)
		tstr = tabs + "if (" + code + ")\n" + tabs + "{\n"
		AddStringToArray(tmpVals, src, tstr)
		tabs += '\t'
	return tabs

def HandleTestEnd(a, tmpVals, tabs):
	tst = a.attrib.get("test")
	if tst:
		src = a.attrib.get("src")
		tstr = tabs + "}\n"
		AddStringToArray(tmpVals, src, tstr)
		return tabs[1:]
	return tabs
