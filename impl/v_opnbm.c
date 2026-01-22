#include "vdi_def.h"

void v_opnbm(short* work_in, MFDB* bitmap, short* handle, short* work_out)
{
	short lcl_contrl[16];
	lcl_contrl[0] = 100;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 20;
	lcl_contrl[5] = 1;
	lcl_contrl[6] = *handle;
	VDI_COPY_LONG(&bitmap, &lcl_contrl[7]);
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		(short*)work_in,
		unused_dummy_array,
		(short*)work_out,
		&((short*)work_out)[45]
	};
	vdi_call(&lcl_vdipb);
	*handle = lcl_contrl[6];

}
