#!/bin/bash

#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

export MULTILIB_TOOLKIT=$1
export MULTILIB_TARGET=$2
export GEN_PATH=$(readlink -f $3)
BUILD_THREADS=$4

python3 ./gen.py $GEN_PATH

multiliblist="$($MULTILIB_TOOLKIT/bin/$MULTILIB_TARGET-gcc -print-multi-lib)"
while IFS= read -r line 
do
	semi=$(expr index "$line" ";")
	path=${line:0:$semi-1}
	flags=${line:$semi}
	flags=${flags//"@"/" -"}
	export MULTILIB_PATH=$path
	export MULTILIB_FLAGS=$flags
	make -j$(BUILD_THREADS)
	make install
done <<< "$multiliblist"
