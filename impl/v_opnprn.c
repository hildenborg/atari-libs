#include "vdi_def.h"

INT16_T v_opnprn(INT16_T handle, PRN_SETTINGS* settings, INT16_T* work_out )
{
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 1;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 16;
	lcl_contrl[5] = 0;
	lcl_contrl[6] = handle;
	INT16_T lcl_intin[16];
	lcl_intin[0] = settings->driver_id;
	for (INT16_T i = 1; i < 10; ++i) {lcl_intin[i] = 1;}
	lcl_intin[10] = 2;
	lcl_intin[11] = (INT16_T)settings->size_id;
	VDI_COPY_LONG(&settings->device, &lcl_contrl[12]);
	VDI_COPY_LONG(&settings, &lcl_contrl[14]);

	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		(short*)work_out,
		&((short*)work_out)[45]
	};

	vdi_call(&lcl_vdipb);
	return lcl_contrl[6];
}
