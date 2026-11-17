# Atari system libraries

C libraries for Atari TOS, AES and VDI calls.
Also supports NVDI and other newer APIs.

## Build

    ./build.sh

- `gen.py` reads `xml/aes.xml`, `xml/tos.xml`, `xml/vdi.xml` and `xml/global.xml`.
  The xml files are then used to generate C calls for the various Atari system functions.
  The C files generated are together with C files found in `impl/` compiled into libraries.
  The libraries are compiled with the **m68k-atari-elf** toolchain and installed into the `$HOME/toolchain/m68k-atari-elf/` path.
- Specifications for the xml files is on the wiki: `https://github.com/hildenborg/atari-libs/wiki`+

## Pitfalls

- `line_a.xml` and everything related to line-a functions have been abandoned and should be ignored.
- Allow `onlyheader` functions in xml to have `id` attribute even if it isn't needed.
- Allow repeated `id` functions as long as they have different names.

## Testing

- Libraries can only be verified in Hatari emulator.
- A gemdos drive for Hatari with NVDI installed is at: `$HOME/testhd`
- An image of emutos can be found as: `$HOME/tos/etos512se.img`
- An example `makefile` and `main.c` for building Atari PRG files with the installed libraries is in: `test/`
