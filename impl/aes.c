/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "aes_def.h"

INT16_T aes_global[16];
void* aes_unused_dummy_addr[16];
INT16_T aes_unused_dummy_int[16];

/*
	Call AES with four bytes in c:
	id,
	intin,
	intout,
	addrin
*/
INT16_T aes_call(AESPB* aespb)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"move.l	#0xc8, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (aespb)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return *(aespb->intout);
}
