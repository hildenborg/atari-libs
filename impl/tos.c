/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "def_types.h"
#include "tos.h"

INT32_T super_sp;

INT32_T Super(void* stack)
{
	register INT32_T result asm ("d0");
	__asm__ volatile (
		"move.l	%%a7, super_sp\n\t"
		"move.l	%1, %%a7@-\n\t"
		"move.w	#32, %%a7@-\n\t"
		"trap	#1\n\t"
		"move.l	super_sp, %%a7\n\t"
		: "=r" (result)
		: "irV" (stack)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}
