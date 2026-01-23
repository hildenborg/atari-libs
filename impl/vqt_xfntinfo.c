#include "vdi_def.h"

INT16_T vqt_xfntinfo(INT16_T handle, INT16_T flags, INT16_T id, INT16_T index, XFNT_INFO* info)
{
	INT16_T lcl_intin[5];
	lcl_intin[0] = flags;
	lcl_intin[1] = id;
	lcl_intin[2] = index;
	VDI_COPY_LONG(&info, &lcl_intin[3]);
	INT16_T lcl_intout[3];
	lcl_intout[1] = 0;
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 229;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 5;
	lcl_contrl[5] = 0;
	lcl_contrl[6] = handle;
	info->size = sizeof(XFNT_INFO);
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		lcl_intout,
		unused_dummy_array
	};
	vdi_call(&lcl_vdipb);

	info->format = lcl_intout[0];
	info->id = lcl_intout[1];
	info->index = lcl_intout[2];
	return lcl_intout[1];
}
