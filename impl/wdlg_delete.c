/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Forgets the handle_exit of the dialog (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_wdlg_delete).
*/

#include "aes_def.h"

static short raw_mt_wdlg_delete(DIALOG* dialog, short* aes_global)
{
	short control[5] = {163, 0, 1, 1, 0};
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
	addrin[0] = (void*)dialog;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_wdlg_delete(DIALOG* dialog, short* aes_global)
{
	short r = raw_mt_wdlg_delete(dialog, aes_global);
	_aes_hndl_unregister(dialog);
	return r;
}

short wdlg_delete(DIALOG* dialog)
{
	return mt_wdlg_delete(dialog, 0);
}

