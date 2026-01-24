#include "vdi_def.h"

INT16_T vs_document_info(INT16_T handle, INT16_T type, const INT8_T *string, INT16_T wchar)
{
	INT16_T _str_len;
	if (wchar != 0)
	{
		_str_len = vdi_wstrlen(string);
	}
	else
	{
		_str_len = vdi_strlen(string);
	}
	INT16_T lcl_intin[1 + _str_len];
	lcl_intin[0] = type;
	if (wchar != 0)
	{
		_str_len = vdi_wstrlen(string);
		VDI_COPY_WORDS(string, &lcl_intin[1], _str_len);
	}
	else
	{
		VDI_CAST_FROM_BYTES(string, &lcl_intin[1], _str_len);
	}
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 5;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = _str_len;
	lcl_contrl[5] = 2103;
	lcl_contrl[6] = handle;
	INT16_T lcl_intout[2];
	lcl_intout[0] = 0;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		(INT16_T*)string,
		unused_dummy_array,
		unused_dummy_array,
		lcl_intout
	};
	vdi_call(&lcl_vdipb);

	return lcl_intout[0];
}
