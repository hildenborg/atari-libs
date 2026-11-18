/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Gives WDIALOG trampolines for the slct and set callbacks (Pure C conventions, see aes_tramp.c).
	The AES call itself is the generated code (raw_mt_lbox_create).
*/

#include "aes_def.h"

static LIST_BOX* raw_mt_lbox_create(OBJECT* tree, SLCT_ITEM slct, SET_ITEM set, LBOX_ITEM* items, short visible_a, short first_a, const short* ctrl_objs, const short* objs, short flags, short pause_a, void* user_data, DIALOG* dialog, short visible_b, short first_b, short entries_b, short pause_b, short* aes_global)
{
	short control[5] = {170, 8, 0, 8, 1};
	short intin[8];
	void* addrin[8];
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
	intin[0] = visible_a;
	intin[1] = first_a;
	intin[2] = flags;
	intin[3] = pause_a;
	intin[4] = visible_b;
	intin[5] = first_b;
	intin[6] = entries_b;
	intin[7] = pause_b;
	addrin[0] = (void*)tree;
	addrin[1] = (void*)slct;
	addrin[2] = (void*)set;
	addrin[3] = (void*)items;
	addrin[4] = (void*)ctrl_objs;
	addrin[5] = (void*)objs;
	addrin[6] = (void*)user_data;
	addrin[7] = (void*)dialog;
	addrout[0] = 0;
	aes_call(&lcl_aespb);
	return addrout[0];
}

LIST_BOX* mt_lbox_create(OBJECT* tree, SLCT_ITEM slct, SET_ITEM set, LBOX_ITEM* items, short visible_a, short first_a, const short* ctrl_objs, const short* objs, short flags, short pause_a, void* user_data, DIALOG* dialog, short visible_b, short first_b, short entries_b, short pause_b, short* aes_global)
{
	// WDIALOG calls set during lbox_create, before the LIST_BOX pointer is known.
	_aes_lbox_pending(set, slct);
	LIST_BOX* box = raw_mt_lbox_create(tree, slct ? (SLCT_ITEM)_aes_tramp_slct_item : 0, set ? (SET_ITEM)_aes_tramp_set_item : 0, items,
		visible_a, first_a, ctrl_objs, objs, flags, pause_a, user_data, dialog, visible_b, first_b, entries_b, pause_b, aes_global);
	_aes_lbox_pending(0, 0);
	if (box && !_aes_lbox_register(box, set, slct))
	{
		mt_lbox_delete(box, aes_global);
		return 0;
	}
	return box;
}

LIST_BOX* lbox_create(OBJECT* tree, SLCT_ITEM slct, SET_ITEM set, LBOX_ITEM* items, short visible_a, short first_a, const short* ctrl_objs, const short* objs, short flags, short pause_a, void* user_data, DIALOG* dialog, short visible_b, short first_b, short entries_b, short pause_b)
{
	return mt_lbox_create(tree, slct, set, items, visible_a, first_a, ctrl_objs, objs, flags, pause_a, user_data, dialog, visible_b, first_b, entries_b, pause_b, 0);
}

