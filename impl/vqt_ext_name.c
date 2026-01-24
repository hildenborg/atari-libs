#include "vdi_def.h"

INT16_T vqt_ext_name(INT16_T handle, INT16_T index, INT8_T* name, INT16_T* font_format, INT16_T* flags)
{
	INT16_T lcl_intin[2];
	lcl_intin[0] = index;
	lcl_intin[1] = 0;
	INT16_T lcl_intout[35];
	lcl_intout[0] = 0;
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 130;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 2;
	lcl_contrl[5] = 1;
	lcl_contrl[6] = handle;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		lcl_intout,
		unused_dummy_array
	};
	vdi_call(&lcl_vdipb);

	VDI_CAST_TO_BYTES(&lcl_intout[1], name, 31);
	if (lcl_contrl[4] <= 34)
	{
		*flags = 0;
		*font_format = 0;
		name[32] = (lcl_contrl[4] == 33) ? 0 : (INT8_T)lcl_intout[33];
	}
	else
	{
		name[32] = (INT8_T)lcl_intout[33];
		*flags = ((UINT16_T)lcl_intout[34]) >> 8;
		*font_format = (UINT16_T)lcl_intout[34] & 0xff;
	}
	return lcl_intout[0];
}
