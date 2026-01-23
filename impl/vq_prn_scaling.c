#include "vdi_def.h"

fix31 vq_prn_scaling(short handle)
{
	short lcl_intin[2];
	lcl_intin[0] = -1;
	lcl_intin[1] = -1;
	short lcl_intout[2];
	lcl_intout[0] = 0;
	lcl_intout[1] = 0;
	short lcl_contrl[16];
	lcl_contrl[0] = 5;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 2;
	lcl_contrl[4] = 0;
	lcl_contrl[5] = 39;
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
	if (lcl_contrl[4] == 2)
	{
		return *(fix31*)(&lcl_intout[0]);
	}
	return((fix31)-1);
#pragma GCC diagnostic pop
}
