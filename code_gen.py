#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import header_gen
import vdi_gen
import tos_gen
import aes_gen

def WriteCode(name, options, dicts):
	functionDict = dicts["functionDict"]
	for c in functionDict:
		for _, ff in functionDict[c].items():
			oh = ff.attrib.get("onlyheader")
			if not oh:
				if name == "tos":
					tos_gen.CodeTosFunction(name, ff, dicts)
				elif name == "aes":
					aes_gen.CodeAESFunction(name, ff, dicts)
				elif name == "vdi":
					vdi_gen.CodeVDIFunction(name, ff, dicts, options)

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

