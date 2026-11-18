/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives the user fonts their display functions back (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_fnts_remove).
*/

#include "aes_def.h"

static void raw_mt_fnts_remove(FNT_DIALOG* fnt_dialog, short* aes_global)
{
	short control[5] = {185, 1, 1, 1, 0};
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
	intin[0] = 1;
	addrin[0] = (void*)fnt_dialog;
	aes_call(&lcl_aespb);
}

void mt_fnts_remove(FNT_DIALOG* fnt_dialog, short* aes_global)
{
	raw_mt_fnts_remove(fnt_dialog, aes_global);
	_aes_fnts_unregister(fnt_dialog);
}

void fnts_remove(FNT_DIALOG* fnt_dialog)
{
	mt_fnts_remove(fnt_dialog, 0);
}

