/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Replaces the display functions of the user fonts with a trampoline (Pure C conventions, see aes_tramp.c).
	fnts_remove and fnts_delete put them back.
	The AES call itself is the generated code (raw_mt_fnts_add).
*/

#include "aes_def.h"

static short raw_mt_fnts_add(FNT_DIALOG* fnt_dialog, FNTS_ITEM* user_fonts, short* aes_global)
{
	short control[5] = {185, 1, 1, 2, 0};
	short intin[1];
	short intout[1];
	void* addrin[2];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = 0;
	addrin[0] = (void*)fnt_dialog;
	addrin[1] = (void*)user_fonts;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_fnts_add(FNT_DIALOG* fnt_dialog, FNTS_ITEM* user_fonts, short* aes_global)
{
	if (!_aes_fnts_register(fnt_dialog, user_fonts))
	{
		_aes_fnts_unregister(fnt_dialog);
		return 0;
	}
	return raw_mt_fnts_add(fnt_dialog, user_fonts, aes_global);
}

short fnts_add(FNT_DIALOG* fnt_dialog, FNTS_ITEM* user_fonts)
{
	return mt_fnts_add(fnt_dialog, user_fonts, 0);
}

