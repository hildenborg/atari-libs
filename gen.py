#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

import xml.etree.ElementTree as ET
import os
import sys
import shutil
import header_gen
import code_gen

def MakeDicts():
	dicts = {
		"typeDict": {},
		"settingsDict": {},
		"callbackDict": {},
		"defineDict": {},
		"structDict": {},
		"functionDict": {},
		"categories": {},
		"targetDict": {},
		"typedefDict": {},
		"includeDict": {}
	}
	return dicts

def ReadGlobals(xmlfile, target, dicts):
	tree = ET.parse(xmlfile)
	root = tree.getroot()

	for t in root.findall('target'):
		n = t.attrib.get("name")
		if n == target:
			# Setup type dict.
			for tp in t.findall('types/type'):
				n = tp.attrib.get("name")
				u = tp.attrib.get("use")
				dicts["typeDict"][n] = u
			# Setup settings dict.
			for tp in t.findall('settings/set'):
				n = tp.attrib.get("name")
				v = tp.attrib.get("value")
				dicts["settingsDict"][n] = v
			break

def FindFunctionCategory(name, dicts):
	functionDict = dicts["functionDict"]
	for c, d in functionDict.items():
		if name in d:
			return c
	return None

def FindDefenitionCategory(name, dicts):
	defineDict = dicts["defineDict"]
	for c, d in defineDict.items():
		if name in d:
			return c
	return None

def OverrideFunction(oo, dicts):
	functionDict = dicts["functionDict"]
	n = oo.attrib.get("name")
	c = FindFunctionCategory(n, dicts)
	if c is not None:
		f = functionDict[c][n]
		oo.set("id", f.attrib.get("id"))
		oo.set("grpid", f.attrib.get("grpid"))
		functionDict[c][n] = oo
	else:
		print("Cannot override function: " + n + " as no such function exists.")
		raise ValueError

def AddToDicts(el, dicts):
	callbackDict = dicts["callbackDict"]
	defineDict = dicts["defineDict"]
	structDict = dicts["structDict"]
	functionDict = dicts["functionDict"]
	categories = dicts["categories"]
	targetDict= dicts["targetDict"]
	typedefDict= dicts["typedefDict"]
	includeDict= dicts["includeDict"]
	# Setup include dict.
	for i in el.findall('include'):
		n = i.attrib.get("name")
		includeDict[n] = i
	# Setup struct dict.
	for s in el.findall('structs'):
		for ss in s:
			n = ss.attrib.get("name")
			if ss.tag == "callback":
				callbackDict[n] = ss
			elif ss.tag == "typedef":
				tn = ss.attrib.get("typename")
				typedefDict[tn] = ss
			else:
				structDict[n] = ss
	# Setup defines dict.
	for d in el.findall('defines'):
		c = d.attrib.get("category")
		categories[c] = True
		if not c in defineDict:
			defineDict[c] = {}
		for dd in d.findall('define'):
			n = dd.attrib.get("name")
			v = dd.attrib.get("value")
			defineDict[c][n] = v
	# Setup functions dict.
	for f in el.findall('functions'):
		c = f.attrib.get("category")
		i = f.attrib.get("grpid")
		categories[c] = True
		if not c in functionDict:
			functionDict[c] = {}
		for ff in f.findall('function'):
			n = ff.attrib.get("name")
			ff.set("grpid", i)
			functionDict[c][n] = ff

	if el.tag != "target":
		# Setup targets dict.
		for t in el.findall('targets/target'):
			n = t.attrib.get("name")
			targetDict[n] = t
	else:
		for o in el.findall('overrides'):
			for oo in o.findall('override'):
				OverrideFunction(oo, dicts)
		# Rename types/structs/functions/defines
		for r in el.findall('renames/rename'):
			n = r.attrib.get("name")
			nn = r.attrib.get("newname")
			fc = FindFunctionCategory(n, dicts)
			dc = FindDefenitionCategory(n, dicts)
			if n in structDict:
				structDict[n].set("name", nn)
			elif fc is not None:
				functionDict[fc][n].set("name", nn)
			elif dc is not None:
				defineDict[dc][n].set("name", nn)
			else:
				print("Cannot rename: " + n + " as no such name can be found.")
				raise ValueError

def ReadDefenitions(xmlfile, targetname, dicts):
	tree = ET.parse(xmlfile)
	root = tree.getroot()
	AddToDicts(root, dicts)
	# Apply target
	if targetname in dicts["targetDict"]:
		target = dicts["targetDict"][targetname]
		AddToDicts(target, dicts)

def Generate(name, build_dir, target, impl):
	dicts = MakeDicts()
	ReadGlobals("xml/global.xml", target, dicts)
	ReadDefenitions("xml/" + name + ".xml", target, dicts)
	header_gen.WriteHeader(name, build_dir, dicts)
	code_gen.WriteCode(name, build_dir, dicts)
	code_gen.WriteMakefileInc(name, build_dir, dicts, impl)
	for n in impl:
		shutil.copyfile("impl/" + n, build_dir + n)

def GenerateGlobals(name, build_dir, target):
	dicts = MakeDicts()
	ReadGlobals("xml/global.xml", target, dicts)
	with open(build_dir + name + ".h", "w") as f:
		header_gen.HeaderBegin(f, name)
		f.write("\n")
		deftarget = target.replace("-", "_")
		f.write("#define TARGET_" + deftarget.upper() + "\n")
		for s, v in dicts["settingsDict"].items():
			if (v != "False"):
				f.write("#define " + s + " " + v + "\n")
		f.write("\n")
		for t, u in dicts["typeDict"].items():
			f.write("typedef " + u + " " + t.upper() + ";\n")

		f.write("\n")
		header_gen.HeaderEnd(f, name)

def main():
	if len(sys.argv) >=2:
		build_dir = sys.argv[1]
		if not build_dir.endswith("/"):
			build_dir += "/"
		target = sys.argv[2]
	else:
		build_dir = "gen/"
		target = "m68k-atari-elf"

	try:
		os.mkdir(build_dir)
	except FileExistsError:
		pass

	GenerateGlobals("def_types", build_dir, target)
	Generate("tos", build_dir, target, [])
	Generate("aes", build_dir, target, ["aes.c", "aes_def.h", "wind_get.c"])
	Generate("vdi", build_dir, target, ["vdi.c", "vdi_def.h", "v_opnvwk.c", "vq_vgdos.c", "vq_gdos.c", "vs_clip.c", "vsm_locator.c"])
#	Generate("line_a", target, ["line_a.c", "line_a_def.h"])

if __name__ == "__main__":
	main()


