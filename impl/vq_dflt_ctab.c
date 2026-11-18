/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

// The table is written to intout. NVDI leaves contrl[4] at 0 here, so success is also seen in the table's magic.
INT16_T vq_dflt_ctab(INT16_T handle, INT32_T ctab_length, COLOR_TAB* ctab)
{
	INT16_T lcl_intin[2];
	VDI_COPY_LONG(&ctab_length, &lcl_intin[0]);
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 206;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 2;
	lcl_contrl[4] = 0;
	lcl_contrl[5] = 7;
	lcl_contrl[6] = handle;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		(INT16_T*)ctab,
		unused_dummy_array
	};
	ctab->magic = 0;
	vdi_call(&lcl_vdipb);

	return (lcl_contrl[4] != 0 || ctab->magic == 0x63746162L) ? 1 : 0;	// COLOR_TAB_MAGIC ('ctab')
}
