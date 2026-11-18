/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Replaces the do_dlg handlers of the sub dialogs with a trampoline (Pure C conventions, see aes_tramp.c).
	pdlg_remove_sub_dialogs and pdlg_delete put them back.
	The AES call itself is the generated code (raw_mt_pdlg_add_sub_dialogs).
*/

#include "aes_def.h"

static short raw_mt_pdlg_add_sub_dialogs(PRN_DIALOG* prn_dialog, PDLG_SUB* sub_dialogs, short* aes_global)
{
	short control[5] = {205, 1, 1, 2, 0};
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
	intin[0] = 3;
	addrin[0] = (void*)prn_dialog;
	addrin[1] = (void*)sub_dialogs;
	intout[0] = 0;
	aes_call(&lcl_aespb);
	return intout[0];
}

short mt_pdlg_add_sub_dialogs(PRN_DIALOG* prn_dialog, PDLG_SUB* sub_dialogs, short* aes_global)
{
	if (!_aes_pdlg_subs_register(prn_dialog, sub_dialogs))
	{
		_aes_pdlg_subs_unregister(prn_dialog);
		return 0;
	}
	return raw_mt_pdlg_add_sub_dialogs(prn_dialog, sub_dialogs, aes_global);
}

short pdlg_add_sub_dialogs(PRN_DIALOG* prn_dialog, PDLG_SUB* sub_dialogs)
{
	return mt_pdlg_add_sub_dialogs(prn_dialog, sub_dialogs, 0);
}

