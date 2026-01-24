#!/bin/bash

#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

export MULTILIB_TOOLKIT=$HOME/toolchain/m68k-atari-elf
export MULTILIB_TARGET=m68k-atari-elf
export GEN_PATH=$(readlink -f gen)
BUILD_THREADS=1

if [ ! -d $GEN_PATH ]; then
	mkdir -p $GEN_PATH
fi

python3 ./gen.py $GEN_PATH $MULTILIB_TARGET True

multiliblist="$($MULTILIB_TOOLKIT/bin/$MULTILIB_TARGET-gcc -print-multi-lib)"
while IFS= read -r line 
do
	semi=$(expr index "$line" ";")
	path=${line:0:$semi-1}
	flags=${line:$semi}
	flags=${flags//"@"/" -"}
	export MULTILIB_PATH=$path
	export MULTILIB_FLAGS=$flags
	make -j$BUILD_THREADS
	make install
done <<< "$multiliblist"
