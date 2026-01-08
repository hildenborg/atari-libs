/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

#ifdef TARGET_M68K_ATARI_MINTELF
void v_opnvwk(INT16_T* work_in, INT16_T* handle, INT16_T* work_out)
#else
void v_opnvwk(INT16_T* work_in, INT16_T* handle, WS* work_out)
#endif // TARGET_M68K_ATARI_ELF
{
	vdipb.intin = work_in;
	vdipb.intout = (INT16_T*)work_out;
	vdipb.ptsout = &((INT16_T*)work_out)[45];
	vdiparblk.contrl[0] = 100;
	vdiparblk.contrl[1] = 0;
	vdiparblk.contrl[3] = 11;
	vdiparblk.contrl[5] = 0;
	vdiparblk.contrl[6] = *handle;
	vdi_call();
	*handle = vdiparblk.contrl[6];
	vdipb.intin = vdiparblk.intin;
	vdipb.intout = vdiparblk.intout;
	vdipb.ptsout = vdiparblk.ptsout;
}
