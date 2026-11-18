#include "vdi_def.h"

// intin = type, then the string, one character (or 16 bit character when wchar is set) per word.
INT16_T vs_document_info(INT16_T handle, INT16_T type, const INT8_T *string, INT16_T wchar)
{
	INT16_T _str_len = (wchar != 0) ? vdi_wstrlen(string) : vdi_strlen(string);
	INT16_T lcl_intin[1 + _str_len];
	lcl_intin[0] = type;
	if (wchar != 0)
	{
		VDI_COPY_WORDS(string, &lcl_intin[1], _str_len);
	}
	else
	{
		VDI_CAST_FROM_BYTES(string, &lcl_intin[1], _str_len);
	}
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 5;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 1 + _str_len;
	lcl_contrl[5] = 2103;
	lcl_contrl[6] = handle;
	INT16_T lcl_intout[2];
	lcl_intout[0] = 0;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		lcl_intout,
		unused_dummy_array
	};
	vdi_call(&lcl_vdipb);

	return lcl_intout[0];
}
