#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

from dataclasses import dataclass, field
import header_gen



@dataclass
class ArrayUse:
	staticSize: int = None
	dynamicSize: list = None
	values: int = 0
	pointers: int = 0
	chars: int = 0
	ret: int = 0						# a return that uses this array
	needLocalArray: bool = False		# We need a local array on the stack.
	dynamicLocalArray: bool = False		# The local array size is known only at runtime.
	estimatedLocalArray: bool = False	# The local array size is known after it has been used. We need to estimate a size that is enough.
	directPointer: bool = False			# One argument only and it is a pointer we directly can use as work.
	usedIndexes: list[int] = field(default_factory=list)

@dataclass
class FuncUse:
	intin: ArrayUse = field(default_factory=ArrayUse)
	intout: ArrayUse = field(default_factory=ArrayUse)
	ptsin: ArrayUse = field(default_factory=ArrayUse)
	ptsout: ArrayUse = field(default_factory=ArrayUse)

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
#	code = code.replace("contrl", "vdiparblk.contrl")
#	code = code.replace("intin", "vdiparblk.intin")
#	code = code.replace("ptsin", "vdiparblk.ptsin")
#	code = code.replace("intout", "vdiparblk.intout")
#	code = code.replace("ptsout", "vdiparblk.ptsout")
	return code

def AddStringToArray(tmpVals, src, str):
	if src:
		arr = "out"
	else:
		arr = "in"
	tmpVals["s_" + arr] = tmpVals["s_" + arr] + str

def AddStringToFastArray(tmpVals, src, dst, addToFast, str):
	if addToFast:
		tmpVals["s_fast_start"] = tmpVals["s_fast_start"] + str
		if src:
			tmpVals["s_fast_end"] = tmpVals["s_fast_end"] + " " + src
		else:
			tmpVals["s_fast_end"] = tmpVals["s_fast_end"] + " " + dst
	else:
		if src:
			tmpVals["s_fast_out"] = tmpVals["s_fast_out"] + str
		else:
			tmpVals["s_fast_start"] = tmpVals["s_fast_start"] + str

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
				count = "contrl[2]"	# ptsout
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
				count = "contrl[4]"	# intout
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
	vdipbString = MakeVDIPBString(src, dst, idx)

	addToFast = False
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
			if typeSize == 2:
				seqString = ptrString[1:] + " = " + argString + ";"
			else:
				seqString = "*" + ptrString + " = " + argString + ";"
			fastSeqString = seqString
		else:
			seqString = MakeCopy(argString, ptrString, src, isLongs, typeSize, count)
			[fastSeqString, addToFast] = MakeFastCopy(argString, ptrString, vdipbString, src, isLongs, typeSize, count, dicts, tmpVals)
	
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
			fastSeqString = seqString
		else:
			# Copy an inlined code number of data
			# Lets create a local variable to that and use that.
			varName = "_" + name + "_len"
			if idx > 0:
				varName += "_" + str(idx)
			lclVar = MakeLocalVar(varName, count, dicts)
			AddStringToArray(tmpVals, src, tabs + lclVar + "\n")
			# In fast mode, the string stored here may not be used,
			#  and will cause a compiler warning if added to source.
			AddStringToFastArray(tmpVals, src, dst, False, tabs + lclVar + "\n")
			# Lets use the local variable instead of count
			if dst is not None:
				tmpVals["rl_" + dst] = tmpVals["rl_" + dst] + " + " + varName
			seqString = MakeCopy(argString, ptrString, src, isLongs, typeSize, varName)
			[fastSeqString, addToFast] = MakeFastCopy(argString, ptrString, vdipbString, src, isLongs, typeSize, varName, dicts, tmpVals)
		seqIdx = -1

	AddStringToArray(tmpVals, src, tabs + seqString + "\n")
	AddStringToFastArray(tmpVals, src, dst, addToFast, tabs + fastSeqString + "\n")

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

def MakeFastCopy(argString, ptrString, vdipbString, isSrc, isLongs, typeSize, count, dicts, tmpVals):
	if not isinstance(count, int) and count.isnumeric():
		count = int(count)

	if not isLongs and typeSize == 2 and count == 1:
		if isSrc:
			return [MakeNiceWordCopy(argString, ptrString, isSrc), False]
		else:
			return [MakeNiceWordCopy(argString, ptrString, isSrc), False]

	if typeSize == 1 or (isLongs and count == 1):
		if isLongs:
			seqString = "VDI_COPY_LONG"
		elif isSrc:
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
		return [seqString, False]
	else:
		if isSrc and not isinstance(count, int):
			# The previously created local variable count is unused.
			# Remove the last line we added.
			# It's almost a hack, but it works.
			tmpVals["s_fast_out"] = tmpVals["s_fast_out"][:-1]	# remove last linefeed
			lastLineIdx = tmpVals["s_fast_out"].rfind("\n")
			tmpVals["s_fast_out"] = tmpVals["s_fast_out"][:lastLineIdx + 1]
		[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
		seqString = "lcl_vdipb." + vdipbString + " = (" + wordType + "*)" + argString + ";"
	return [seqString, True]


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
		tstr = "&" + src + "[" + str(idx) + "]"
	else:
		tstr = "&" + dst + "[" + str(idx) + "]"
	return tstr

def MakeVDIPBString(src, dst, idx):
	if idx < 0:
		idx = 0
	if src:
		tstr = src
	else:
		tstr = dst
	return tstr

def CheckArgType(a, t, dicts):
	[_, _, isPtr, _, _] = header_gen.GetTypeName(t, dicts)
	longs = a.attrib.get("longs")
	if longs and longs == "0":
		# Special case when we don't use the pointer as a pointer.
		isPtr = False

	p = 0
	v = 0
	ch = 0
	if isPtr:
		# Arg is a pointer, count it.
		p = 1
	else:
		# Arg is a const value, count it.
		v = 1
	if "int8_t" in t:
		# Arg is a char, count it.
		ch = 1
	return [v, p, ch]

def GetCountInWords(arg):
	mult = 1
	count = arg.attrib.get("words")
	if not count:
		count = arg.attrib.get("longs")
		if count:
			mult = 2
			if count == "0":
				count = 1	# size of a pointer * 2
		else:
			count = 1	# default count if none given.
	if isinstance(count, int) or count.isnumeric():
		return int(count * mult)
	return None

def GetStaticArraySize(ff, chkarr, dir):
	lclArraySize = 0
	for a in ff.findall('arg'):
		if a.find('sequence') is None:
			arr = a.attrib.get(dir)
			if arr and arr == chkarr:
				words = GetCountInWords(a)
				if words is None:
					return None	# array size is not compile time known.
				lclArraySize += words
		for s in a.findall('sequence'):
			arr = s.attrib.get(dir)
			if arr and arr == chkarr:
				words = GetCountInWords(s)
				if words is None:
					return None	# array size is not compile time known.
				lclArraySize += words
	return int(lclArraySize)

def IsNotRuntimeKnown(count):
	keywords = ["strlen", "_str_len", "all", "contrl[4]", "contrl[2]"]
	for word in keywords:
		if count == word:
			return True
	return False

def GetCountInWordsOrCode(arg, arr):
	count = GetCountInWords(arg)
	if count is not None:
		return count

	mult = ""
	count = arg.attrib.get("words")
	if not count:
		mult = " * 2"
		count = arg.attrib.get("longs")

	if arr == "src" and IsNotRuntimeKnown(count):
		return None
	return count + mult

def GetDynamicArraySize(ff, chkarr, dir):
	lclArraySize = 0
	lclArrayCode = []
	for a in ff.findall('arg'):
		if a.find('sequence') is None:
			arr = a.attrib.get(dir)
			if arr and arr == chkarr:
				words = GetCountInWordsOrCode(a, chkarr)
				if words is None:
					return None	# array size is not runtime known. Have to guess...
				if isinstance(words, int) or words.isnumeric():
					lclArraySize += words
				else:
					lclArrayCode.append(words)
		for s in a.findall('sequence'):
			arr = s.attrib.get(dir)
			if arr and arr == chkarr:
				words = GetCountInWordsOrCode(a, chkarr)
				if words is None:
					return None	# array size is not runtime known. Have to guess...
				if isinstance(words, int) or words.isnumeric():
					lclArraySize += words
				else:
					lclArrayCode.append(words)
	lclArrayCode.insert(0, int(lclArraySize))
	return lclArrayCode

def GetTypeUsage(ff, arr, dir, dicts):
	v = 0
	p = 0
	ch = 0
	ret = 0
	for a in ff.findall('arg'):
		t = a.attrib.get("type")
		if a.find('sequence') is None:
			srcdst = a.attrib.get(dir)
			if srcdst and srcdst == arr:
				[tv, tp, tch] = CheckArgType(a, t, dicts)
				v += tv
				p += tp
				ch += tch
		for s in a.findall('sequence'):
			srcdst = s.attrib.get(dir)
			if srcdst and srcdst == arr:
				[tv, tp, tch] = CheckArgType(s, t, dicts)
				v += tv
				p += tp
				ch += tch
	
	r = ff.find("return")
	if r is not None:
		retsrc = r.attrib.get("src")
		if retsrc == arr:
			ret = 1		# Only set ret if it uses the array.

	return [v, p, ch, ret]

def SetDefaultSizeAndIdx(arg, idx, arrUse: ArrayUse):
	arg_idx = arg.attrib.get("idx")
	if idx is None:
		if not arg_idx:
			print ("Error: Trying to use automatic indexing on multiple dynamoc arrays.")
		return None
	haveSize = True
	mult = 1
	count = arg.attrib.get("words")
	if not count:
		count = arg.attrib.get("longs")
		if count:
			mult = 2
			if count == "0":
				count = 1	# size of a pointer * 2
		else:
			haveSize = False
			count = 1	# default count if none given.

	if arg_idx:
		if int(arg_idx) != idx:
			idx = int(arg_idx)
	else:
		arg.set("idx", idx)

	next_idx = None
	if isinstance(count, int) or count.isnumeric():
		count = int(count) * mult
		next_idx = idx + count
		if not haveSize:
			arg.set("words", count)
		for i in range(count):
			arrUse.usedIndexes.append(idx + i)

	return next_idx

def IsSrcOrdst(arg, arr):
	src = arg.attrib.get("src")
	if src and src == arr:
		return True
	dst = arg.attrib.get("dst")
	if dst and dst == arr:
		return True
	return False

# Fill in idx and size where missing and applicable
def PreprocessSizeAndIdx(ff, arr, arrUse: ArrayUse):
	idx = 0
	for a in ff.findall('arg'):
		if a.find('sequence') is None:
			if IsSrcOrdst(a, arr):
				idx = SetDefaultSizeAndIdx(a, idx, arrUse)
		for s in a.findall('sequence'):
			if IsSrcOrdst(s, arr):
				idx = SetDefaultSizeAndIdx(s, idx, arrUse)

# Sets deafult values and determines the type of code we need to build for this array.
def PreprocessInArray(ff, chkarr, arrUse: ArrayUse, dicts):
	PreprocessSizeAndIdx(ff, chkarr, arrUse)
	arrUse.staticSize = GetStaticArraySize(ff, chkarr, "dst")
	arrUse.dynamicSize = GetDynamicArraySize(ff, chkarr, "dst")
	[vals, ptrs, chars, ret] = GetTypeUsage(ff, chkarr, "dst", dicts)
	arrUse.values = vals
	arrUse.pointers = ptrs
	arrUse.chars = chars
	arrUse.ret = ret
	if vals >= 0 and ptrs == 0 and chars == 0:
		# Simple case with just const values set into array list.
		# We need a local array and we know the size.
		arrUse.needLocalArray = True
	elif vals == 0 and ptrs == 1 and chars == 0:
		# Simple case where we can use the ptr directly.
		arrUse.directPointer = True
	else:
		# Multiple pointers or combination of values and pointers
		# is the size static or dynamic?
		# do we need char conversion?
		if arrUse.staticSize is not None:
			# We known the size so a normal local array will do  
			arrUse.needLocalArray = True
		elif arrUse.dynamicSize is not None:
			# We need to dynamically allocate memory for work data
			# We can runtime calculate the size of the array.
			# VLA is the solution.
			arrUse.dynamicLocalArray = True
			arrUse.needLocalArray = True
		else:
			print("Error: dst arrays must be known either compile time or runtime before the vdi call.")

def PreprocessOutArray(ff, chkarr, arrUse: ArrayUse, dicts):
	name = ff.attrib.get("name")
	PreprocessSizeAndIdx(ff, chkarr, arrUse)
	arrUse.staticSize = GetStaticArraySize(ff, chkarr, "src")
	arrUse.dynamicSize = GetDynamicArraySize(ff, chkarr, "src")
	[vals, ptrs, chars, ret] = GetTypeUsage(ff, chkarr, "src", dicts)
	arrUse.values = vals
	arrUse.pointers = ptrs
	arrUse.chars = chars
	arrUse.ret = ret
	if vals != 0 and ptrs == 0 and chars == 0:
		print ("Error: " + name + " - No values in output, must be pointers")
	elif vals == 0 and ptrs == 1 and chars == 0:
		# Simple case where we can use the ptr directly.
		arrUse.directPointer = True
	else:
		# Multiple pointers or combination of values and pointers
		# is the size static or dynamic?
		# do we need char conversion?
		if arrUse.staticSize is not None:
			# We known the size so a normal local array will do  
			arrUse.needLocalArray = True
		elif arrUse.dynamicSize is not None:
			# We need to dynamically allocate memory for work data
			# VLA is the solution.
			arrUse.dynamicLocalArray = True
			arrUse.needLocalArray = True
		else:
			# Nothing is known about the size of the array until after the vdi call.
			# We need to make an estimate...
			arrUse.estimatedLocalArray = True
			arrUse.needLocalArray = True

def	PreprocessFunction(ff, dicts):
	funcUse = FuncUse()
	PreprocessInArray(ff, "intin", funcUse.intin, dicts)
	PreprocessInArray(ff, "ptsin", funcUse.ptsin, dicts)
	PreprocessOutArray(ff, "intout", funcUse.intout, dicts)
	PreprocessOutArray(ff, "ptsout", funcUse.ptsout, dicts)
	return funcUse

	# no longs or words = words="1"
	# words and longs can be c code
	# words can be strlen
	# longs can be 0 when dst and ptr, for storing the pointer.
	# reserve
	# no src or dst means arg is only used locally.
	# sort idx access


def SetupTmpVals():
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
		"s_fast_start": "",
		"s_fast_end": "",
		"s_fast_out": "",
		"r_intin": 0,
		"r_ptsin": 0,
		"m_intin": "",
		"m_ptsin": "",
		"rl_intin": "",
		"rl_ptsin": "",
	}
	return tmpVals

def ReserveMemory(ff, name, tmpVals):
	# Some functions use only some positions in an array but expects a specific size of array.
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

def CodeVDIFunction(iname, build_dir, ff, dicts):
	name = ff.attrib.get("name")
	id = ff.attrib.get("id")		# VDI function
	subid = ff.attrib.get("subid")	# VDI sub function
	if not subid:
		subid = 0
	grpid = ff.attrib.get("grpid")	# Trap num (always 2 for vdi)
	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError

	flagTreadSafe = header_gen.GetSetting(dicts, "flagTreadSafe")
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)

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

	with open(build_dir + name + ".c", "w") as f:
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
		f.write("\t" + wordType + " contrl[16];\n")

		tmpVals = SetupTmpVals()

		# If return is an intout, then update the automatic index.
		if retIsSrcIdx:
			if retsrc == "intout" and retidx == "0":
				ts = GetTypeSize(ret, dicts)
				tmpVals["c_intout"] = (ts + 1) >> 1	# even words

		ReserveMemory(ff, name, tmpVals)

		PreprocessFunction(ff, dicts)	# Insert default and automatic attributes.
		# for debug
#		[v, p, ch] = tmpVals["chk_intin"]
#		f.write("// intin v: " + str(v) + ", p: " + str(p) + ", ch: " + str(ch) +"\n")
#		[v, p, ch] = tmpVals["chk_intout"]
#		f.write("// intout v: " + str(v) + ", p: " + str(p) + ", ch: " + str(ch) +"\n")
#		[v, p, ch] = tmpVals["chk_ptsin"]
#		f.write("// ptsin v: " + str(v) + ", p: " + str(p) + ", ch: " + str(ch) +"\n")
#		[v, p, ch] = tmpVals["chk_ptsout"]
#		f.write("// ptsout v: " + str(v) + ", p: " + str(p) + ", ch: " + str(ch) +"\n")
		# end for debug
		#void* mupp = alloca(tabs); when we dynamically knows the length.
		# char -> short, reverse convert in same buffer.
		# short -> char, needs estimate for max buffer.

		tabs = '\t'
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			t = a.attrib.get("type")
			seqIdx = 0
			if a.find('sequence') is None:
				seqIdx = HandleVDIArg(name, n, t, a, None, tmpVals, tabs, dicts, seqIdx)
			prevSeq = a
			for s in a.findall('sequence'):
				seqIdx = HandleVDIArg(name, n, t, s, prevSeq, tmpVals, tabs, dicts, seqIdx)
				prevSeq = s

		ptsins = int(tmpVals["r_ptsin"] / 2)
		sptsins = str(ptsins) + tmpVals["rl_ptsin"]
		sptsins = sptsins.replace("0 + ", "")
		if sptsins != "0":
			f.write("\t" + wordType + " ptsin[" + sptsins +"];\n")

		intins = int(tmpVals["r_intin"])
		sintins = str(intins) + tmpVals["rl_intin"]
		sintins = sintins.replace("0 + ", "")
		if sintins != "0":
			f.write("\t" + wordType + " intin[" + sintins +"];\n")

		gotIntout = "c_intout" in tmpVals and tmpVals["c_intout"] > 0
		if gotIntout:
			f.write("\t" + wordType + " intout[" + str(tmpVals["c_intout"]) +"];\n")

		f.write("\tVDIPB lcl_vdipb = \n")
		f.write("\t{\n")
		f.write("\t\tcontrl,\n")
		if sintins != "0":
			f.write("\t\tintin,\n")
		else:
			f.write("\t\tvdiparblk.intin,\t// Unused.\n")
		if sptsins != "0":
			f.write("\t\tptsin,\n")
		else:
			f.write("\t\tvdiparblk.ptsin,\t// Unused.\n")
		if gotIntout:
			f.write("\t\tintout,\n")
		else:
			f.write("\t\tvdiparblk.intout,\t// Unused.\n")
		f.write("\t\tvdiparblk.ptsout\t// Unused.\n")
		f.write("\t};\n")

		HandleUntouched(f, tmpVals, "intin")
		HandleUntouched(f, tmpVals, "ptsin")

		if (tmpVals["s_fast_start"] != ""):
			f.write(tmpVals["s_fast_start"])

		f.write(tabs + "contrl[0] = " + str(id) + ";\n")

		f.write(tabs + "contrl[1] = " + sptsins + ";\n")

		f.write(tabs + "contrl[3] = " + sintins + ";\n")

		f.write(tabs + "contrl[5] = " + str(subid) + ";\n")
		
		f.write("\tvdi_call(&lcl_vdipb);\n\n")

		if (tmpVals["s_fast_out"] != ""):
			f.write(tmpVals["s_fast_out"])

		if retIsSrcIdx:
			f.write("\t")
			header_gen.WriteType(f, "result", ret, dicts)
			if ret == "int16_t" or ret == "uint16_t":
				f.write(" = " + retsrc + "[" + retidx + "];\n")
			elif ret == "int8_t" or ret == "uint8_t":
				f.write(" = (" + ret + ")" + retsrc + "[" + retidx + "];\n")
			else:
				# Assume 4 byte size
				f.write(";\n\t")
				f.write("VDI_COPY_LONG(&(" + retsrc + "[" + retidx + "]), &result);\n")

		if ret != "void":
			if retIsCode:
				f.write("\treturn ")
				rcode = MakeVDIInlineCode(retcode)
				f.write(rcode)
				f.write(";\n")
			else:
				f.write("\treturn result;\n")
		f.write("}\n\n")

def HandleUntouched(f, tmpVals, arr):
	indices = [i for i, x in enumerate(tmpVals["m_" + arr]) if x == "_"]
	
	for idx in indices:
		f.write("\t" + arr + "[" + str(idx) + "] = 0;\n")

