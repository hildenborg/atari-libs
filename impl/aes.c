/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "def_types.h"
#include "aes.h"

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
INT16_T aes_calli(UINT32_T c)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"move.l	%%d1, %%a0\n\t"
		"move.l	%%a0@, %%a0\n\t"
		"movep.l	%1, %%a0@(1)\n\t"
		"clr.w	%%a0@(8)\n\t"
		"move.w	#0xc8, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (&aespb), "r" (c)
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
INT16_T aes_callo(UINT32_T c)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"move.l	%%d1, %%a0\n\t"
		"move.l	%%a0@, %%a0\n\t"
		"movep.l	%1, %%a0@(1)\n\t"
		"move.w	%%a0@(6), %%a0@(8)\n\t"
		"clr.w	%%a0@(6)\n\t"
		"move.w	#0xc8, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (&aespb)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return aesparblk.intout[0];
}

/*
	Special handling for wind_get
*/
INT16_T wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4)
{
	UINT32_T call = (104 << 24) | (2 << 16) | (5 << 8) | 0;
	aesparblk.intin[0] = handle;
	aesparblk.intin[1] = mode;
	if(mode == WF_DCOLOR || mode == WF_COLOR)
	{
		aesparblk.intin[2] = *parm1;
		call = (104 << 24) | (3 << 16) | (5 << 8) | 0;
	}
	INT16_T result = aes_calli(call);
	*parm1 = aesparblk.intout[1];
	*parm2 = aesparblk.intout[2];
	*parm3 = aesparblk.intout[3];
	*parm4 = aesparblk.intout[4];
	return result;
}
