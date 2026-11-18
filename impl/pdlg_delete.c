/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives the sub dialogs their do_dlg handlers back (see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_pdlg_delete).
*/

#include "aes_def.h"

static short raw_mt_pdlg_delete(PRN_DIALOG* prn_dialog, short* aes_global)
{
	short control[5] = {201, 0, 1, 1, 0};
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
	addrin[0] = (void*)prn_dialog;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_pdlg_delete(PRN_DIALOG* prn_dialog, short* aes_global)
{
	short r = raw_mt_pdlg_delete(prn_dialog, aes_global);
	_aes_pdlg_subs_unregister(prn_dialog);
	return r;
}

short pdlg_delete(PRN_DIALOG* prn_dialog)
{
	return mt_pdlg_delete(prn_dialog, 0);
}

