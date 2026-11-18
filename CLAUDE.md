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

## Generator

- `gen.py` main: per API (`tos`, `aes`, `vdi`) it loads `global.xml` types/settings, collects the API xml into dicts
  (`AddToDicts`), then applies the matching `<target>` (overrides first, then renames).
  `header_gen.py` writes `<api>.h`, `code_gen.py` dispatches each non-`onlyheader` function to
  `tos_gen.py` / `aes_gen.py` / `vdi_gen.py` (one `.c` per function) and writes `<api>.mk`. Files in `impl/` are then copied over the generated ones.
- Run `gen.py` from the repo root (xml and `impl/` paths are relative). With no arguments it builds into `gen/` with testing enabled.
- Types beginning with lowercase `c` mean const. Generator code that compares types must strip that prefix first (e.g. `cGRECT*` vs `GRECT*`).
- An `<override>` always takes `id`/`subid`/`grpid` from the original function and inherits its other attributes (`threaded`, `onlyheader`, ...) unless set on the override.
- A `<rename>` keeps the dict key as the original xml name (so type references to it still resolve) and only changes the emitted name;
  defines are re-keyed. Renaming to a name that already exists is an error.
- A `<sequence>` becomes an array arg of its own (`vdi_gen.PreprocessSequences`): `idx` is its index in the vdi array, like any arg;
  the word offset into the parent pointer is stored separately (`seqOffset`), and its `name` is a pointer expression to that data.
- ptsin/ptsout counts in contrl are in points (2 words); local pts arrays must be an even number of words.

## Verifying generator changes

- `build.sh` installs into the toolchain. To check changes without installing, generate into a scratch dir before and after the change and diff:
  `python3 gen.py <dir> m68k-atari-elf` (also `m68k-atari-mintelf`, which is a `<target>` in the xml files).
- Compile without installing: `make GEN_PATH=<dir> all`.
- Only the m68k-atari-elf toolchain is installed; mintelf output can at most be syntax checked with the elf compiler (`-fsyntax-only -I<dir>`).

## Pitfalls

- `line_a.xml` and everything related to line-a functions have been abandoned and should be ignored.
- Allow `onlyheader` functions in xml to have `id` attribute even if it isn't needed.
- Allow repeated `id` functions as long as they have different names.

## Testing

- Libraries can only be verified in Hatari emulator.
- A gemdos drive for Hatari with NVDI installed is at: `$HOME/testhd`
- An image of emutos can be found as: `$HOME/tos/etos512se.img`
- An example `makefile` and `main.c` for building Atari PRG files with the installed libraries is in: `test/`
- TOS library tests: `test/tos/run.sh -g <gen dir> [config ...]` builds the test programs against a generated and built
  library (`make GEN_PATH=<dir> all`), runs them headless in Hatari on st-emutos, ste-tos206, tt-emutos, falcon-tos404
  and falcon-emutos (TOS images from `$HOME/tos`), and prints failures. Without `-g` it tests the installed library.
  Results come back through Hatari NatFeats, the harness (`test/tos/harness.c`) only uses raw traps, never the library.
  Each test program reports per function; a crash only loses that program's results (`CRASHED?` shows the last call).
