#include "vdi_def.h"

void v_bez_fill(INT16_T handle, INT16_T count, INT16_T* xyarray, INT8_T* bezarray, INT16_T* extent, INT16_T* totpts, INT16_T* totmoves)
{
	INT16_T lcl_intin_num = (count + 1) >> 1;
	INT16_T lcl_intin[lcl_intin_num];
	VDI_SWAP_BYTES(bezarray, &lcl_intin[0], count);
	INT16_T lcl_intout[2];
	INT16_T lcl_contrl[16];
	lcl_contrl[0] = 9;
	lcl_contrl[1] = lcl_intin_num;
	lcl_contrl[3] = count;
	lcl_contrl[5] = 13;
	lcl_contrl[6] = handle;
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		(INT16_T*)xyarray,
		lcl_intout,
		(INT16_T*)extent
	};
	vdi_call(&lcl_vdipb);

	*totpts = lcl_intout[0];
	*totmoves = lcl_intout[1];
}
