/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Forgets the callbacks of the list box (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_lbox_delete).
*/

#include "aes_def.h"

static short raw_mt_lbox_delete(LIST_BOX* box, short* aes_global)
{
	short control[5] = {173, 0, 1, 1, 0};
	short intout[1];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		aes_unused_dummy_int,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	addrin[0] = (void*)box;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_lbox_delete(LIST_BOX* box, short* aes_global)
{
	short r = raw_mt_lbox_delete(box, aes_global);
	_aes_lbox_unregister(box);
	return r;
}

short lbox_delete(LIST_BOX* box)
{
	return mt_lbox_delete(box, 0);
}

