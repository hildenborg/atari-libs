#include "aes_def.h"

INT16_T mt_appl_init(INT16_T* aes_global)
{
	INT16_T control[5] = {10, 0, 1, 0, 0};
	INT16_T intout[1] = {-1};
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		aes_unused_dummy_int,
		intout,
		aes_unused_dummy_addr,
		aes_unused_dummy_addr
	};
	aes_global[0] = 0;
	aes_global[2] = -1;
	INT16_T result = (INT16_T)aes_call(&lcl_aespb);
	return result;
}

INT16_T appl_init(void)
{
	return mt_appl_init(_aes_global);
}

