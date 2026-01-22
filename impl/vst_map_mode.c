#include "vdi_def.h"

short vst_map_mode(short handle, short mode)
{
	short lcl_intin[2];
	lcl_intin[0] = mode;
	lcl_intin[1] = 1;
	short lcl_contrl[16];
	lcl_contrl[0] = 236;
	lcl_contrl[1] = 0;
	lcl_contrl[3] = 2;
	lcl_contrl[5] = 0;
	lcl_contrl[6] = handle;
	short lcl_intout[2];
	VDIPB lcl_vdipb =
	{
		lcl_contrl,
		lcl_intin,
		unused_dummy_array,
		lcl_intout,
		unused_dummy_array
	};
	vdi_call(&lcl_vdipb);

	return (lcl_contrl[4] != 0) ? lcl_intout[0] : (mode == 1 ? 1 : 0);
}
