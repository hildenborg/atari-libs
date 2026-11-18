/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

/*
	vq_devinfo and vqt_devinfo (opcode 248).
	ptsout[0] is the open flag, the device name follows as a C string packed in ptsout[1..]
	(contrl[2] points), the file name comes back one character per word in intout (contrl[4] words).
*/
static void devinfo(INT16_T handle, INT16_T devnum, INT16_T* devexists, INT8_T* filename, INT8_T* devstr)
{
	INT16_T lcl_intin[1];
	INT16_T lcl_intout[128];
	INT16_T lcl_ptsout[128];
	INT16_T lcl_contrl[16];
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		lcl_intout,
		lcl_ptsout
	};
	lcl_intin[0] = devnum;
	lcl_ptsout[0] = 0;
	lcl_contrl[0] = 248;
	lcl_contrl[1] = 0;
	lcl_contrl[2] = 0;
	lcl_contrl[3] = 1;
	lcl_contrl[4] = 0;
	lcl_contrl[5] = 0;
	lcl_contrl[6] = handle;
	vdi_call(&lcl_vdipb);

	*devexists = lcl_contrl[2] > 0 ? lcl_ptsout[0] : 0;
	if (filename != 0)
	{
		INT16_T len = lcl_contrl[4];
		if (len < 0) len = 0;
		if (len > 127) len = 127;
		VDI_CAST_TO_BYTES(lcl_intout, filename, len);
	}
	if (devstr != 0)
	{
		const INT8_T* src = (const INT8_T*)&lcl_ptsout[1];
		INT16_T max = (INT16_T)(lcl_contrl[2] * 4 - 2);
		if (max > 2 * 127) max = 2 * 127;
		INT16_T i = 0;
		while (i < max && src[i] != 0)
		{
			devstr[i] = src[i];
			i++;
		}
		devstr[i] = 0;
	}
}

void vq_devinfo(INT16_T handle, INT16_T devnum, INT16_T* devexists, INT8_T* filename, INT8_T* devstr)
{
	devinfo(handle, devnum, devexists, filename, devstr);
}

#ifdef TARGET_M68K_ATARI_MINTELF
void vqt_devinfo(INT16_T handle, INT16_T devnum, INT16_T* devexists, INT8_T* filename, INT8_T* devstr)
{
	devinfo(handle, devnum, devexists, filename, devstr);
}
#else
void vqt_devinfo(INT16_T handle, INT16_T devnum, INT16_T* devexists, INT8_T* devstr)
{
	devinfo(handle, devnum, devexists, 0, devstr);
}
#endif
