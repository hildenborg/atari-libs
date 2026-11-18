/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives the user fonts their display functions back (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_fnts_delete).
*/

#include "aes_def.h"

static short raw_mt_fnts_delete(FNT_DIALOG* fnt_dialog, short vdi_handle, short* aes_global)
{
	short control[5] = {181, 1, 1, 1, 0};
	short intin[1];
	short intout[1];
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
	intin[0] = vdi_handle;
	addrin[0] = (void*)fnt_dialog;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_fnts_delete(FNT_DIALOG* fnt_dialog, short vdi_handle, short* aes_global)
{
	short r = raw_mt_fnts_delete(fnt_dialog, vdi_handle, aes_global);
	_aes_fnts_unregister(fnt_dialog);
	return r;
}

short fnts_delete(FNT_DIALOG* fnt_dialog, short vdi_handle)
{
	return mt_fnts_delete(fnt_dialog, vdi_handle, 0);
}

