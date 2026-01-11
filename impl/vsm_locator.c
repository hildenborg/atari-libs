/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

INT16_T vsm_locator(INT16_T handle, INT16_T x, INT16_T y, INT16_T* xout, INT16_T* yout, INT16_T* term)
{
	INT16_T lcl_ptsin[2];
	lcl_ptsin[0] = x;
	lcl_ptsin[1] = y;
	INT16_T lcl_ptsout[2];
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 28;
	lcl_contrl[1] = 2;
	lcl_contrl[3] = 0;
	lcl_contrl[5] = 0;
	lcl_contrl[6] = handle;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		unused_dummy_array,
		lcl_ptsin,
		term,
		lcl_ptsout
	};
	vdi_call(&lcl_vdipb);

	*xout = lcl_ptsout[0];
	*yout = lcl_ptsout[1];

	return (lcl_contrl[4] << 1) | lcl_contrl[2];
}