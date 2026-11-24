#!/bin/bash

#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

# Usage: build.sh [TOOLKIT_PATH TARGET GEN_PATH BUILD_THREADS]
# Generates, builds and installs the libraries for all multilibs of the toolchain.
# Nothing is installed unless everything could be built.

set -euo pipefail

die() {
	echo "build.sh: error: $*" >&2
	exit 1
}

if (( $# == 4 )); then
	MULTILIB_TOOLKIT=$1
	MULTILIB_TARGET=$2
	GEN_PATH=$3
	BUILD_THREADS=$4
elif (( $# == 0 )); then
	MULTILIB_TOOLKIT=$HOME/toolchain/m68k-atari-elf
	MULTILIB_TARGET=m68k-atari-elf
	GEN_PATH=gen
	BUILD_THREADS=1
else
	echo "Usage: $0 [TOOLKIT_PATH TARGET GEN_PATH BUILD_THREADS]" >&2
	exit 2
fi

[ -n "$GEN_PATH" ] || die "GEN_PATH is empty"
[[ $BUILD_THREADS =~ ^[1-9][0-9]*$ ]] || die "BUILD_THREADS must be a positive number, not '$BUILD_THREADS'"
[ -f gen.py ] || die "gen.py not found, run build.sh from the repository root"
command -v python3 > /dev/null || die "python3 not found"
command -v make > /dev/null || die "make not found"

# -m: also works when the directory doesn't exist yet.
GEN_PATH=$(realpath -m -- "$GEN_PATH") || die "bad GEN_PATH"
GCC=$MULTILIB_TOOLKIT/bin/$MULTILIB_TARGET-gcc
[ -x "$GCC" ] || die "compiler not found: $GCC"
export MULTILIB_TOOLKIT MULTILIB_TARGET GEN_PATH

mkdir -p -- "$GEN_PATH" || die "cannot create $GEN_PATH"
python3 ./gen.py "$GEN_PATH" "$MULTILIB_TARGET" || die "gen.py failed"

# Each line is <path>;<flags>, with the flags separated by @.
multiliblist=$("$GCC" -print-multi-lib) || die "$GCC -print-multi-lib failed"
[ -n "$multiliblist" ] || die "$GCC -print-multi-lib returned no multilibs"
mapfile -t multilibs <<< "$multiliblist"

# Run make for all multilibs, with the given make arguments.
make_multilibs() {
	local line flags
	for line in "${multilibs[@]}"; do
		flags=${line#*;}
		export MULTILIB_PATH=${line%%;*}
		export MULTILIB_FLAGS=${flags//@/ -}
		make "$@" || die "make $* failed for multilib '$MULTILIB_PATH'"
	done
}

make_multilibs -j"$BUILD_THREADS" all
make_multilibs install
