# atari-libs
System specific C libraries for Atari 16/32 bit computers.  
The libraries are generated from xml files, and the specifications can be found in the wiki.  
Extensive testing have been done for commonly used functions, but rarely used functions may be bugged.  
If you are using [m68k-atari-dev toolchain](https://github.com/hildenborg/m68k-atari-dev), then the atari-libs libraries are automatically built and installed.

## Build for m68k-atari-elf toolchain:
1. Open a terminal.
2. Enter: `./build.sh`

This will make a lot of files in a folder called "gen".
It will also build three libraries: "libtos.a", "libaes.a" and "libvdi.a".
Those libraries together with necessary headers is also installed to the toolchain.

## How to use:
If you are using [m68k-atari-dev toolchain](https://github.com/hildenborg/m68k-atari-dev), then you need to add link flags for the libraries you use: "-ltos" "-laes" "-lvdi".  
And you need to include the proper header files in your sources: "#include <tos.h>"  "#include <aes.h>"  "#include <vdi.h>"  
Library and include paths are already set.

## Generate source files:
1. Open a terminal.
2. Enter: `python3 gen.py gen`

This will generate a lot of source files in a folder called "gen".  
Some files that stand out:  
* "tos.h" contains all GEMDOS, BIOS and XBIOS declarations.
* "aes.h" contains all AES declarations.
* "vdi.h" contains all VDI declarations.
* "def_types.h" contains typedefs for all the types used in the generated files.
* "aes_def.h" is included by "aes.h" and contains global AES symbols etc.
* "vdi_def.h" is included by "vdi.h" and contains global VDI symbols etc.
* "tos.mk" can be included in a makefile to get a list of all TOS source files.
* "aes.mk" can be included in a makefile to get a list of all AES source files.
* "vdi.mk" can be included in a makefile to get a list of all VDI source files.

