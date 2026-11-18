/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives WDIALOG a trampoline for handle_exit (Pure C conventions, see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_wdlg_create).
*/

#include "aes_def.h"

static DIALOG* raw_mt_wdlg_create(HNDL_OBJ handle_exit, OBJECT* tree, void* user_data, short code, void* data, short flags, short* aes_global)
{
	short control[5] = {160, 2, 0, 4, 1};
	short intin[2];
	void* addrin[4];
	void* addrout[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		aes_unused_dummy_int,
		addrin,
		addrout
	};
	intin[0] = code;
	intin[1] = flags;
	addrin[0] = (void*)handle_exit;
	addrin[1] = (void*)tree;
	addrin[2] = (void*)user_data;
	addrin[3] = (void*)data;
	addrout[0] = 0;
	aes_call(&lcl_aespb);
	return addrout[0];
}

DIALOG* mt_wdlg_create(HNDL_OBJ handle_exit, OBJECT* tree, void* user_data, short code, void* data, short flags, short* aes_global)
{
	// WDIALOG calls handle_exit during wdlg_create, before the DIALOG pointer is known.
	_aes_hndl_pending(handle_exit);
	DIALOG* d = raw_mt_wdlg_create((HNDL_OBJ)_aes_tramp_hndl_obj, tree, user_data, code, data, flags, aes_global);
	_aes_hndl_pending(0);
	if (d && !_aes_hndl_register(d, handle_exit))
	{
		mt_wdlg_delete(d, aes_global);
		return 0;
	}
	return d;
}

DIALOG* wdlg_create(HNDL_OBJ handle_exit, OBJECT* tree, void* user_data, short code, void* data, short flags)
{
	return mt_wdlg_create(handle_exit, tree, user_data, code, data, flags, 0);
}

