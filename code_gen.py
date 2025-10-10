import header_gen
import vdi_gen

def CodeTosFunction(iname, ff, dicts):
	name = ff.attrib.get("name")
	id = ff.attrib.get("id")		# Trap function
	grpid = ff.attrib.get("grpid")	# Trap num
	if grpid != "1" and grpid != "13" and grpid != "14":
		print ("group id: " + grpid + "\n")
		raise ValueError
	att = ""
	fncEnd = ""
	ret = "void"
	r = ff.find("return")
	if r is not None:
		ret = r.attrib.get("type")
		if ret == "noreturn":
			ret = "void"
			att = "__attribute__ ((noreturn))"
			fncEnd = "\t__builtin_unreachable();\n"
	with open("gen/" + name + ".c", "w") as f:
		f.write('#include "' + iname + '.h"\n\n')
		header_gen.WriteType(f, "", ret, dicts)
		if att:
			f.write(" " + att)
		f.write(" " + name + "(")
		first = True
		argnum = 0
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			t = a.attrib.get("type")
			if n:
				if not first:
					f.write(", ")
				first = False
				header_gen.WriteType(f, n, t, dicts)
				argnum = argnum + 1
		if first:
			f.write("void")
		f.write(")\n{\n")

		if ret != "void":
			argnum = argnum + 1
			f.write("\tregister ")
			header_gen.WriteType(f, "result", ret, dicts)
			f.write(' asm ("d0");\n')

		f.write("\t__asm__ volatile (\n")

		stackadd = 2
		for a in reversed(ff.findall('arg')):
			t = a.attrib.get("type")
			v = a.attrib.get("value")

			if t == "int16_t" or t == "uint16_t":
				f.write('\t\t"move.w')
				stackadd = stackadd + 2
			else:
				f.write('\t\t"move.l')
				stackadd = stackadd + 4
			if v:
				f.write('\t#' + v)
			else:
				argnum = argnum - 1
				f.write('\t%' + str(argnum))
			f.write(', %%a7@-\\n\\t"\n')

		f.write('\t\t"move.w\t#' + str(id) + ', %%a7@-\\n\\t"\n')
		f.write('\t\t"trap\t#' + str(grpid) + '\\n\\t"\n')

		if stackadd <= 8:
			f.write('\t\t"addq.l\t#' + str(stackadd) + ', %%a7\\n\\t"\n')
		else:
			f.write('\t\t"lea\t%%a7@(' + str(stackadd) + '), %%a7\\n\\t"\n')

		argnum = 0
		if ret != "void":
			argnum = 1
			f.write('\t\t: "=r" (result)\n')
		else:
			f.write('\t\t:\n')

		f.write("\t\t: ")
		first = True
		for a in ff.findall('arg'):
			n = a.attrib.get("name")
			if n:
				if not first:
					f.write(", ")
				first = False
#				f.write('"r" (' + n + ')')
				f.write('"g" (' + n + ')')

		if ret != "void":
			f.write('\n\t\t: "d1", "d2", "a0", "a1", "a2"\n\t);\n')
			if fncEnd:
				f.write(fncEnd)
			f.write("\treturn result;\n")
		else:
			f.write('\n\t\t: "d0", "d1", "d2", "a0", "a1", "a2"\n\t);\n')
			if fncEnd:
				f.write(fncEnd)

			
		f.write("}\n\n")

# non pointer (always in, error if out) = intin
# pointer in = addrin
# int16_t pointer inout = intin, intout
# int16_t pointer out = intout
# pointer addout = addrout (only rsrc_gaddr!)
def CodeAESFunction(iname, ff, dicts):
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

	with open("gen/" + name + ".c", "w") as f:
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

def WriteCode(name, dicts):
	functionDict = dicts["functionDict"]
	for c in functionDict:
		for _, ff in functionDict[c].items():
			oh = ff.attrib.get("onlyheader")
			if not oh:
				if name == "tos":
					CodeTosFunction(name, ff, dicts)
				elif name == "aes":
					CodeAESFunction(name, ff, dicts)
				elif name == "vdi":
					vdi_gen.CodeVDIFunction(name, ff, dicts)

def WriteMakefileInc(name, dicts, impl):
	functionDict = dicts["functionDict"]
	with open("gen/" + name + ".mk", "w") as f:
		f.write(name.upper() + "_SOURCES :=")
		counter = 0
		for c in functionDict:
			for _, ff in functionDict[c].items():
				n = ff.attrib.get("name")
				oh = ff.attrib.get("onlyheader")
				if not oh:
					f.write(" ")
					if counter == 8:
						counter = 0
						f.write("\\\n\t")
					else:
						counter = counter + 1
					f.write(n + ".c")
		for n in impl:
			if ".h" in n:
				pass
			else:
				f.write(" ")
				if counter == 8:
					counter = 0
					f.write("\\\n\t")
				else:
					counter = counter + 1
				f.write(n)

		f.write("\n")

