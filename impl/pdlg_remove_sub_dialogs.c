/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives the sub dialogs their do_dlg handlers back (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_pdlg_remove_sub_dialogs).
*/

#include "aes_def.h"

static short raw_mt_pdlg_remove_sub_dialogs(PRN_DIALOG* prn_dialog, short* aes_global)
{
	short control[5] = {205, 1, 1, 1, 0};
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
	intin[0] = 4;
	addrin[0] = (void*)prn_dialog;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_pdlg_remove_sub_dialogs(PRN_DIALOG* prn_dialog, short* aes_global)
{
	short r = raw_mt_pdlg_remove_sub_dialogs(prn_dialog, aes_global);
	_aes_pdlg_subs_unregister(prn_dialog);
	return r;
}

short pdlg_remove_sub_dialogs(PRN_DIALOG* prn_dialog)
{
	return mt_pdlg_remove_sub_dialogs(prn_dialog, 0);
}

