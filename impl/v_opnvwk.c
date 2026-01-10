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
	INT16_T contrl[16];
	VDIPB lcl_vdipb =
	{
		contrl,
		work_in,
		vdiparblk.ptsin,	// Unused.
		(INT16_T*)work_out,
		&((INT16_T*)work_out)[45]
	};
	contrl[0] = 100;
	contrl[1] = 0;
	contrl[3] = 11;
	contrl[5] = 0;
	contrl[6] = *handle;
	vdi_call(&lcl_vdipb);
	*handle = vdiparblk.contrl[6];
}
