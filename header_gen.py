#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

def HeaderBegin(f, name):
	f.write("#ifndef " + name.upper() + "_DEFINED\n")
	f.write("#define " + name.upper() + "_DEFINED\n\n")
	f.write("#ifdef __cplusplus\n")
	f.write('extern "C" {\n')
	f.write("#endif\n\n")

def HeaderEnd(f, name):
	f.write("#ifdef __cplusplus\n")
	f.write("}\n")
	f.write("#endif\n\n")
	f.write("#endif // " + name.upper() + "_DEFINED\n")
	
def HeaderCategory(f, category):
	f.write("/*\n")
	f.write("\tCategory: " + category + "\n")
	f.write("*/\n\n")

def HeaderDefines(f, defines):
	for n, v in defines.items():
		f.write("#define " + n + " " + v + "\n")
	f.write("\n\n")

def GetTypeName(t: str, dicts):
	callbackDict = dicts["callbackDict"]
	structDict = dicts["structDict"]
	typeDict = dicts["typeDict"]
	typedefDict= dicts["typedefDict"]
	isConst = ""
	isPtr=""
	isArray=""
	isBitfield=""
	typename = ""

	if t[0] == 'c':
		isConst = "const"
		t = t[1:]
	if t[-1] == ']':
		arr = t.index('[')
		isArray = t[arr:]
		t = t[:arr]
	while t[-1] == '*':
		isPtr = isPtr + "*"
		t = t[:-1]
	try:
		colon = t.index(':')
		isBitfield = t[colon:]
		t = t[:colon]
	except ValueError:
		pass	

	if t in callbackDict:
		typename = t
	elif t in structDict:
		typename = structDict[t].attrib.get("name")
	elif t in typedefDict:
		typename = t	# It is typedef'ed so we don't need to convert.
	elif t in typeDict:
		typename = typeDict[t]
	else:
		# Type may be undefined or included from header file...
		# We just accept it as is and let the later compiler stage sort it out.
		# There should be some kind of mechanics behind this though... For renaming etc.
		typename = t

	return [isConst, typename, isPtr, isArray, isBitfield]


def WriteType(f, name: str, t: str, dicts):
	[isConst, typename, isPtr, isArray, isBitfield] = GetTypeName(t, dicts)
	if isConst:
		isConst += " "
	if name:
		tstr = isConst + typename + isPtr + " " + name + isArray + isBitfield
	else:
		tstr = isConst + typename + isPtr + isArray + isBitfield
	f.write(tstr)
	return isPtr

def HeaderCallback(f, cc, dicts):
	name = cc.attrib.get("name")
	r = cc.find("return")
	ret = r.attrib.get("type")
	f.write("typedef ")
	WriteType(f, "", ret, dicts)
	f.write(" (*" + name + ")(")
	first = True
	for a in cc.findall('arg'):
		if not first:
			f.write(", ")
		first = False
		n = a.attrib.get("name")
		t = a.attrib.get("type")
		WriteType(f, n, t, dicts)
	f.write(");\n")

def HeaderCallbacks(f, dicts):
	callbackDict = dicts["callbackDict"]
	f.write("/*\n\tCallbacks.\n*/\n\n")
	for _, cc in callbackDict.items():
		HeaderCallback(f, cc, dicts)
	f.write("\n\n")

def HeaderFunction(f, ff, dicts):
	name = ff.attrib.get("name")
	att = ""
	ret = "void"
	r = ff.find("return")
	if r is not None:
		ret = r.attrib.get("type")
		if ret == "noreturn":
			ret = "void"
			att = "__attribute__ ((noreturn))"
	WriteType(f, "", ret, dicts)
	if att:
		f.write(" " + att)

	f.write(" " + name + "(")
	first = True
	for a in ff.findall('arg'):
		n = a.attrib.get("name")
		t = a.attrib.get("type")
		if n:
			if not first:
				f.write(", ")
			first = False
			WriteType(f, n, t, dicts)
	if first:
		f.write("void")
	f.write(");\n")

def HeaderFunctions(f, functions, dicts):
	for _, ff in functions.items():
		HeaderFunction(f, ff, dicts)
	f.write("\n\n")

def HeaderForwards(f, dicts):
	f.write("/*\n\tTypedef forward declarations of structs and unions.\n*/\n\n")
	structDict = dicts["structDict"]
	for _, ss in structDict.items():
		tagname = ss.tag
		if tagname == "struct" or tagname == "union":
			name = ss.attrib.get("name")
			f.write("typedef " + tagname + " " + name + " " + name + ";\n")
	f.write("\n\n")

def HeaderExterns(f, dicts):
	f.write("/*\n\tExtern declarations of structs and unions.\n*/\n\n")
	structDict = dicts["structDict"]
	for _, ss in structDict.items():
		tagname = ss.tag
		if tagname == "struct" or tagname == "union":
			name = ss.attrib.get("name")
			extern = ss.attrib.get("extern")
			if extern:
				f.write("extern " + name + " " + extern + ";\n")
	f.write("\n\n")


def HeaderTypedefs(f, dicts):
	f.write("/*\n\tTypedefs.\n*/\n\n")
	typedefDict= dicts["typedefDict"]
	for _, ss in typedefDict.items():
		typename = ss.attrib.get("typename")
		type = ss.attrib.get("type")
		f.write("typedef ")
		WriteType(f, typename, type, dicts)
		f.write(";\n")
	f.write("\n\n")

def HeaderIncludes(f, dicts):
	f.write("/*\n\tIncludes.\n*/\n\n")
	includeDict= dicts["includeDict"]
	for n, i in includeDict.items():
		f.write("#include " + n + "\n")
	f.write("\n\n")

def HeaderStructs(f, dicts):
	f.write("/*\n\tStructs and unions.\n*/\n\n")
	structDict = dicts["structDict"]
	for _, ss in structDict.items():
		tagname = ss.tag
		if tagname == "struct" or tagname == "union":
			name = ss.attrib.get("name")
			
			f.write(tagname + " ")
			if (tagname == "struct"):
				f.write("__attribute__((packed)) ")
			f.write(name + "\n{\n")
			for a in ss.findall('member'):
				n = a.attrib.get("name")
				t = a.attrib.get("type")
				f.write("\t")
				WriteType(f, n, t, dicts)
				f.write(";\n")
			f.write("} __attribute__ ((aligned (2)));\n\n")
	f.write("\n\n")

def HeaderCategories(f, dicts):
	categories = dicts["categories"]
	functionDict = dicts["functionDict"]
	defineDict = dicts["defineDict"]
	for c in categories:
		HeaderCategory(f, c)
		if c in defineDict:
			HeaderDefines(f, defineDict[c])
		if c in functionDict:
			HeaderFunctions(f, functionDict[c], dicts)


def WriteHeader(name, dicts):
	with open("gen/" + name + ".h", "w") as f:
		HeaderBegin(f, name)
		HeaderIncludes(f, dicts)
		HeaderForwards(f, dicts)
		HeaderTypedefs(f, dicts)
		HeaderCallbacks(f, dicts)
		HeaderStructs(f, dicts)
		HeaderExterns(f, dicts)
		HeaderCategories(f, dicts)
		HeaderEnd(f, name)
