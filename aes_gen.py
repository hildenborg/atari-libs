#	Copyright (C) 2026 Mikael Hildenborg
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

	threaded = ff.attrib.get("threaded")

	if grpid != "2":
		print ("group id: " + grpid + "\n")
		raise ValueError
	ret = "void"
	retVal = ""
	retSrc = "intout"
	r = ff.find("return")
	if r is not None:
		ret = r.attrib.get("type")
		retVal = r.attrib.get("value")
		retSrc = r.attrib.get("src")
		if not retSrc:
			retSrc = "intout"

	with open(build_dir + name + ".c", "w") as f:
		f.write('#include "aes_def.h"\n\n')

		[isPtr, retType] = header_gen.GetTypeString("", ret, dicts)
		funcDecl = retType
		if threaded:
			funcDecl += " mt_" + name + "("
		else:
			funcDecl += " " + name + "("
		intin = 0
		intout = 0
		addrin = 0
		addrout = 0
		clrOutput = ""
		if retSrc == "intout" and not retVal:
			if ret == "int32_t" or ret == "uint32_t":
				clrOutput = "\tintout[0] = 0;\n"
				clrOutput += "\tintout[1] = 0;\n"
				intout = 2
			else:
				if ret != "void":	
					clrOutput = "\tintout[0] = 0;\n"
				intout = 1
		elif retSrc == "addrout":
			clrOutput = "\taddrout[0] = 0;\n"
			addrout = 1
		s_intin = ""
		s_addrin = ""
		s_intout = ""
		s_addrout = ""
		got_global = False
		first = True
		[_, wordType, _, _, _] = header_gen.GetTypeName("int16_t", dicts)
		intinSize = intin
		intoutSize = intout
		addrinSize = addrin
		addroutSize = addrout
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			if n == "aes_global":
				got_global = True
			src = a.attrib.get("src")
			dst = a.attrib.get("dst")
			t = a.attrib.get("type")
			# Base type without the leading "c" (const) marker, for type comparisons below.
			bt = t[1:] if t and t[0] == 'c' else t
			nc = a.attrib.get("nullchk")
			if not n:
				n = a.attrib.get("value")
				isPtr = ""
			else:
				if not first:
					funcDecl += ", "
				first = False
				[isPtr, argType] = header_gen.GetTypeString(n, t, dicts)
				funcDecl += argType
			beginnc = ""
			endnc = ""
			if nc:
				beginnc = "\tif(" + n + " != 0)\n\t{\n\t"
				endnc = "\t}\n"

			# idx places the arg at that word (or address) index, otherwise it follows the previous one.
			# The array sizes are the highest index used.
			idx = a.attrib.get("idx")
			if src == "intout":
				pos = int(idx) if idx else intout
				if (bt == "int16_t*" or bt == "uint16_t*"):
					s_intout += beginnc + "\t*" + n + " = " + "intout[" + str(pos) + "];\n" + endnc
					size = 1
				elif bt == "GRECT*":
					s_intout += beginnc + "\t*" + n + " = " + "*(GRECT*)(&intout[" + str(pos) + "]);\n" + endnc
					size = 4
				else:
					s_intout += beginnc + "\tAES_COPY_LONG(&intout[" + str(pos) + "], " + n + ");\n" + endnc
					size = 2
				intout = pos + size
				intoutSize = max(intoutSize, intout)
			elif src == "addrout":
				pos = int(idx) if idx else addrout
				s_addrout += beginnc + "\t*" + n + " = " + "addrout[" + str(pos) + "];\n" + endnc
				addrout = pos + 1
				addroutSize = max(addroutSize, addrout)
			if dst == "intin":
				pos = int(idx) if idx else intin
				if (bt == "int32_t" or bt == "uint32_t"):
					s_intin += beginnc + "\tAES_COPY_LONG(&" + n + ", &intin[" + str(pos) + "]);\n" + endnc
					size = 2
				elif bt == "GRECT*":
					s_intin += beginnc + "\t*(GRECT*)(&" + "intin[" + str(pos) + "]) = " + isPtr + n + ";\n" + endnc
					size = 4
				elif (bt != "int16_t*" and bt != "uint16_t*") and isPtr:
					s_intin += beginnc + "\tAES_COPY_LONG(&" + n + ", &intin[" + str(pos) + "]);\n" + endnc
					size = 2
				else:
					s_intin += beginnc + "\t" + "intin[" + str(pos) + "] = " + isPtr + n + ";\n" + endnc
					size = 1
				intin = pos + size
				intinSize = max(intinSize, intin)
			elif dst == "addrin":
				pos = int(idx) if idx else addrin
				s_addrin += beginnc + "\t" + "addrin[" + str(pos) + "] = (void*)" + n + ";\n" + endnc
				addrin = pos + 1
				addrinSize = max(addrinSize, addrin)

		intin = intinSize
		intout = max(intout, intoutSize)
		addrin = addrinSize
		addrout = max(addrout, addroutSize)
		f.write(funcDecl)
		if threaded:
			if not first:
				f.write(", ")
			f.write(wordType + "* aes_global")
		if first:
			funcDecl += "void"

		f.write(")\n{\n")

		res = ff.find("reserve")
		if res is not None:
			# Only Intout for now.
			resArr = res.attrib.get("dst")
			resCnt = res.attrib.get("count")
			if resArr == "intout" and resCnt:
				intout = int(resCnt)

		lcl_aespb = "\tAESPB lcl_aespb =\n\t{\n"

		f.write("\t" + wordType + " control[5] = {")
		f.write(str(id) + ", ")
		f.write(str(intin) + ", ")
		f.write(str(intout) + ", ")
		f.write(str(addrin) + ", ")
		f.write(str(addrout))
		f.write("};\n")
		lcl_aespb += "\t\tcontrol,\n"
		if threaded or got_global:
			lcl_aespb += "\t\taes_global,\n"
		else:
			lcl_aespb += "\t\t0,\n"
		if intin > 0:
			f.write("\t" + wordType + " intin[" + str(intin) + "];\n")
			lcl_aespb += "\t\tintin,\n"
		else:
			lcl_aespb += "\t\taes_unused_dummy_int,\n"
		if intout > 0:
			f.write("\t" + wordType + " intout[" + str(intout) + "];\n")
			lcl_aespb += "\t\tintout,\n"
		else:
			lcl_aespb += "\t\taes_unused_dummy_int,\n"
		if addrin > 0:
			f.write("\tvoid* addrin[" + str(addrin) + "];\n")
			lcl_aespb += "\t\taddrin,\n"
		else:
			lcl_aespb += "\t\taes_unused_dummy_addr,\n"
		if addrout > 0:
			f.write("\tvoid* addrout[" + str(addrout) + "];\n")
			lcl_aespb += "\t\taddrout\n"
		else:
			lcl_aespb += "\t\taes_unused_dummy_addr\n"
		lcl_aespb += "\t};\n"
		f.write(lcl_aespb)
			
		f.write(s_intin)
		f.write(s_addrin)
		f.write(clrOutput)
		f.write("\taes_call(&lcl_aespb);\n")
		f.write(s_intout)
		f.write(s_addrout)
		if ret != "void":
			if retVal:
				f.write("\treturn " + retVal + ";\n")
			elif retSrc == "addrout":
				f.write("\treturn addrout[0];\n")
			else:
				if ret != "int16_t" and ret != "uint16_t":
					[_, castType, _, _, _] = header_gen.GetTypeName(ret, dicts)
					f.write("#pragma GCC diagnostic push\n")
					f.write("#pragma GCC diagnostic ignored \"-Wstrict-aliasing\"\n")
					f.write("\treturn *(" + castType + "*)(&intout[0]);\n")
					f.write("#pragma GCC diagnostic pop\n")
				else:
					f.write("\treturn intout[0];\n")
		f.write("}\n\n")

		if threaded:
			f.write(funcDecl.replace("mt_" + name, name))
			f.write(")\n{\n\t")
			if ret != "void":
				f.write("return ")
			f.write("mt_" + name + "(")

			first = True
			for a in ff.findall('arg'):
				n = a.attrib.get("name")
				if n:
					if not first:
						f.write(", ")
					first = False
					f.write(n)
			if not first:
				f.write(", ")
			f.write("0")
			f.write(");\n}\n\n")

