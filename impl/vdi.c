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
		: "d1", "d2", "a0", "a1", "a2", "cc"
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
		: "d1", "d2", "a0", "a1", "a2", "cc"
	);
	return result;
}

void v_opnvwk(INT16_T* work_in, INT16_T* handle, WS* work_out)
{
	#ifdef FAST_VDI
		vdipb.intin = work_in;
		vdipb.intout = (INT16_T*)work_out;
		vdipb.ptsout = &((INT16_T*)work_out)[45];
	#else
		VDI_COPY_WORDS(work_in, &vdiparblk.intin[0], 11);
	#endif
	vdiparblk.contrl[0] = 100;
	vdiparblk.contrl[1] = 0;
	vdiparblk.contrl[3] = 11;
	vdiparblk.contrl[5] = 0;
	vdiparblk.contrl[6] = *handle;
	vdi_call();
	*handle = vdiparblk.contrl[6];
	#ifdef FAST_VDI
		vdipb.intin = vdiparblk.intin;
		vdipb.intout = vdiparblk.intout;
		vdipb.ptsout = vdiparblk.ptsout;
	#else
		VDI_COPY_WORDS(&vdiparblk.intout[0], work_out, 45);
		VDI_COPY_LONGS(&vdiparblk.ptsout[0], &((INT16_T*)work_out)[45], 6);
	#endif
}

void vs_clip(INT16_T handle, INT16_T clip_flag, INT16_T* xyarray)
{
	vdiparblk.contrl[6] = handle;
	vdiparblk.intin[0] = clip_flag;
	if (clip_flag != 0 && xyarray != 0)
	{
	#ifdef FAST_VDI
		vdipb.ptsin = xyarray;
	#else
		VDI_COPY_LONGS(xyarray, &vdiparblk.ptsin[0], 2);
	#endif
	}
	vdiparblk.contrl[0] = 129;
	vdiparblk.contrl[1] = 2;
	vdiparblk.contrl[3] = 1;
	vdiparblk.contrl[5] = 0;
	vdi_call();
	#ifdef FAST_VDI
		vdipb.ptsin = vdiparblk.ptsin;
	#endif
}

#ifdef DEBUG
void CheckVdipb(void)
{
	if (vdipb.ptsin != vdiparblk.ptsin || vdipb.ptsout != vdiparblk.ptsout ||
		vdipb.intin != vdiparblk.intin || vdipb.intout != vdiparblk.intout ||
		vdipb.contrl != vdiparblk.contrl)
	{
		// Stop execution so we can debug.
		asm ("illegal");
	}
}
#endif

void vdi_call(void)
{
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"moveq	#0x73, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "i" (&vdipb)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
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
