#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
import header_gen


@dataclass
class ArrayUse:
	name: str = ""						# intin etc.
	vdipb: str = ""						# name of local array or direct pointer (or dummy pointer if unused)
	values: int = 0
	pointers: int = 0
	chars: int = 0
	ret: int = 0						# a return that uses this array
	needLocalArray: bool = False		# We need a local array on the stack.
	dynamicLocalArray: bool = False		# The local array size is known only at runtime.
	directPointer: bool = False			# One argument only and it is a pointer we directly can use as work.
	usedIndexes: list[int] = field(default_factory=list)
	xmlArgs: list[ET.Element] = field(default_factory=list)
	usesStrlen: ET.Element = None		# An argument that uses strlen
	ff: ET.Element = None				# Function element
	arraySize: str = ""					# array size in words needed
	ctrlCount: str = ""					# Size to put in contrl array

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

def GetArraySize(arrUse: ArrayUse):
	for res in arrUse.ff.findall("reserve"):
		arr = res.attrib.get("src")
		if not arr:
			arr = res.attrib.get("dst")
		if arr and arr == arrUse.name:
			cnt = res.attrib.get("words")
			if cnt:
				arrUse.ctrlCount = str(cnt)
				arrUse.arraySize = str(cnt)
				return
			else:
				cnt = res.attrib.get("longs")
				if cnt:
					arrUse.ctrlCount = str(cnt)
					arrUse.arraySize = str(int(cnt) * 2)
					return

	topIdx = 0
	lastSize = ""
	mul = 1
	if "out" in arrUse.name:
		topIdx = arrUse.ret
	for a in arrUse.xmlArgs:
		idx = a.attrib.get("idx")
		seqIdx = a.attrib.get("seqIdx")
		if seqIdx and "src" in arrUse.name:
			idx = seqIdx
		if int(idx) >= int(topIdx):
			topIdx = idx
			est = a.attrib.get("estimate")
			words = a.attrib.get("words")
			longs = a.attrib.get("longs")
			if est:
				lastSize = est
				mul = 1
			elif words:		
				lastSize = words
				mul = 1
			else:
				if isinstance(longs, int) or longs.isnumeric():
					if int(longs) == 0:
						longs = 1
				lastSize = longs
				mul = 2

	if isinstance(lastSize, int) or lastSize.isnumeric():
		topIdx = int(topIdx) + (int(lastSize) * int(mul))
		lastSize = ""
	topIdxW = topIdx	
	if "pts" in arrUse.name:
		topIdx = (int(topIdx) + 1) >> 1
	if lastSize != "":
		if mul != 1:
			lastSize = "(" + lastSize + " << 1)"
		if int(topIdx) != 0:
			arrUse.ctrlCount = str(topIdx) + " + " + str(lastSize)
			arrUse.arraySize = str(topIdxW) + " + " + str(lastSize)
		else:
			arrUse.ctrlCount = str(lastSize)
			arrUse.arraySize = str(lastSize)
	else:
		arrUse.ctrlCount = str(topIdx)
		arrUse.arraySize = str(topIdxW)
	
def CombineIntWithStr(i, s):
	if s != "":
		if i != 0:
			return str(i) + s
		else:
			return s[3:]	# skip " + "
	else:
		return str(i)

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
		retlongs = r.attrib.get("longs")
		retsrc = r.attrib.get("src")
		retidx = r.attrib.get("idx")
		if retsrc == arrUse.name:
			if retidx:
				arrUse.ret += int(retidx)
			if retlongs:
				arrUse.ret += 2
			else:
				arrUse.ret += 1

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
		arg.set("idx", str(idx))

	next_idx = None
	if isinstance(count, int) or count.isnumeric():
		count = int(count) * mult
		next_idx = idx + count
		if not haveSize:
			arg.set("words", str(count))
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
				if words is not None:
					if words == "strlen":
						a.set("words", "_str_len")
						arrUse.usesStrlen = a
					elif words == "wstrlen":
						a.set("words", "_wstr_len")
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
	SetRetUsage(ff, arrUse)
	PreprocessSizeAndIdx(ff, arrUse)
	GetArraySize(arrUse)

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
		if isinstance(arrUse.arraySize, int) or arrUse.arraySize.isnumeric():
			# We known the size so a normal local array will do  
			arrUse.needLocalArray = True
		else:
			# We need to dynamically allocate memory for work data
			# We can runtime calculate the size of the array.
			# VLA is the solution.
			arrUse.dynamicLocalArray = True
			arrUse.needLocalArray = True

def PreprocessOutArray(ff, chkarr, arrUse: ArrayUse, dicts):
	name = ff.attrib.get("name")
	arrUse.name = chkarr
	arrUse.ff = ff
	SetRetUsage(ff, arrUse)
	PreprocessSizeAndIdx(ff, arrUse)
	SetTypeUsage(ff, arrUse, dicts)
	GetArraySize(arrUse)

	if arrUse.ret == 0 and arrUse.pointers == 1 and arrUse.chars == 0:
		# Simple case where we can use the ptr directly.
		arrUse.directPointer = True
	else:
		# Multiple pointers or combination of values and pointers
		# is the size static or dynamic?
		# do we need char conversion?
		if isinstance(arrUse.arraySize, int) or arrUse.arraySize.isnumeric():
			# We known the size so a normal local array will do  
			arrUse.needLocalArray = True
		else:
			# We need to dynamically allocate memory for work data
			# VLA is the solution.
			arrUse.dynamicLocalArray = True
			arrUse.needLocalArray = True

def	PreprocessFunction(ff, dicts):
	funcUse = FuncUse()
	PreprocessInArray(ff, "intin", funcUse.intin, dicts)
	PreprocessInArray(ff, "ptsin", funcUse.ptsin, dicts)
	PreprocessSequences(ff)
	PreprocessOutArray(ff, "intout", funcUse.intout, dicts)
	PreprocessOutArray(ff, "ptsout", funcUse.ptsout, dicts)
	return funcUse

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
	f.write("\tVDI_CAST_FROM_BYTES(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strWords + ");\n")

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
			f.write("\t" + arrUse.vdipb + "[" + str(idx) + "] = ")
			# Just a single value.
			if value:
				f.write(str(value))
			else:
				[_, _, isPtr, _, _] = header_gen.GetTypeName(type, dicts)
				if isPtr:
					f.write("*")
				f.write(name)
			f.write(";\n")
			return
	strWords = str(words).replace("contrl", "lcl_contrl")
	# Runtime known length or multiple values.
	f.write("\tVDI_COPY_WORDS(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strWords + ");\n")

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
			if "*" in type:
				f.write("\tVDI_COPY_LONG(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "]);\n")
			else:
				f.write("\tVDI_COPY_LONG(&" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "]);\n")
			return
	# Runtime known length or multiple values.
	strLongs = str(longs).replace("contrl", "lcl_contrl")
	f.write("\tVDI_COPY_LONGS(" + name + ", &" + arrUse.vdipb + "[" + str(idx) + "], " + strLongs + ")\n")

def WriteWorkInArgSetup(f, arrUse : ArrayUse, dicts):
	arrUse.xmlArgs.sort(key=SortOnIndex)
	for a in arrUse.xmlArgs:
		words = a.attrib.get("words")
		longs = a.attrib.get("longs")
		if words is not None:
			WriteInWords(f, a, arrUse, words, dicts)
		elif longs is not None:
			WriteInLongs(f, a, arrUse, longs, dicts)

def WriteOutString(f, a, arrUse : ArrayUse, words, dicts):
	name = a.attrib.get("name")
	idx = a.attrib.get("idx")
	if isinstance(words, int) or words.isnumeric():
		if int(words) == 1:
			# Simple typecast
			f.write("\t*" + name + " = " + arrUse.vdipb + "[" + str(idx) + "];\n")
			return
	strWords = str(words).replace("contrl", "lcl_contrl")
	f.write("\tVDI_CAST_TO_BYTES(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strWords + ");\n")

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
				f.write("\t" + name + "[" + str(seqIdx) + "] = " + arrUse.vdipb + "[" + str(idx) + "];\n")
			else:
				f.write("\t*" + name + " = " + arrUse.vdipb + "[" + str(idx) + "];\n")
			return
	# Runtime known length or multiple values.
	strWords = str(words).replace("contrl", "lcl_contrl")
	f.write("\tVDI_COPY_WORDS(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strWords + ");\n")

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
	f.write("\tVDI_COPY_LONGS(&" + arrUse.vdipb + "[" + str(idx) + "], " + name + ", " + strLongs + ");\n")

def WriteWorkOutArgSetup(f, arrUse : ArrayUse, dicts):
	arrUse.xmlArgs.sort(key=SortOnIndex)
	for a in arrUse.xmlArgs:
		words = a.attrib.get("words")
		longs = a.attrib.get("longs")
		nc = a.attrib.get("nullchk")
		if nc:
			name = a.attrib.get("name")
			f.write("\tif(" + name + " != 0)\n\t{\n\t")
		if words is not None:
			WriteOutWords(f, a, arrUse, words, dicts)
		elif longs is not None:
			WriteOutLongs(f, a, arrUse, longs, dicts)
		if nc:
			f.write("\t}\n")

def WriteWorkInSetup(f, ff, arrUse : ArrayUse, dicts):
	# Need to set in arrUse, what pointer vdipb should use.
	if arrUse.values == 0 and arrUse.pointers == 0:
		# Not used, use dummy pointer.
		arrUse.vdipb = "unused_dummy_array"
		#arrUse.contrl = "0"
		return
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
#	count = GetArraySizeString(arrUse)
	count = arrUse.arraySize
	if arrUse.needLocalArray:
		if not (isinstance(count, int) or count.isnumeric()):
			# create a variable to hold the size
			if count != "_str_len":	# Avoid creating a local var if we have one that works already.
				lcl_var_name = "lcl_" + arrUse.name + "_num"
				f.write("\t" + wordType + " " + lcl_var_name + " = " + count + ";\n")
				count = lcl_var_name	# use local variable as count from now on.
		#arrUse.ctrlCount = count
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
		#arrUse.ctrlCount = count
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
		#arrUse.contrl = "0"
		return

	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
#	count = GetArraySizeString(arrUse)
	count = arrUse.arraySize
	if arrUse.needLocalArray:
		#arrUse.contrl = count
		# Create array
		arrUse.vdipb = "lcl_" + arrUse.name
		if arrUse.name == "intout" and str(count) == "1":
			# Just to be safe if the call uses int instead of short.
			count = "2"
		f.write("\t" + wordType + " " + arrUse.vdipb + "[" + count + "];\n")
#		WriteWorkOutArgSetup(f, arrUse, dicts)
	elif arrUse.directPointer:
		argname = arrUse.xmlArgs[0].attrib.get("name")
		idx = arrUse.xmlArgs[0].attrib.get("idx")
		if int(idx) == 0:
			arrUse.vdipb = "(" + wordType + "*)" + argname
		else:
			arrUse.vdipb = "&((" + wordType + "*)" + argname + ")[" + str(idx) + "]"
		#arrUse.contrl = count
	else:
		print ("Error: Nope, don't understand.")

def MakeContrlArg(value, idx):
	newElement = ET.Element("arg")
	newElement.set("value", str(value))
	newElement.set("idx", str(idx))
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
	intinLen = funcUse.intin.ctrlCount
	contrlIn.append(MakeContrlArg(intinLen, 3))
	ptsinLen = funcUse.ptsin.ctrlCount
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

def WriteStrLen(f, a, dicts):
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	name = a.attrib.get("name")
	words = a.attrib.get("words")
	if words == "_str_len":
		f.write("\t" + wordType + " _str_len = vdi_strlen(" + name + ");\n")
	else:
		f.write("\t" + wordType + " _wstr_len = vdi_wstrlen(" + name + ");\n")

def WriteWorkInStrLen(f, funcUse : FuncUse, dicts):
	# If any of the arrays use strlen, then we want to know that length before setting up local arrays.
	[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
	if funcUse.intin.usesStrlen is not None:
		WriteStrLen(f, funcUse.intin.usesStrlen, dicts)
	elif funcUse.ptsin.usesStrlen is not None:
		WriteStrLen(f, funcUse.ptsin.usesStrlen, dicts)

def WriteWorkReturnSetup(f, ff):
	r = ff.find("return")
	if r is not None:
		type = r.attrib.get("type")
		longs = r.attrib.get("longs")
		if type and type != "void":
			src = r.attrib.get("src")
			idx = r.attrib.get("idx")
			code = r.attrib.get("code")
			if code or src != "intout":
				return
			f.write("\tlcl_intout[" + str(idx) + "] = 0;\n")
			if longs:
				idx = int(idx) + 1
				f.write("\tlcl_intout[" + str(idx) + "] = 0;\n")

def WriteWorkSetup(f, ff, funcUse : FuncUse, dicts):
	WriteWorkInStrLen(f, funcUse, dicts)
	WriteWorkInSetup(f, ff, funcUse.intin, dicts)
	WriteWorkInSetup(f, ff, funcUse.ptsin, dicts)
	WriteWorkOutSetup(f, funcUse.intout, dicts)
	WriteWorkOutSetup(f, funcUse.ptsout, dicts)
	WriteWorkReturnSetup(f, ff)
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
		longs = r.attrib.get("longs")
		if type and type != "void":
			src = r.attrib.get("src")
			idx = r.attrib.get("idx")
			code = r.attrib.get("code")
			if code:
				code = code.replace("contrl", "lcl_contrl")
				f.write("\treturn " + code + ";\n")
			elif longs:
				[_, castType, castPtr, _, _] = header_gen.GetTypeName(type, dicts)
				f.write("#pragma GCC diagnostic push\n")
				f.write("#pragma GCC diagnostic ignored \"-Wstrict-aliasing\"\n")
				f.write("\treturn *(" + castType + castPtr + "*)(&lcl_" + src + "[" + str(idx) +"]);\n")
				f.write("#pragma GCC diagnostic pop\n")
			else:
				f.write("\treturn lcl_" + src + "[" + str(idx) +"];\n")

def WriteFunction(f, ff, funcUse : FuncUse, dicts):
	name = ff.attrib.get("name")
	retType = "void"
	r = ff.find("return")
	if r is not None:
		retType = r.attrib.get("type")

	testing = dicts["settingsDict"]["testing"]

	if testing == "True":
		# Filter out functions that are not practical for current tests.
		if str(funcUse.intout.arraySize) == "0" and str(funcUse.ptsout.arraySize) == "0":
			testing = "False"

	if testing == "True":
		AppendDebug(ff, funcUse)
		WriteTestingFunction(f, ff, retType, dicts, funcUse)

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

	# Write begin function
	WriteWorkSetup(f, ff, funcUse, dicts)
	# Write vdi call
	f.write("\tvdi_call(&lcl_vdipb);\n\n")

	# Write result = if return
	WriteWorkExit(f, ff, funcUse, dicts)

	if testing == "True":
		WriteTestingCheck(f, name, funcUse)
	# Write return if return
	WriteReturn(f, ff, dicts)
	f.write("}\n")


def AppendDebugArray(dbg, arrUse : ArrayUse):
	arr = ET.Element(arrUse.name)
	dbg.append(arr)
	arr.set("arraySize", str(arrUse.arraySize))
	arr.set("ctrlCount", str(arrUse.ctrlCount))

def AppendDebug(ff, funcUse : FuncUse):
	dbg = ff.find("debug")
	if dbg is not None:
		AppendDebugArray(dbg, funcUse.intin)
		AppendDebugArray(dbg, funcUse.ptsin)
		AppendDebugArray(dbg, funcUse.intout)
		AppendDebugArray(dbg, funcUse.ptsout)

def GetTypeUse(t: str, dicts):
	callbackDict = dicts["callbackDict"]
	structDict = dicts["structDict"]
	typeDict = dicts["typeDict"]
	typedefDict= dicts["typedefDict"]
	isPtr = ""
	typename = ""
	isCallback = False
	isStruct = False

	if t[0] == 'c':
		t = t[1:]
	if t[-1] == ']':
		arr = t.index('[')
		isPtr = "*"
		t = t[:arr]
	while t[-1] == '*':
		isPtr = isPtr + "*"
		t = t[:-1]

	if t in callbackDict:
		typename = t
		isCallback = True
	elif t in structDict:
		typename = structDict[t].attrib.get("name")
		isStruct = True
	elif t in typedefDict:
		typename = t
	elif t in typeDict:
		typename = typeDict[t]
	else:
		typename = t

	return [typename, isPtr, isStruct, isCallback]

def WriteInitTestVariable(f, a, n, dicts, funcUse : FuncUse):
	t = a.attrib.get("type")
	[typename, isPtr, isStruct, isCallback] = GetTypeUse(t, dicts)
	if n == "handle":
		if isPtr:
			a.set("test_ampersand", "1")
		return
	if not isPtr:
		f.write("\t" + typename + " " + n)
		if isCallback:
			f.write(" = test_" + typename +"_callback;\n")
		else:
			f.write(" = 0;\n")
	else:
		f.write("\t" + typename + " " + n)
		if isPtr == "**":
			f.write("_arr[MAX_TEST_ARRAY] = {0};\n")
			f.write("\t" + typename + "* " + n + " = " + n + "_arr;\n")
			a.set("test_ampersand", "1")
			return
		if isCallback:
			f.write(" = test_" + typename +"_callback;\n")
			a.set("test_ampersand", "1")
		elif isStruct:
			a.set("test_ampersand", "1")
			f.write(" = test_" + typename +"_struct;\n")
		else:
			count = a.attrib.get("words")
			if count:
				if count == "1":
					a.set("test_ampersand", "1")
					f.write(" = 0;\n")
				else:
					f.write("[MAX_TEST_ARRAY] = {0};\n")
			else:
				count = a.attrib.get("longs")
				if count == "1":
					if t == "int16_t" or t == "uint16_t":
						f.write("[2] = {0};\n")
					else:
						a.set("test_ampersand", "1")
						f.write(" = 0;\n")
				else:
					f.write("[MAX_TEST_ARRAY] = {0};\n")

def WriteTestingFunction(f, ff, retType, dicts, funcUse : FuncUse):
	# Excludes may be tested in the future.
	excludes = ["v_opnwk", "v_getoutline", "v_getbitmap_info", "vqt_fontheader", "v_flushcache", "vqt_get_table", "v_loadcache", "v_savecache", "v_get_outline"]
	name = ff.attrib.get("name")
	if name not in excludes:
		dicts["testCalls"][name] = "INT16_T test_" + name + "(FILE* fp)"

	f.write('#include \"test.h\"\n\n')
	f.write("INT16_T test_" + name + "_intout;\n")
	f.write("INT16_T test_" + name + "_ptsout;\n\n")
	f.write("INT16_T test_" + name + "(FILE* fp)\n{\n")
	f.write("\ttest_" + name + "_intout = 0;\n")
	f.write("\ttest_" + name + "_ptsout = 0;\n")
	f.write("\tINT16_T test_status = 0;\n")

	for a in ff.findall('arg'):
		n = a.attrib.get("name")
		if n:
			WriteInitTestVariable(f, a, n, dicts, funcUse)

	f.write("\n")
	f.write("\tfprintf(fp, \"Trying: " + name + "\\n\");\n")
	f.write("\tfflush(fp);\n")
#	if retType != "void":
#		header_gen.WriteType(f, "result", retType, dicts)
#		f.write(" = ")
	f.write("\t" + name + "(")
	first = True
	for a in ff.findall('arg'):
		n = a.attrib.get("name")
		if n:
			ampersand = a.attrib.get("test_ampersand")
			if not first:
				f.write(", ")
			first = False
			if n == "handle":
				n = "glbl_handle"
			if ampersand:
				f.write("&" + n)
			else:
				f.write(n)
	f.write(");\n\n")
	f.write("\tfprintf(fp, \"Done.\\n\");\n")

	f.write("\tif (test_" + name + "_intout != 0)\n")
	f.write("\t{\n\t\tfprintf(fp, \"" + name + ": intout = %d\\n\", test_" + name + "_intout);\n")
	f.write("\t\ttest_status = 1;\n\t}\n")
	f.write("\tif (test_" + name + "_ptsout != 0)\n")
	f.write("\t{\n\t\tfprintf(fp, \"" + name + ": ptsout = %d\\n\", test_" + name + "_ptsout);\n")
	f.write("\t\ttest_status = 1;\n\t}\n")
	f.write("\tfflush(fp);\n")
	f.write("\treturn test_status;\n")
	f.write("}\n\n")

def WriteTestingCheck(f, name, funcUse : FuncUse):
	intoutcnt = str(funcUse.intout.ctrlCount)
	intoutcnt = intoutcnt.replace("contrl", "lcl_contrl")
	f.write("\tif (lcl_contrl[4] > " + intoutcnt + ")\n")
	f.write("\t{\n\t\ttest_" + name + "_intout = lcl_contrl[4];\n\t}\n")
	ptsoutcnt = str(funcUse.ptsout.ctrlCount)
	ptsoutcnt = ptsoutcnt.replace("contrl", "lcl_contrl")
	f.write("\tif (lcl_contrl[2] > " + ptsoutcnt + ")\n")
	f.write("\t{\n\t\ttest_" + name + "_ptsout = lcl_contrl[2];\n\t}\n")

def CodeVDIFunction(iname, build_dir, ff, dicts):
	subid = ff.attrib.get("subid")	# VDI sub function
	if not subid:
		ff.set("subid", "0")
	grpid = ff.attrib.get("grpid")	# Trap num (always 2 for vdi)
	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError

	# Create a debug element we can add information to.
	# This can be useful if we later save out the xml for analyzing.
	dbgElement = ET.Element("debug")
	ff.append(dbgElement)

	name = ff.attrib.get("name")
	with open(build_dir + name + ".c", "w") as f:
		f.write('#include "vdi_def.h"\n\n')
		funcUse = PreprocessFunction(ff, dicts)	# Insert default and automatic attributes.
		WriteFunction(f, ff, funcUse, dicts)


