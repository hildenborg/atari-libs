#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import header_gen

def CodeTosFunction(iname, ff, dicts):
	name = ff.attrib.get("name")
	id = ff.attrib.get("id")		# Trap function
	grpid = ff.attrib.get("grpid")	# Trap num
	if grpid != "1" and grpid != "13" and grpid != "14":
		print ("group id: " + grpid + "\n")
		raise ValueError
	clobbers = '"d1", "d2", "a0", "a1", "a2"'
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
			clobbers = '"d0", ' + clobbers
	else:
		clobbers = '"d0", ' + clobbers

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
				f.write('"irV" (' + n + ')')

		clobbers += ', "cc", "memory"'
		f.write('\n\t\t: ' + clobbers + '\n\t);\n')
		if fncEnd:
			f.write(fncEnd)
		if ret != "void":
			f.write("\treturn result;\n")
			
		f.write("}\n\n")
