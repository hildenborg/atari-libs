#!/bin/bash

#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

TOOLKIT=$1
TARGET=$2

python3 ./gen.py

export MULTILIB_TARGET=$TARGET
export MULTILIB_TOOLKIT = TOOLKIT

multiliblist="$($TOOLKIT/bin/$TARGET-gcc -print-multi-lib)"
while IFS= read -r line 
do
	semi=$(expr index "$line" ";")
	path=${line:0:$semi-1}
	flags=${line:$semi}
	flags=${flags//"@"/" -"}
	export MULTILIB_PATH=$path
	export MULTILIB_FLAGS=$flags
	make
	make install
done <<< "$multiliblist"
