/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"
INT16_T unused_dummy_array[16];	// Should never be used.

void vdi_call(VDIPB* vdipb)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"moveq	#0x73, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (vdipb)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
}

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


INT16_T vdi_zero_ended_string_to_words(const INT8_T* src, INT16_T* dst)
{
	register INT16_T len asm ("d0");
	__asm__ volatile (
		"move.l	%1, %%a0\n\t"
		"move.l	%2, %%a1\n\t"
		"move.l	%%a0, %0\n\t"
		"moveq	#0, %%d1\n\t"
		"bra.s	2f\n\t"
		"1:\n\t"
		"move.w	%%d1, %%a1@+\n\t"
		"2:\n\t"
		"move.b	%%a0@+, %%d1\n\t"
		"bne.s	1b\n\t"
		"sub.l	%%a0, %0\n\t"		// -(len + 1)
		"not.l	%0\n\t"			// !(-(len + 1)) = len
		: "=r" (len)
		: "g" (src), "g" (dst)
		: "d1", "a0", "a1", "cc", "memory"
	);
	// len do not include zero at end
	return len;
}

void vdi_words_to_bytes(const INT16_T* src, INT8_T* dst, INT16_T len)
{
	for (INT16_T i = len; --i >= 0; *dst++ = (INT8_T)*src++) {}
}

void vdi_bytes_to_words(const INT8_T* src, INT16_T* dst, INT16_T len)
{
	for (INT16_T i = len; --i >= 0; *dst++ = (INT16_T)*src++) {}
}
