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

	with open(build_dir + name + ".c", "w") as f:
		f.write('#include "' + iname + '.h"\n\n')
		header_gen.WriteType(f, "", ret, dicts)
		f.write(" " + name + "(")
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
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			src = a.attrib.get("src")
			dst = a.attrib.get("dst")
			t = a.attrib.get("type")
			if not first:
				f.write(", ")
			first = False
			isPtr = header_gen.WriteType(f, n, t, dicts)
			if src == "intout":
				s_intout = s_intout + "\t*" + n + " = aesparblk.intout[" + str(intout) + "];\n"
				intout = intout + 1
			elif src == "addrout":
				s_addrout = s_addrout + "\t*" + n + " = aesparblk.addrout[" + str(addrout) + "];\n"
				addrout = addrout + 1
			if dst == "intin":
				s_intin = s_intin + "\taesparblk.intin[" + str(intin) + "] = " + isPtr + n + ";\n"
				intin = intin + 1
			elif dst == "addrin":
				s_addrin = s_addrin + "\taesparblk.addrin[" + str(addrin) + "] = (void*)" + n + ";\n"
				addrin = addrin + 1

		if first:
			f.write("void")
		f.write(")\n{\n")

		f.write(s_intin)
		f.write(s_addrin)
		f.write("\t")
		if ret != "void":
			f.write("short result = ")
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
		f.write(");\n")
		f.write(s_intout)
		f.write(s_addrout)
		if ret != "void":
			f.write("\treturn result;\n")
		f.write("}\n\n")

