# atari-libs
System specific C libraries for Atari 16/32 bit computers.
Very little testing have been done so far.
The libraries are generated from xml files, and the specifications can be found in the wiki.

## Build (Linux):
1. Open a terminal.
2. Enter: `python3 gen.py`

This will generate a lot of source files in a folder called "gen".  
Useful header files:  
* "tos.h" contains all GEMDOS, BIOS and XBIOS declarations.
* "aes.h" contains all AES declarations.
* "vdi.h" contains all VDI declarations.
* "def_types.h" contains typedefs for all the types used in the generated files.
* "aes_def.h" is included by "aes.h" and contains global AES symbols etc.
* "vdi_def.h" is included by "vdi.h" and contains global VDI symbols etc.
* "tos.mk" can be included in a makefile to get a list of all TOS source files.
* "aes.mk" can be included in a makefile to get a list of all AES source files.
* "vdi.mk" can be included in a makefile to get a list of all VDI source files.

If you are using [m68k-atari-dev toolchain](https://github.com/hildenborg/m68k-atari-dev), then there is a makefile provided that builds all libraries. That will generate three more files in the "gen" folder:
* "libtos.a" containing all TOS functions.
* "libaes.a" containing all AES functions.
* "libvdi.a" containing all VDI functions.

