#include "aes_def.h"

INT16_T mt_form_wkeybd(OBJECT* tree, INT16_T object, INT16_T nextob, INT16_T ichar, INT16_T* onextob, INT16_T* ochar, INT16_T windowhandle, INT16_T* aes_global)
{
	INT16_T control[5] = {64, 4, 3, 1, 0};
	INT16_T intin[4];
	INT16_T intout[3];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = object;
	intin[1] = ichar;
	intin[2] = nextob;
	intin[3] = windowhandle;
	addrin[0] = (void*)tree;
	aes_call(&lcl_aespb);
	*onextob = intout[1];
	*ochar = intout[2];
	return intout[0];
}

INT16_T form_wkeybd(OBJECT* tree, INT16_T object, INT16_T nextob, INT16_T ichar, INT16_T* onextob, INT16_T* ochar, INT16_T windowhandle)
{
	return mt_form_wkeybd(tree, object, nextob, ichar, onextob, ochar, windowhandle, 0);
}

