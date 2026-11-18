/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Trampolines for callbacks that WDIALOG/MagiC call with Pure C conventions.

	Pure C passes short parameters as 2 bytes on the stack, gcc expects every parameter in a
	4 byte slot. The library gives WDIALOG a trampoline instead of the user's function. The
	trampoline rebuilds the arguments in gcc layout and calls a dispatcher, which looks up the
	user's function and calls it.

	The user's function is found by:
	- HNDL_OBJ (wdlg_create): the DIALOG pointer. Calls during wdlg_create use the pending handler.
	- SET_ITEM, SLCT_ITEM (lbox_create): the LIST_BOX pointer, the pending pair during lbox_create.
	- XFRM_POPUP_CB (xfrm_popup): only used during the call.
	- PDLG_HNDL (PDLG_SUB.do_dlg, pdlg_add_sub_dialogs): the PDLG_SUB pointer.
	- UTXT_FN (FNTS_ITEM.display, fnts_add): the font id.
	The tables are small and fixed, TOS programs only have a few dialogs open.
*/

#include "aes_def.h"

#define TRAMP_SLOTS 32

/*
	Pure C stack layout on entry (offset from a7, 0 is the return address) -> gcc call.
	Words are sign extended to longs. a0 holds the entry a7, d0/d1/a0/a1 are scratch in both conventions.
*/
__asm__(
	// HNDL_OBJ: dialog 4, events 8, obj.w 12, clicks.w 14, data 16
	".globl _aes_tramp_hndl_obj\n"
	"_aes_tramp_hndl_obj:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.l	%a0@(16), %a7@-\n\t"
	"move.w	%a0@(14), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.w	%a0@(12), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(8), %a7@-\n\t"
	"move.l	%a0@(4), %a7@-\n\t"
	"jsr	_aes_dispatch_hndl_obj\n\t"
	"lea	%a7@(20), %a7\n\t"
	"rts\n"

	// SET_ITEM: box 4, tree 8, item 12, obj_index.w 16, user_data 18, rect 22, first.w 26
	".globl _aes_tramp_set_item\n"
	"_aes_tramp_set_item:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.w	%a0@(26), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(22), %a7@-\n\t"
	"move.l	%a0@(18), %a7@-\n\t"
	"move.w	%a0@(16), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(12), %a7@-\n\t"
	"move.l	%a0@(8), %a7@-\n\t"
	"move.l	%a0@(4), %a7@-\n\t"
	"jsr	_aes_dispatch_set_item\n\t"
	"lea	%a7@(28), %a7\n\t"
	"rts\n"

	// SLCT_ITEM: box 4, tree 8, item 12, user_data 16, obj_index.w 20, last_state.w 22
	".globl _aes_tramp_slct_item\n"
	"_aes_tramp_slct_item:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.w	%a0@(22), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.w	%a0@(20), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(16), %a7@-\n\t"
	"move.l	%a0@(12), %a7@-\n\t"
	"move.l	%a0@(8), %a7@-\n\t"
	"move.l	%a0@(4), %a7@-\n\t"
	"jsr	_aes_dispatch_slct_item\n\t"
	"lea	%a7@(24), %a7\n\t"
	"rts\n"

	// XFRM_POPUP_CB: tree 4, scrollpos.w 8, nlines.w 10, param 12
	".globl _aes_tramp_xfrm_popup\n"
	"_aes_tramp_xfrm_popup:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.l	%a0@(12), %a7@-\n\t"
	"move.w	%a0@(10), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.w	%a0@(8), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(4), %a7@-\n\t"
	"jsr	_aes_dispatch_xfrm_popup\n\t"
	"lea	%a7@(16), %a7\n\t"
	"rts\n"

	// PDLG_HNDL: settings 4, sub 8, exit_obj.w 12
	".globl _aes_tramp_pdlg_hndl\n"
	"_aes_tramp_pdlg_hndl:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.w	%a0@(12), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.l	%a0@(8), %a7@-\n\t"
	"move.l	%a0@(4), %a7@-\n\t"
	"jsr	_aes_dispatch_pdlg_hndl\n\t"
	"lea	%a7@(12), %a7\n\t"
	"rts\n"

	// UTXT_FN: x.w 4, y.w 6, clip_rect 8, id 12, pt 16, ratio 20, string 24
	".globl _aes_tramp_utxt\n"
	"_aes_tramp_utxt:\n\t"
	"move.l	%a7, %a0\n\t"
	"move.l	%a0@(24), %a7@-\n\t"
	"move.l	%a0@(20), %a7@-\n\t"
	"move.l	%a0@(16), %a7@-\n\t"
	"move.l	%a0@(12), %a7@-\n\t"
	"move.l	%a0@(8), %a7@-\n\t"
	"move.w	%a0@(6), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"move.w	%a0@(4), %d0\n\t"
	"ext.l	%d0\n\t"
	"move.l	%d0, %a7@-\n\t"
	"jsr	_aes_dispatch_utxt\n\t"
	"lea	%a7@(28), %a7\n\t"
	"rts\n"
);

/*
	Window dialogs.
*/
static struct { DIALOG* dialog; HNDL_OBJ fn; } hndl_tab[TRAMP_SLOTS];
static HNDL_OBJ hndl_pending;

INT16_T _aes_dispatch_hndl_obj(DIALOG* dialog, EVNT* events, INT16_T obj, INT16_T clicks, void* data)
{
	HNDL_OBJ fn = hndl_pending;
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (hndl_tab[i].fn && hndl_tab[i].dialog == dialog)
		{
			fn = hndl_tab[i].fn;
			break;
		}
	}
	return fn ? fn(dialog, events, obj, clicks, data) : 1;
}

void _aes_hndl_pending(HNDL_OBJ fn)
{
	hndl_pending = fn;
}

INT16_T _aes_hndl_register(DIALOG* dialog, HNDL_OBJ fn)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (hndl_tab[i].fn == 0)
		{
			hndl_tab[i].dialog = dialog;
			hndl_tab[i].fn = fn;
			return 1;
		}
	}
	return 0;
}

void _aes_hndl_unregister(DIALOG* dialog)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (hndl_tab[i].dialog == dialog)
		{
			hndl_tab[i].fn = 0;
			hndl_tab[i].dialog = 0;
		}
	}
}

/*
	List boxes.
*/
static struct { LIST_BOX* box; SET_ITEM set; SLCT_ITEM slct; } lbox_tab[TRAMP_SLOTS];
static SET_ITEM set_pending;
static SLCT_ITEM slct_pending;

static int lbox_find(LIST_BOX* box)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if ((lbox_tab[i].set || lbox_tab[i].slct) && lbox_tab[i].box == box)
		{
			return i;
		}
	}
	return -1;
}

INT16_T _aes_dispatch_set_item(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, INT16_T obj_index, void* user_data, GRECT* rect, INT16_T first)
{
	int i = lbox_find(box);
	SET_ITEM fn = i >= 0 ? lbox_tab[i].set : set_pending;
	return fn ? fn(box, tree, item, obj_index, user_data, rect, first) : 0;
}

void _aes_dispatch_slct_item(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, void* user_data, INT16_T obj_index, INT16_T last_state)
{
	int i = lbox_find(box);
	SLCT_ITEM fn = i >= 0 ? lbox_tab[i].slct : slct_pending;
	if (fn)
	{
		fn(box, tree, item, user_data, obj_index, last_state);
	}
}

void _aes_lbox_pending(SET_ITEM set, SLCT_ITEM slct)
{
	set_pending = set;
	slct_pending = slct;
}

INT16_T _aes_lbox_register(LIST_BOX* box, SET_ITEM set, SLCT_ITEM slct)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (lbox_tab[i].set == 0 && lbox_tab[i].slct == 0)
		{
			lbox_tab[i].box = box;
			lbox_tab[i].set = set;
			lbox_tab[i].slct = slct;
			return 1;
		}
	}
	return 0;
}

void _aes_lbox_unregister(LIST_BOX* box)
{
	int i;
	while ((i = lbox_find(box)) >= 0)
	{
		lbox_tab[i].box = 0;
		lbox_tab[i].set = 0;
		lbox_tab[i].slct = 0;
	}
}

/*
	xfrm_popup init callback, only during the call.
*/
static XFRM_POPUP_CB xfrm_current;

void _aes_dispatch_xfrm_popup(OBJECT* tree, INT16_T scrollpos, INT16_T nlines, void* param)
{
	if (xfrm_current)
	{
		xfrm_current(tree, scrollpos, nlines, param);
	}
}

void _aes_xfrm_popup_set(XFRM_POPUP_CB fn)
{
	xfrm_current = fn;
}

/*
	Printer sub dialogs: the do_dlg handler of each PDLG_SUB.
*/
static struct { PDLG_SUB* sub; PDLG_HNDL fn; PRN_DIALOG* dialog; } pdlg_tab[TRAMP_SLOTS];

INT32_T _aes_dispatch_pdlg_hndl(PRN_SETTINGS* settings, PDLG_SUB* sub, INT16_T exit_obj)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (pdlg_tab[i].fn && pdlg_tab[i].sub == sub)
		{
			return pdlg_tab[i].fn(settings, sub, exit_obj);
		}
	}
	return 0;
}

// Replace the do_dlg handlers in the list with the trampoline. Returns 0 if the table is full.
INT16_T _aes_pdlg_subs_register(PRN_DIALOG* dialog, PDLG_SUB* subs)
{
	for (PDLG_SUB* s = subs; s; s = s->next)
	{
		if (s->do_dlg == 0 || s->do_dlg == (PDLG_HNDL)_aes_tramp_pdlg_hndl)
		{
			continue;
		}
		int i;
		for (i = 0; i < TRAMP_SLOTS && pdlg_tab[i].fn; i++)
		{
		}
		if (i == TRAMP_SLOTS)
		{
			return 0;
		}
		pdlg_tab[i].sub = s;
		pdlg_tab[i].fn = s->do_dlg;
		pdlg_tab[i].dialog = dialog;
		s->do_dlg = (PDLG_HNDL)_aes_tramp_pdlg_hndl;
	}
	return 1;
}

// Give the sub dialogs of this printer dialog their own handlers back.
void _aes_pdlg_subs_unregister(PRN_DIALOG* dialog)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (pdlg_tab[i].fn && pdlg_tab[i].dialog == dialog)
		{
			pdlg_tab[i].sub->do_dlg = pdlg_tab[i].fn;
			pdlg_tab[i].fn = 0;
			pdlg_tab[i].sub = 0;
			pdlg_tab[i].dialog = 0;
		}
	}
}

/*
	User font display functions (FNTS_ITEM.display), found by font id.
*/
static struct { INT32_T id; UTXT_FN fn; FNTS_ITEM* item; FNT_DIALOG* dialog; } utxt_tab[TRAMP_SLOTS];

void _aes_dispatch_utxt(INT16_T x, INT16_T y, INT16_T* clip_rect, INT32_T id, INT32_T pt, INT32_T ratio, char* string)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (utxt_tab[i].fn && utxt_tab[i].id == id)
		{
			utxt_tab[i].fn(x, y, clip_rect, id, pt, ratio, string);
			return;
		}
	}
}

// Replace the display functions in the list with the trampoline. Returns 0 if the table is full.
INT16_T _aes_fnts_register(FNT_DIALOG* dialog, FNTS_ITEM* items)
{
	for (FNTS_ITEM* f = items; f; f = f->next)
	{
		if (f->display == 0 || f->display == (UTXT_FN)_aes_tramp_utxt)
		{
			continue;
		}
		int i;
		for (i = 0; i < TRAMP_SLOTS && utxt_tab[i].fn; i++)
		{
		}
		if (i == TRAMP_SLOTS)
		{
			return 0;
		}
		utxt_tab[i].id = f->id;
		utxt_tab[i].fn = f->display;
		utxt_tab[i].item = f;
		utxt_tab[i].dialog = dialog;
		f->display = (UTXT_FN)_aes_tramp_utxt;
	}
	return 1;
}

// Give the fonts added to this dialog their own display functions back.
void _aes_fnts_unregister(FNT_DIALOG* dialog)
{
	for (int i = 0; i < TRAMP_SLOTS; i++)
	{
		if (utxt_tab[i].fn && utxt_tab[i].dialog == dialog)
		{
			utxt_tab[i].item->display = utxt_tab[i].fn;
			utxt_tab[i].fn = 0;
			utxt_tab[i].item = 0;
			utxt_tab[i].dialog = 0;
		}
	}
}
