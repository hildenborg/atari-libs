/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"
INT16_T unused_dummy_array[16];	// Should never be used.

short vdi_strlen(const void* src)
{
	register INT16_T len asm ("d0");
	__asm__ volatile (
		"move.l	%1, %%a0\n\t"
		"moveq	#0, %%d0\n\t"
		"bra.s	2f\n\t"
		"1:\n\t"
		"addq.w #1, %%d0\n\t"
		"2:\n\t"
		"move.b	%%a0@+, %%d1\n\t"
		"bne.s	1b\n\t"
		: "=r" (len)
		: "g" (src)
		: "d0", "d1", "a0", "cc"
	);
	return len;
}

void vdi_words_to_bytes(const INT16_T* src, INT8_T* dst, INT16_T len)
{
	dst[len]= 0;
	for (INT16_T i = len; --i >= 0; *dst++ = (INT8_T)*src++) {}
}

void vdi_bytes_to_words(const INT8_T* src, INT16_T* dst, INT16_T len)
{
	for (INT16_T i = len; --i >= 0; *dst++ = (INT16_T)*src++) {}
}
