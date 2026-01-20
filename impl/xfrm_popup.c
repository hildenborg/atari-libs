#include "aes_def.h"

INT16_T mt_xfrm_popup(OBJECT* tree, INT16_T x, INT16_T y, INT16_T firstscrlob, INT16_T lastscrlob, INT16_T nlines, XFRM_POPUP_CB init, void* param, INT16_T* lastscrlpos, INT16_T* aes_global)
{
	INT16_T control[5] = {135, 6, 2, 2, 0};
	INT16_T intin[6];
	INT16_T intout[2];
	void* addrin[3];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = x;
	intin[1] = y;
	intin[2] = firstscrlob;
	intin[3] = lastscrlob;
	intin[4] = nlines;
	intin[5] = *lastscrlpos;
	addrin[0] = (void*)tree;
	addrin[1] = (void*)init;
	addrin[2] = (void*)param;
	intout[1] = *lastscrlpos;
	aes_call(&lcl_aespb);
	*lastscrlpos = intout[1];
	return intout[0];
}

INT16_T xfrm_popup(OBJECT* tree, INT16_T x, INT16_T y, INT16_T firstscrlob, INT16_T lastscrlob, INT16_T nlines, XFRM_POPUP_CB init, void* param, INT16_T* lastscrlpos)
{
	return mt_xfrm_popup(tree, x, y, firstscrlob, lastscrlob, nlines, init, param, lastscrlpos, 0);
}

