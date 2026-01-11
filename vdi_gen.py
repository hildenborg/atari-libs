#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
import header_gen



@dataclass
class ArrayUse:
	name: str = ""						# intin etc.
	vdipb: str = ""						# name of local array or direct pointer (or dummy pointer if unused)
	contrl: str = "0"					# contrl array count as a c string
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
	xmlArgs: list[ET.Element] = field(default_factory=list)
	usesStrlen: ET.Element = None		# An argument that uses strlen
	ff: ET.Element = None				# Function element

@dataclass
class FuncUse:
	intin: ArrayUse = field(default_factory=ArrayUse)
	intout: ArrayUse = field(default_factory=ArrayUse)
	ptsin: ArrayUse = field(default_factory=ArrayUse)
	ptsout: ArrayUse = field(default_factory=ArrayUse)

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

def GetStaticArraySize(arrUse: ArrayUse):
	lclArraySize = arrUse.ret
	for a in arrUse.xmlArgs:
		words = GetCountInWords(a)
		if words is None:
			return None	# array size is not compile time known.
		lclArraySize += words
	return int(lclArraySize)

def IsNotRuntimeKnown(count):
	keywords = ["strlen", "_str_len", "contrl[4]", "contrl[2]"]
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

	if "out" in arr and IsNotRuntimeKnown(count):
		count = arg.attrib.get("estimate")
		if count:
			return count
		return None
	return count + mult

def GetDynamicArraySize(arrUse: ArrayUse):
	lclArraySize = arrUse.ret
	lclArrayCode = []
	for a in arrUse.xmlArgs:
		words = GetCountInWordsOrCode(a, arrUse.name)
		if words is None:
			return None	# array size is not runtime known. Have to guess...
		if isinstance(words, int) or words.isnumeric():
			lclArraySize += int(words)
		else:
			lclArrayCode.append(words)
	lclArrayCode.insert(0, int(lclArraySize))
	return lclArrayCode

def SetTypeUsage(ff, arrUse: ArrayUse, dicts):
	v = 0
	p = 0
	ch = 0
	ret = 0
	for a in arrUse.xmlArgs:
		t = a.attrib.get("type")
		[tv, tp, tch] = CheckArgType(a, t, dicts)
		v += tv
		p += tp
		ch += tch

	arrUse.values = v
	arrUse.pointers = p
	arrUse.chars = ch

def SetRetUsage(ff, arrUse: ArrayUse):
	arrUse.ret = 0
	r = ff.find("return")
	if r is not None:
		retsrc = r.attrib.get("src")
		if retsrc == arrUse.name:
			arrUse.ret = 1

def SetDefaultSizeAndIdx(arg, idx, arrUse: ArrayUse):
	arg_idx = arg.attrib.get("idx")
	if idx is None:
		if not arg_idx:
			print ("Error: Trying to use automatic indexing on multiple dynamic arrays.")
		idx = arg_idx # Restart from defined
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
		if arrUse is not None:
			for i in range(count):
				arrUse.usedIndexes.append(idx + i)

	return next_idx

def IsSrcOrDst(arg, arr):
	src = arg.attrib.get("src")
	if src and src == arr:
		return True
	dst = arg.attrib.get("dst")
	if dst and dst == arr:
		return True
	return False

# Fill in idx and size where missing and applicable
# And as the name do not say: build an element array and fixup some sequence stuff etc.
def PreprocessSizeAndIdx(ff, arrUse: ArrayUse):
	idx = arrUse.ret
	for a in ff.findall('arg'):
		type = a.attrib.get("type")
		name = a.attrib.get("name")
		if a.find('sequence') is None:
			if IsSrcOrDst(a, arrUse.name):
				arrUse.xmlArgs.append(a)
				idx = SetDefaultSizeAndIdx(a, idx, arrUse)
				words = a.attrib.get("words")
				if words is not None and words == "strlen":
					a.set("words", "_str_len")
					arrUse.usesStrlen = a
		else:
			# Only for intout and ptsout
			seqIdx = 0
			for s in a.findall('sequence'):
				if IsSrcOrDst(s, arrUse.name):
					s.set("seqIdx", str(seqIdx))
					arrUse.xmlArgs.append(s)
					seqIdx = seqIdx + 1

def PreprocessSequences(ff):
	idx = 0	# None of the functions that have sequences uses return
	for a in ff.findall('arg'):
		type = a.attrib.get("type")
		name = a.attrib.get("name")
		# Only for intout and ptsout
		for s in a.findall('sequence'):
			src = s.attrib.get("src")
			if src and "out" in src:
				s.set("type", type)
				s.set("name", name)
				idx = SetDefaultSizeAndIdx(s, idx, None)

# Sets deafult values and determines the type of code we need to build for this array.
def PreprocessInArray(ff, chkarr, arrUse: ArrayUse, dicts):
	arrUse.name = chkarr
	arrUse.ff = ff
#	fncname = ff.attrib.get("name")
#	if fncname == "v_bit_image":
#		pass
	SetRetUsage(ff, arrUse)
	PreprocessSizeAndIdx(ff, arrUse)
	res = ff.find("reserve")
	if res is not None:
		dst = res.attrib.get("dst")
		if dst and dst == arrUse.name:
			cnt = res.attrib.get("words")
			if cnt:
				arrUse.staticSize = int(cnt)
			else:
				cnt = res.attrib.get("longs")
				if cnt:
					arrUse.staticSize = int(cnt) << 1
	if arrUse.staticSize is None:
		arrUse.staticSize = GetStaticArraySize(arrUse)
		arrUse.dynamicSize = GetDynamicArraySize(arrUse)

	SetTypeUsage(ff, arrUse, dicts)

	if arrUse.values >= 0 and arrUse.pointers == 0 and arrUse.chars == 0:
		# Simple case with just const values set into array list.
		# We need a local array and we know the size.
		arrUse.needLocalArray = True
	elif arrUse.values == 0 and arrUse.pointers == 1 and arrUse.chars == 0:
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
	arrUse.name = chkarr
	arrUse.ff = ff
	SetRetUsage(ff, arrUse)
	PreprocessSizeAndIdx(ff, arrUse)
	SetTypeUsage(ff, arrUse, dicts)
	arrUse.staticSize = GetStaticArraySize(arrUse)
	arrUse.dynamicSize = GetDynamicArraySize(arrUse)

	if arrUse.values != 0:
		print ("Error: " + name + " - No values in output, must be pointers")
	elif arrUse.ret == 0 and arrUse.pointers == 1 and arrUse.chars == 0:
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
			print(name + " needs estimate")

def	PreprocessFunction(ff, dicts):
	funcUse = FuncUse()
	PreprocessInArray(ff, "intin", funcUse.intin, dicts)
	PreprocessInArray(ff, "ptsin", funcUse.ptsin, dicts)
	PreprocessSequences(ff)
	PreprocessOutArray(ff, "intout", funcUse.intout, dicts)
	PreprocessOutArray(ff, "ptsout", funcUse.ptsout, dicts)
	return funcUse

def GetArraySizeString(arrUse : ArrayUse):
	count = ""
	num = 0
	first = True
	if arrUse.dynamicSize is not None:
		for c in arrUse.dynamicSize:
			sc =str(c)
			if sc !="0":
				sc = sc.replace("contrl", "lcl_contrl")
				if not first:
					count += " + "
				first = False
				count += "(" + str(sc) + ")"
				num = num + 1
		if num == 1:
			count = count[1:-1]	# Skip parantheses if not needed
	else:
		count = str(arrUse.staticSize)
	return count

def SortOnIndex(n):
	idx = n.attrib.get("idx")
	if idx is None:
		return 100000	# Large number to be put last.
	return int(idx)

def WriteInString(f, a, arrUse : ArrayUse, words, dicts):
	type = a.attrib.get("type")
	name = a.attrib.get("name")
	idx = a.attrib.get("idx")
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	if isinstance(words, int) or words.isnumeric():
		if int(words) == 1:
			# Simple typecast
			f.write(arrUse.vdipb + "[" + str(idx) + "]) = (" + wordType + ")")
			[_, _, isPtr, _, _] = header_gen.GetTypeName(type, dicts)
			if isPtr:
				f.write("*")
			f.write(name)
			return
	strWords = str(words).replace("contrl", "lcl_contrl")
	f.write("VDI_CAST_FROM_BYTES(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strWords + ")")

def WriteInWords(f, a, arrUse : ArrayUse, words, dicts):
	type = a.attrib.get("type")
	if "int8_t" in type:
		WriteInString(f, a, arrUse, words, dicts)
		return
	if isinstance(words, int) or words.isnumeric():
		if (int(words) & 1) == 0:
			# Even number of words, lets do longs instead
			WriteInLongs(f, a, arrUse, int(words) >> 1, dicts)
			return
	name = a.attrib.get("name")
	idx = a.attrib.get("idx")
	value = a.attrib.get("value")
	if isinstance(words, int) or words.isnumeric():
		# Compile time known length
		words = int(words)
		if words == 1:
			f.write(arrUse.vdipb + "[" + str(idx) + "] = ")
			# Just a single value.
			if value:
				f.write(str(value))
			else:
				[_, _, isPtr, _, _] = header_gen.GetTypeName(type, dicts)
				if isPtr:
					f.write("*")
				f.write(name)
			return
	strWords = str(words).replace("contrl", "lcl_contrl")
	# Runtime known length or multiple values.
	f.write("VDI_COPY_WORDS(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strWords + ")")

def WriteInLongs(f, a, arrUse : ArrayUse, longs, dicts):
	name = a.attrib.get("name")
	type = a.attrib.get("type")
	idx = a.attrib.get("idx")
	[_, longType, _, _, _] = header_gen.GetTypeName("int32_t", dicts)
	if isinstance(longs, int) or longs.isnumeric():
		# Compile time known length
		longs = int(longs)
		if longs == 0:
			# Pointer copy
			f.write("\tVDI_COPY_LONG(&" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "]);\n")
			return
		elif longs == 1:
			f.write("\tVDI_COPY_LONG(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "]);\n")
			return
	# Runtime known length or multiple values.
	strLongs = str(longs).replace("contrl", "lcl_contrl")
	f.write("VDI_COPY_LONGS(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strLongs + ")")

def WriteWorkInArgSetup(f, arrUse : ArrayUse, dicts):
	arrUse.xmlArgs.sort(key=SortOnIndex)
	for a in arrUse.xmlArgs:
		words = a.attrib.get("words")
		longs = a.attrib.get("longs")
		f.write("\t")
		if words is not None:
			WriteInWords(f, a, arrUse, words, dicts)
		elif longs is not None:
			WriteInLongs(f, a, arrUse, longs, dicts)
		f.write(";\n")

def WriteOutString(f, a, arrUse : ArrayUse, words, dicts):
	name = a.attrib.get("name")
	idx = a.attrib.get("idx")
	if isinstance(words, int) or words.isnumeric():
		if int(words) == 1:
			# Simple typecast
			f.write("*" + name + " = " + arrUse.vdipb + "[" + str(idx) + "]")
			return
	strWords = str(words).replace("contrl", "lcl_contrl")
	f.write("VDI_CAST_TO_BYTES(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strWords + ")")

def WriteOutWords(f, a, arrUse : ArrayUse, words, dicts):
	type = a.attrib.get("type")
	if "int8_t" in type:
		WriteOutString(f, a, arrUse, words, dicts)
		return
	if isinstance(words, int) or words.isnumeric():
		if (int(words) & 1) == 0:
			# Even number of words, lets do longs instead
			WriteOutLongs(f, a, arrUse, int(words) >> 1, dicts)
			return
	name = a.attrib.get("name")
	idx = a.attrib.get("idx")
	seqIdx = a.attrib.get("seqIdx")
	if isinstance(words, int) or words.isnumeric():
		# Compile time known length
		words = int(words)
		if words == 1:
			if seqIdx:
				f.write(name + "[" + str(seqIdx) + "] = " + arrUse.vdipb + "[" + str(idx) + "]")
			else:
				f.write("*" + name + " = " + arrUse.vdipb + "[" + str(idx) + "]")
			return
	# Runtime known length or multiple values.
	strWords = str(words).replace("contrl", "lcl_contrl")
	f.write("VDI_COPY_WORDS(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strWords + ")")

def WriteOutLongs(f, a, arrUse : ArrayUse, longs, dicts):
	name = a.attrib.get("name")
	type = a.attrib.get("type")
	idx = a.attrib.get("idx")
	[_, longType, _, _, _] = header_gen.GetTypeName("int32_t", dicts)
	if isinstance(longs, int) or longs.isnumeric():
		# Compile time known length
		longs = int(longs)
		if longs == 1:
			f.write("\tVDI_COPY_LONG(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ");\n")
			return
	# Runtime known length or multiple values.
	strLongs = str(longs).replace("contrl", "lcl_contrl")
	f.write("VDI_COPY_LONGS(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strLongs + ")")

def WriteWorkOutArgSetup(f, arrUse : ArrayUse, dicts):
	arrUse.xmlArgs.sort(key=SortOnIndex)
	for a in arrUse.xmlArgs:
		words = a.attrib.get("words")
		longs = a.attrib.get("longs")
		f.write("\t")
		if words is not None:
			WriteOutWords(f, a, arrUse, words, dicts)
		elif longs is not None:
			WriteOutLongs(f, a, arrUse, longs, dicts)
		f.write(";\n")

def WriteWorkInSetup(f, ff, arrUse : ArrayUse, dicts):
	# Need to set in arrUse, what pointer vdipb should use.
	if arrUse.values == 0 and arrUse.pointers == 0:
		# Not used, use dummy pointer.
		arrUse.vdipb = "unused_dummy_array"
		arrUse.contrl = "0"
		return

	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	count = GetArraySizeString(arrUse)
	if arrUse.needLocalArray:
		if not (isinstance(count, int) or count.isnumeric()):
			# create a variable to hold the size
			if count != "_str_len":	# Avoid creating a local var if we have one that works already.
				lcl_var_name = "lcl_" + arrUse.name + "_num"
				f.write("\t" + wordType + " " + lcl_var_name + " = " + count + ";\n")
				count = lcl_var_name	# use local variable as count from now on.
		arrUse.contrl = count
		# Create array
		arrUse.vdipb = "lcl_" + arrUse.name
		f.write("\t" + wordType + " " + arrUse.vdipb + "[" + count + "];\n")
		WriteWorkInArgSetup(f, arrUse, dicts)
	elif arrUse.directPointer:
		argname = arrUse.xmlArgs[0].attrib.get("name")
		idx = arrUse.xmlArgs[0].attrib.get("idx")
		if int(idx) == 0:
			arrUse.vdipb = "(" + wordType + "*)" + argname
		else:
			arrUse.vdipb = "&((" + wordType + "*)" + argname + ")[" + str(idx) + "]"
		arrUse.contrl = count
	else:
		print ("Error: Nope, don't understand.")
	# Check if we need to zero some data.
	res = ff.find('reserve')
	if res is not None and arrUse.needLocalArray and not arrUse.dynamicLocalArray:
		dst = res.attrib.get("dst")
		if dst and dst == arrUse.name:
			count = res.attrib.get("words")
			if not count:
				count = res.attrib.get("longs")
				count = int(count) << 1
			for i in range(int(count)):
				if i not in arrUse.usedIndexes:
					f.write("\t" + arrUse.vdipb + "[" + str(i) +"] = 0;\n")

def WriteWorkOutSetup(f, arrUse : ArrayUse, dicts):
	if arrUse.values == 0 and arrUse.pointers == 0 and arrUse.ret == 0:
		# Not used, use dummy pointer.
		arrUse.vdipb = "unused_dummy_array"
		arrUse.contrl = "0"
		return

	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	count = GetArraySizeString(arrUse)
	if arrUse.needLocalArray:
		arrUse.contrl = count
		# Create array
		arrUse.vdipb = "lcl_" + arrUse.name
		f.write("\t" + wordType + " " + arrUse.vdipb + "[" + count + "];\n")
#		WriteWorkOutArgSetup(f, arrUse, dicts)
	elif arrUse.directPointer:
		argname = arrUse.xmlArgs[0].attrib.get("name")
		idx = arrUse.xmlArgs[0].attrib.get("idx")
		if int(idx) == 0:
			arrUse.vdipb = "(" + wordType + "*)" + argname
		else:
			arrUse.vdipb = "&((" + wordType + "*)" + argname + ")[" + str(idx) + "]"
		arrUse.contrl = count
	else:
		print ("Error: Nope, don't understand.")

def MakeContrlArg(value, idx):
	newElement = ET.Element("arg")
	newElement.set("value", value)
	newElement.set("idx", idx)
	newElement.set("type", "int16_t")
	return newElement

def WriteWorkContrlSetup(f, ff, funcUse : FuncUse, dicts):
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	f.write("\t" + wordType + " lcl_contrl[16];\n")
	contrlIn = []
	for a in ff.findall('arg'):
		dst = a.attrib.get("dst")
		if dst and dst == "contrl":
			contrlIn.append(a)
	id = ff.attrib.get("id")
	contrlIn.append(MakeContrlArg(id, 0))
	subid = ff.attrib.get("subid")
	if subid:
		contrlIn.append(MakeContrlArg(subid, 5))
	intinLen = funcUse.intin.contrl
	contrlIn.append(MakeContrlArg(intinLen, 3))
	ptsinLen = funcUse.ptsin.contrl
	contrlIn.append(MakeContrlArg(ptsinLen, 1))

	contrlIn.sort(key=SortOnIndex)
	for a in contrlIn:
		type = a.attrib.get("type")
		idx = a.attrib.get("idx")
		name = a.attrib.get("name")
		[_, _, isPtr, _, _] = header_gen.GetTypeName(type, dicts)
		if name:
			longs = a.attrib.get("longs")
			if longs:
				if int(longs) == 0:
					f.write("\tVDI_COPY_LONG(&" + name + ", &lcl_contrl[" + str(idx) + "]);\n")
				elif int(longs) == 1:
					if isPtr:
						f.write("\tVDI_COPY_LONG(" + name + ", &lcl_contrl[" + str(idx) + "]);\n")
					else:
						f.write("\tVDI_COPY_LONG(&" + name + ", &lcl_contrl[" + str(idx) + "]);\n")
				else:
					print ("error, multiple int32_t")
			else:
				f.write("\tlcl_contrl[" + str(idx) + "] = ")
				if isPtr:
					f.write("*")
				f.write(name + ";\n")
		else:
			value = a.attrib.get("value")
			f.write("\tlcl_contrl[" + str(idx) + "] = " + str(value) + ";\n")

def WriteWorkContrlExit(f, ff, dicts):
	contrlOut = []
	for a in ff.findall('arg'):
		src = a.attrib.get("src")
		if src and src == "contrl":
			contrlOut.append(a)

	contrlOut.sort(key=SortOnIndex)
	for a in contrlOut:
		type = a.attrib.get("type")
		idx = a.attrib.get("idx")
		name = a.attrib.get("name")
		[_, _, isPtr, _, _] = header_gen.GetTypeName(type, dicts)
		if not isPtr:
			print ("error: output needs pointer.")
			return
		longs = a.attrib.get("longs")
		if longs:
			if int(longs) == 1:
				f.write("\tVDI_COPY_LONG(&lcl_contrl[" + str(idx) + "], " + name + ");\n")
			else:
				print ("error, multiple int32_t")
		else:
			f.write("\t*" + name + " = lcl_contrl[" + str(idx) + "];\n")

def WriteWorkInStrLen(f, funcUse : FuncUse, dicts):
	# If any of the arrays use strlen, then we want to know that length before setting up local arrays.
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	if funcUse.intin.usesStrlen is not None:
		name = funcUse.intin.usesStrlen.attrib.get("name")
		f.write("\t" + wordType + " _str_len = vdi_strlen(" + name + ");\n")
	elif funcUse.ptsin.usesStrlen is not None:
		name = funcUse.ptsin.usesStrlen.attrib.get("name")
		f.write("\t" + wordType + " _str_len = vdi_strlen(" + name + ");\n")

def WriteWorkSetup(f, ff, funcUse : FuncUse, dicts):
	WriteWorkInStrLen(f, funcUse, dicts)
	WriteWorkInSetup(f, ff, funcUse.intin, dicts)
	WriteWorkInSetup(f, ff, funcUse.ptsin, dicts)
	WriteWorkOutSetup(f, funcUse.intout, dicts)
	WriteWorkOutSetup(f, funcUse.ptsout, dicts)
	WriteWorkContrlSetup(f, ff, funcUse, dicts)
	# Write vdipb
	f.write("\tVDIPB lcl_vdipb =\n\t{\n")
	f.write("\t\tlcl_contrl,\n")
	f.write("\t\t" + funcUse.intin.vdipb + ",\n")
	f.write("\t\t" + funcUse.ptsin.vdipb + ",\n")
	f.write("\t\t" + funcUse.intout.vdipb + ",\n")
	f.write("\t\t" + funcUse.ptsout.vdipb + "\n")
	f.write("\t};\n")

def WriteWorkOutExit(f, arrUse : ArrayUse, dicts):
	if arrUse.needLocalArray:
		WriteWorkOutArgSetup(f, arrUse, dicts)

def WriteWorkExit(f, ff, funcUse : FuncUse, dicts):
	WriteWorkOutExit(f, funcUse.intout, dicts)
	WriteWorkOutExit(f, funcUse.ptsout, dicts)
	WriteWorkContrlExit(f, ff, dicts)

def WriteReturn(f, ff, dicts):
	r = ff.find("return")
	if r is not None:
		type = r.attrib.get("type")
		if type and type != "void":
			src = r.attrib.get("src")
			idx = r.attrib.get("idx")
			f.write("\treturn lcl_" + src + "[" + str(idx) +"];\n")

def WriteFunction(f, ff, funcUse : FuncUse, dicts):
	# Write begin function
	WriteWorkSetup(f, ff, funcUse, dicts)
	# Write vdi call
	f.write("\tvdi_call(&lcl_vdipb);\n\n")

	# Write result = if return
	WriteWorkExit(f, ff, funcUse, dicts)
	# Write return if return
	WriteReturn(f, ff, dicts)

def CodeVDIFunction(iname, build_dir, ff, dicts):
	name = ff.attrib.get("name")
	subid = ff.attrib.get("subid")	# VDI sub function
	if not subid:
		ff.set("subid", "0")
	grpid = ff.attrib.get("grpid")	# Trap num (always 2 for vdi)
	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError

	retType = "void"
	r = ff.find("return")
	if r is not None:
		retType = r.attrib.get("type")

	with open(build_dir + name + ".c", "w") as f:
		f.write('#include "vdi_def.h"\n\n')

		header_gen.WriteType(f, "", retType, dicts)
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

		funcUse = PreprocessFunction(ff, dicts)	# Insert default and automatic attributes.
		WriteFunction(f, ff, funcUse, dicts)
		f.write("}\n")


