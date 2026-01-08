/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

void vs_clip(INT16_T handle, INT16_T clip_flag, INT16_T* xyarray)
{
	vdiparblk.contrl[6] = handle;
	vdiparblk.intin[0] = clip_flag;
	if (clip_flag != 0 && xyarray != 0)
	{
		vdipb.ptsin = xyarray;
	}
	vdiparblk.contrl[0] = 129;
	vdiparblk.contrl[1] = 2;
	vdiparblk.contrl[3] = 1;
	vdiparblk.contrl[5] = 0;
	vdi_call();
	vdipb.ptsin = vdiparblk.ptsin;
}
