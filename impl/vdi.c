/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "def_types.h"
#include "vdi.h"

VDIPARBLK vdiparblk;

VDIPB vdipb = 
{
	vdiparblk.contrl,
	vdiparblk.intin,
	vdiparblk.ptsin,
	vdiparblk.intout,
	vdiparblk.ptsout
};

INT16_T vq_gdos(void)
{
	// Shouldn't we do an add #2 after the trap?
	register INT16_T result asm ("d0");
	__asm__ volatile (
		"move.w	#-2, %%a7@-\n\t"
		"trap	#2\n\t"
		"cmp.w	#-2, %%d0\n\t"
		"sne	%%d0\n\t"
		"ext.w	%%d0\n\t"
		: "=r" (result)
		:
		: "d1", "d2", "a0", "a1", "a2"
	);
	return result;
}

INT32_T vq_vgdos(void)
{
	// Shouldn't we do an add #2 after the trap?
	register INT32_T result asm ("d0");
	__asm__ volatile (
		"move.w	#-2, %%a7@-\n\t"
		"trap	#2\n\t"
		: "=r" (result)
		:
		: "d1", "d2", "a0", "a1", "a2"
	);
	return result;
}

void vdi_call(void)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"clr.w	%%a0@(8)\n\t"
		"move.w	#0x73, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (&vdipb)
		: "d0", "d1", "d2", "a0", "a1", "a2"
	);
}

INT16_T vdi_zero_ended_string_to_words(INT8_T* src, INT16_T* dst)
{
	register INT16_T len asm ("d0");
	__asm__ volatile (
		"move.l	%1, %0\n\t"
		"moveq	#0, d1\n\t"
		"bra.s	2f\n\t"
		"1:\n\t"
		"move.w	%%d1, %2@+\n\t"
		"2:\n\t"
		"move.b	%1@+, %%d1\n\t"
		"bne.s	1b\n\t"
		"sub.l	%1, %0\n\t"		// -(len + 1)
		"not.l	%0\n\t"			// !(-(len + 1)) = len
		: "=r" (len)
		: "a" (src), "a" (dst)
		: "d1"
	);
	// len do not include zero at end
	return len;
}

void vdi_words_to_bytes(INT16_T* src, INT8_T* dst, INT16_T len)
{
	__asm__ volatile (
		"bra.s	2f\n\t"
		"1:\n\t"
		"addq.l	#1, %0\n\t"
		"move.b	%0@+, %1@+\n\t"
		"2:\n\t"
		"dbra	%2, 1b\n\t"
		:
		: "a" (src), "a" (dst), "d" (len)
		:
	);
}

void vdi_bytes_to_words(INT8_T* src, INT16_T* dst, INT16_T len)
{
	__asm__ volatile (
		"bra.s	2f\n\t"
		"1:\n\t"
		"clr.b	%1@+\n\t"
		"move.b	%0@+, %1@+\n\t"
		"2:\n\t"
		"dbra	%2, 1b\n\t"
		:
		: "a" (src), "a" (dst), "d" (len)
		:
	);
}
/*
void vdi_large_generic_copy(void* src, void* dst, INT16_T len)
{
	__asm__ volatile (
		"move.w %2, d1\n\t"
		"lsr.w #4, d1\n\t"
		"bra.s	2f\n\t"
		"1:\n\t"
		".rept 8\n\t"
		"move.l	%0@+, %1@+\n\t"
		".endr\n\t"
		"2:\n\t"
		"dbra	d1, 1b\n\t"

		"move.w %2, d1\n\t"
		"lsr.w #1, d1\n\t"
		"and.w #7, d1\n\t"
		"bra.s	2f\n\t"
		"1:\n\t"
		"move.l	%0@+, %1@+\n\t"
		"2:\n\t"
		"dbra	d1, 1b\n\t"

		"and.w	#1, %2\n\t"
		"beq.s	1f\n\t"
		"move.w	%0@+, %1@+\n\t"
		"1:\n\t"
		:
		: "a" (src), "a" (dst), "d" (len)
		:
	);
}
*/