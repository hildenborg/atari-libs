/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

void vs_clip(INT16_T handle, INT16_T clip_flag, INT16_T* xyarray)
{
	INT16_T contrl[16];
	INT16_T intin[1];
	VDIPB lcl_vdipb =
	{
		contrl,
		intin,
		unused_dummy_array,
		unused_dummy_array,
		unused_dummy_array
	};
	contrl[6] = handle;
	intin[0] = clip_flag;
	if (clip_flag != 0 && xyarray != 0)
	{
		lcl_vdipb.ptsin = xyarray;
	}
	contrl[0] = 129;
	contrl[1] = 2;
	contrl[3] = 1;
	contrl[5] = 0;
	vdi_call(&lcl_vdipb);
}
