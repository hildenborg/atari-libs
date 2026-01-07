#!/bin/bash

#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

if (( $# == 4 )); then
	export MULTILIB_TOOLKIT=$1
	export MULTILIB_TARGET=$2
	export GEN_PATH=$(readlink -f $3)
	BUILD_THREADS=$4
else
	export MULTILIB_TOOLKIT=$HOME/toolchain/m68k-atari-elf
	export MULTILIB_TARGET=m68k-atari-elf
	export GEN_PATH=$(readlink -f gen)
	BUILD_THREADS=1
fi

if [ ! -d $GEN_PATH ]; then
	mkdir -p $GEN_PATH
fi

python3 ./gen.py $GEN_PATH $MULTILIB_TARGET

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
