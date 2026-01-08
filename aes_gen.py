#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import header_gen

# non pointer (always in, error if out) = intin
# pointer in = addrin
# int16_t pointer inout = intin, intout
# int16_t pointer out = intout
# pointer addout = addrout (only rsrc_gaddr!)
def CodeAESFunction(iname, build_dir, ff, dicts):
	name = ff.attrib.get("name")
	id = ff.attrib.get("id")		# AES function
	grpid = ff.attrib.get("grpid")	# Trap num (always 2 for aes)

	flagTreadSafe = header_gen.GetSetting(dicts, "flagTreadSafe")
	threaded = ff.attrib.get("threaded")
	if not threaded:
		# only for functions that ask for it.
		flagTreadSafe = False

	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError
	ret = "void"
	r = ff.find("return")
	if r is not None:
		ret = r.attrib.get("type")

	usesAddrOut = False
	for a in ff.findall('arg'):
		src = a.attrib.get("src")
		if src == "addrout":
			usesAddrOut = True

	aesparblk = ""
	if not flagTreadSafe:
		aesparblk = "aesparblk."

	with open(build_dir + name + ".c", "w") as f:
		f.write('#include "aes_def.h"\n\n')

		[isPtr, retType] = header_gen.GetTypeString("", ret, dicts)
		funcDecl = retType
		if flagTreadSafe:
			funcDecl += " mt_" + name + "("
		else:
			funcDecl += " " + name + "("
		intin = 0
		intout = 0
		if ret != "void":
			intout = 1
		addrin = 0
		addrout = 0
		s_intin = ""
		s_addrin = ""
		s_intout = ""
		s_addrout = ""
		first = True
		[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			src = a.attrib.get("src")
			dst = a.attrib.get("dst")
			t = a.attrib.get("type")
			if not first:
				funcDecl += ", "
			first = False
			[isPtr, argType] = header_gen.GetTypeString(n, t, dicts)
			funcDecl += argType
			if src == "intout":
				s_intout = s_intout + "\t*" + n + " = " + aesparblk + "intout[" + str(intout) + "];\n"
				intout = intout + 1
			elif src == "addrout":
				s_addrout = s_addrout + "\t*" + n + " = " + aesparblk + "addrout[" + str(addrout) + "];\n"
				addrout = addrout + 1
			if dst == "intin":
				if (t == "int32_t" or t == "uint32_t") and not isPtr:
					[_, longType, _, _, _] = header_gen.GetTypeName("int32_t", dicts)
					s_intin = s_intin + "\t*(" + longType + "*)(&" + aesparblk + "intin[" + str(intin) + "]) = " + isPtr + n + ";\n"
					intin = intin + 2
				else:
					s_intin = s_intin + "\t" + aesparblk + "intin[" + str(intin) + "] = " + isPtr + n + ";\n"
					intin = intin + 1
			elif dst == "addrin":
				s_addrin = s_addrin + "\t" + aesparblk + "addrin[" + str(addrin) + "] = (void*)" + n + ";\n"
				addrin = addrin + 1

		f.write(funcDecl)
		if flagTreadSafe:
			if not first:
				f.write(", ")
			f.write(wordType + "* aes_global")
		if first:
			funcDecl += "void"

		f.write(")\n{\n")

		aespb = "aespb"
		if flagTreadSafe:
			aespb = "lcl_aespb"
			lcl_aespb = "\tAESPB lcl_aespb =\n\t{\n"

			f.write("\t" + wordType + " control[5];\n")
			lcl_aespb += "\t\tcontrol,\n"
			lcl_aespb += "\t\taes_global,\n"
			if intin > 0:
				f.write("\t" + wordType + " intin[" + str(intin) + "];\n")
				lcl_aespb += "\t\tintin,\n"
			else:
				lcl_aespb += "\t\taesparblk.intin,\t// Unused.\n"
			if intout > 0:
				f.write("\t" + wordType + " intout[" + str(intout) + "];\n")
				lcl_aespb += "\t\tintout,\n"
			else:
				lcl_aespb += "\t\taesparblk.intout,\t// Unused.\n"
			if addrin > 0:
				f.write("\tvoid* addrin[" + str(addrin) + "];\n")
				lcl_aespb += "\t\taddrin,\n"
			else:
				lcl_aespb += "\t\taesparblk.addrin,\t// Unused.\n"
			if addrout > 0:
				f.write("\tvoid* addrout[" + str(addrout) + "];\n")
				lcl_aespb += "\t\taddrout\n"
			else:
				lcl_aespb += "\t\taesparblk.addrout\t// Unused.\n"
			lcl_aespb += "\t};\n"
			f.write(lcl_aespb)
			
		f.write(s_intin)
		f.write(s_addrin)
		f.write("\t")
		if ret != "void":
			f.write(retType + " result = ")
		# There are no calls that use addrin and addrout at the same time
		if usesAddrOut:
			f.write("aes_callo(")
			f.write("(" + str(id) + " << 24) | ")
			f.write("(" + str(intin) + " << 16) | ")
			f.write("(" + str(intout) + " << 8) | ")
			f.write(str(addrout))
		else:
			f.write("aes_calli(")
			f.write("(" + str(id) + " << 24) | ")
			f.write("(" + str(intin) + " << 16) | ")
			f.write("(" + str(intout) + " << 8) | ")
			f.write(str(addrin))
		f.write(", &" + aespb + ");\n")
		f.write(s_intout)
		f.write(s_addrout)
		if ret != "void":
			f.write("\treturn result;\n")
		f.write("}\n\n")
		if flagTreadSafe:
			f.write("\n\n")
			f.write(funcDecl.replace("mt_" + name, name))
			f.write(")\n{\n\t")
			if ret != "void":
				f.write("return ")
			f.write("mt_" + name + "(")

			first = True
			for a in ff.findall('arg'):
				n = a.attrib.get("name")
				if not first:
					f.write(", ")
				first = False
				f.write(n)
			if not first:
				f.write(", ")
			f.write("aesparblk.global")
			f.write(");\n}\n\n")

