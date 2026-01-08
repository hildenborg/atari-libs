/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "aes_def.h"

AESPARBLK aesparblk;

AESPB aespb = 
{
	aesparblk.contrl,
	aesparblk.global,
	aesparblk.intin,
	aesparblk.intout,
	aesparblk.addrin,
	aesparblk.addrout
};

/*
	Call AES with four bytes in c:
	id,
	intin,
	intout,
	addrin
*/
INT16_T aes_calli(UINT32_T c, AESPB* aespb)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"move.l	%1, %%d2\n\t"
		"move.l	%%d1, %%a0\n\t"
		"move.l	%%a0@, %%a0\n\t"
		"movep.l %%d2, %%a0@(1)\n\t"
		"clr.w	%%a0@(8)\n\t"
		"move.l	#0xc8, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (aespb), "g" (c)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return aesparblk.intout[0];
}

/*
	Call AES with four bytes in c:
	id,
	intin,
	intout,
	addrout
*/
INT16_T aes_callo(UINT32_T c, AESPB* aespb)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"move.l	%1, %%d2\n\t"
		"move.l	%%d1, %%a0\n\t"
		"move.l	%%a0@, %%a0\n\t"
		"movep.l %%d2, %%a0@(1)\n\t"
		"move.w	%%a0@(6), %%a0@(8)\n\t"
		"clr.w	%%a0@(6)\n\t"
		"move.l	#0xc8, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (aespb), "g" (c)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return aesparblk.intout[0];
}
