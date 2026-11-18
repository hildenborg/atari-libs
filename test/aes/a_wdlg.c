/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	WDIALOG extensions (NVDI): window dialogs, list boxes, font and printer dialogs.
	Availability comes from appl_getinfo(7). fslx_ (file selector) and edit_ (editor) need MagiC.

	WDIALOG calls back into the program (handle_exit, list box callbacks) with Pure C conventions:
	short parameters are 2 bytes on the stack. The library puts trampolines in between (impl/aes_tramp.c).
	The test callbacks are plain C functions, as an application would write them. The trampolines are
	also called directly with Pure C style arguments (test_trampolines), for callbacks WDIALOG can't be
	made to call here.
*/

#include <string.h>
#include <vdi.h>
#include "a_tree.h"

static short gi7;	// appl_getinfo(7) bits: 1 wdlg, 2 lbox, 4 fnts, 8 fslx, 16 pdlg

/*
	Window dialog.
*/
static int h_calls;
static short h_obj, h_clicks;
static void* h_data;
static int h_closed;

static short hndl_c(DIALOG* dialog, EVNT* events, short obj, short clicks, void* data)
{
	if (h_calls++ == 0)
	{
		h_obj = obj;
		h_clicks = clicks;
		h_data = data;
	}
	if (obj == HNDL_CLSD)
	{
		h_closed = 1;
		return 0;
	}
	return 1;
}

static void test_wdlg(void)
{
	static const char* funcs[] =
	{
		"wdlg_create", "wdlg_open", "wdlg_close", "wdlg_delete", "wdlg_get_tree", "wdlg_get_edit", "wdlg_get_udata",
		"wdlg_get_handle", "wdlg_set_edit", "wdlg_set_tree", "wdlg_set_size", "wdlg_set_iconify", "wdlg_set_uniconify",
		"wdlg_evnt", "wdlg_redraw", 0
	};
	if (!(gi7 & 1))
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no WDIALOG window dialogs (appl_getinfo(7) = 0x%x)", gi7);
		}
		return;
	}
	static int udata, cdata;
	build_tree();
	tree[D_ROOT].ob_state = NORMAL;

	t_run("wdlg_create");
	h_calls = 0;
	DIALOG* d = wdlg_create(hndl_c, tree, &udata, 77, &cdata, 0);
	if (!CHECK_TRUE("wdlg_create", d != 0, "wdlg_create() = %p", d))
	{
		return;
	}
	// During wdlg_create WDIALOG calls handle_exit(HNDL_INIT) with clicks = code and the data pointer.
	CHECK_TRUE("wdlg_create", h_calls >= 1 && h_obj == HNDL_INIT && h_clicks == 77 && h_data == &cdata,
		"handle_exit called %d times, first with obj %d clicks %d data %p (expected %d, 77, %p)",
		h_calls, h_obj, h_clicks, h_data, HNDL_INIT, &cdata);

	t_run("wdlg_open");
	short wh = wdlg_open(d, "WDIALOG test", NAME | MOVER | CLOSER, -1, -1, 0, 0);
	CHECK_TRUE("wdlg_open", wh > 0, "wdlg_open() = %d", wh);
	t_run("wdlg_get_handle");
	CHECK_EQ("wdlg_get_handle", wdlg_get_handle(d), wh);
	t_run("wdlg_get_udata");
	CHECK_EQ("wdlg_get_udata", (long)wdlg_get_udata(d), (long)&udata);
	t_run("wdlg_get_tree");
	OBJECT* t = 0;
	GRECT r;
	memset(&r, 0, sizeof(r));
	wdlg_get_tree(d, &t, &r);
	CHECK_TRUE("wdlg_get_tree", t == tree && r.g_w == tree[D_ROOT].ob_width, "wdlg_get_tree() = %p, %d,%d %dx%d", t, r.g_x, r.g_y, r.g_w, r.g_h);

	t_run("wdlg_set_edit");
	short e = wdlg_set_edit(d, D_EDIT);
	t_run("wdlg_get_edit");
	short cursor = -1;
	short ge = wdlg_get_edit(d, &cursor);
	CHECK_TRUE("wdlg_get_edit", ge == D_EDIT, "wdlg_set_edit() = %d, wdlg_get_edit() = %d cursor %d", e, ge, cursor);
	CHECK_TRUE("wdlg_set_edit", e == D_EDIT || e == 0, "wdlg_set_edit(edit field) = %d", e);

	t_run("wdlg_set_tree");
	CHECK_TRUE("wdlg_set_tree", wdlg_set_tree(d, tree) != 0, "wdlg_set_tree()");
	t_run("wdlg_set_size");
	CHECK_TRUE("wdlg_set_size", wdlg_set_size(d, &r) != 0, "wdlg_set_size()");
	t_run("wdlg_redraw");
	wdlg_redraw(d, &r, ROOT, MAX_DEPTH);
	t_check(1, "wdlg_redraw", "called (void)");

	t_run("wdlg_evnt");
	EVNT ev;
	memset(&ev, 0, sizeof(ev));
	CHECK_EQ("wdlg_evnt", wdlg_evnt(d, &ev), 1);	// No event: continue
	// A close message for the window: WDIALOG calls handle_exit(HNDL_CLSD), which returns 0.
	memset(&ev, 0, sizeof(ev));
	ev.mwhich = MU_MESAG;
	ev.msg[0] = WM_CLOSED;
	ev.msg[1] = a_apid;
	ev.msg[3] = wh;
	h_closed = 0;
	short er = wdlg_evnt(d, &ev);
	CHECK_TRUE("wdlg_evnt", er == 0 && h_closed, "wdlg_evnt(WM_CLOSED) = %d, handle_exit got HNDL_CLSD: %s", er, h_closed ? "yes" : "no");

	t_run("wdlg_set_iconify");
	GRECT ig = { 0, 0, 72, 72 };
	short ic = wdlg_set_iconify(d, &ig, "Icon", 0, 0);
	t_run("wdlg_set_uniconify");
	short uc = wdlg_set_uniconify(d, &r, "WDIALOG test", tree);
	t_check(ic >= 0 && uc >= 0, "wdlg_set_iconify", "wdlg_set_iconify() = %d (AES 0x%04x has no iconify)", ic, a_aesver);
	t_check(ic >= 0 && uc >= 0, "wdlg_set_uniconify", "wdlg_set_uniconify() = %d", uc);

	t_run("wdlg_close");
	short x = -1, y = -1;
	CHECK_TRUE("wdlg_close", wdlg_close(d, &x, &y) != 0 && x >= 0 && y >= 0, "wdlg_close() position %d,%d", x, y);
	t_run("wdlg_delete");
	CHECK_TRUE("wdlg_delete", wdlg_delete(d) != 0, "wdlg_delete()");
	a_drain_keys();
}

/*
	List box: 4 visible items of 6, a vertical and a horizontal slider.
*/
enum { L_ROOT, L_BOX, L_I1, L_I2, L_I3, L_I4, L_UP, L_DOWN, L_BACK, L_SLID, L_LEFT, L_RIGHT, L_HBACK, L_HSLID, L_COUNT };
static OBJECT ltree[L_COUNT];
static char lstr[4][24];
static int s_calls, s_bad, c_calls, c_bad;
static short c_objs[4], c_states[4];
static void* lbox_udata;

static short set_c(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, short obj_index, void* user_data, GRECT* rect, short first)
{
	s_calls++;
	if (tree != ltree || obj_index < L_I1 || obj_index > L_I4 || user_data != lbox_udata || first != 0)
	{
		s_bad++;
	}
	return 0;
}

static void slct_c(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, void* user_data, short obj_index, short last_state)
{
	if (c_calls < 4)
	{
		c_objs[c_calls] = obj_index;
		c_states[c_calls] = last_state;
	}
	c_calls++;
	// obj_index is 0 for an item that isn't visible (deselected after scrolling).
	if (tree != ltree || (obj_index != 0 && (obj_index < L_I1 || obj_index > L_I4)) || user_data != lbox_udata)
	{
		c_bad++;
	}
}

static void build_ltree(void)
{
	a_obj(ltree, L_ROOT, -1, L_BOX, L_HBACK, G_BOX, NONE, NORMAL, (void*)0x00021100L, 0, 0, 30, 9);
	a_obj(ltree, L_BOX, L_UP, L_I1, L_I4, G_IBOX, NONE, NORMAL, 0, 1, 1, 20, 4);
	for (int i = 0; i < 4; i++)
	{
		strcpy(lstr[i], "item");
		a_obj(ltree, (short)(L_I1 + i), (short)(i < 3 ? L_I1 + i + 1 : L_BOX), -1, -1, G_STRING, SELECTABLE | TOUCHEXIT, NORMAL, lstr[i], 0, (short)i, 20, 1);
	}
	a_obj(ltree, L_UP, L_DOWN, -1, -1, G_BOXCHAR, TOUCHEXIT, NORMAL, (void*)((1L << 24) | 0x00011100L), 21, 1, 2, 1);
	a_obj(ltree, L_DOWN, L_BACK, -1, -1, G_BOXCHAR, TOUCHEXIT, NORMAL, (void*)((2L << 24) | 0x00011100L), 21, 4, 2, 1);
	a_obj(ltree, L_BACK, L_LEFT, L_SLID, L_SLID, G_BOX, TOUCHEXIT, NORMAL, (void*)0x00011111L, 21, 2, 2, 2);
	a_obj(ltree, L_SLID, L_BACK, -1, -1, G_BOX, TOUCHEXIT, NORMAL, (void*)0x00011100L, 0, 0, 2, 1);
	a_obj(ltree, L_LEFT, L_RIGHT, -1, -1, G_BOXCHAR, TOUCHEXIT, NORMAL, (void*)((4L << 24) | 0x00011100L), 1, 6, 2, 1);
	a_obj(ltree, L_RIGHT, L_HBACK, -1, -1, G_BOXCHAR, TOUCHEXIT, NORMAL, (void*)((3L << 24) | 0x00011100L), 19, 6, 2, 1);
	a_obj(ltree, L_HBACK, L_ROOT, L_HSLID, L_HSLID, G_BOX, TOUCHEXIT, NORMAL, (void*)0x00011111L, 3, 6, 16, 1);
	a_obj(ltree, L_HSLID, L_HBACK, -1, -1, G_BOX, TOUCHEXIT | LASTOB, NORMAL, (void*)0x00011100L, 0, 0, 4, 1);
	a_fix_tree(ltree, L_COUNT);
	short x, y, w, h;
	form_center(ltree, &x, &y, &w, &h);
}

// A list of n items, each allocated with Malloc (lbox_free_items/lbox_free_list release them).
static LBOX_ITEM* make_items(int n, LBOX_ITEM** arr)
{
	LBOX_ITEM* first = 0;
	for (int i = n - 1; i >= 0; i--)
	{
		LBOX_ITEM* it = Malloc(sizeof(LBOX_ITEM));
		memset(it, 0, sizeof(*it));
		it->next = first;
		it->data1 = (short)i;
		first = it;
		arr[i] = it;
	}
	return first;
}

static void test_lbox(void)
{
	static const char* funcs[] =
	{
		"lbox_create", "lbox_update", "lbox_do", "lbox_delete", "lbox_cnt_items", "lbox_get_tree", "box_get_visible",
		"lbox_get_udata", "lbox_get_slct_idx", "lbox_get_items", "lbox_get_item", "lbox_get_slct_item", "lbox_get_idx",
		"lbox_get_bvis", "lbox_get_bentries", "lbox_get_bfirst", "lbox_set_items", "lbox_free_items", "lbox_free_list",
		"lbox_set_bsldr", "lbox_set_bentries", "lbox_bscroll_to", "lbox_get_avis", "lbox_get_visible", "lbox_get_afirst",
		"lbox_get_first", "lbox_set_asldr", "lbox_set_slider", "lbox_ascroll_to", "lbox_scroll_to", 0
	};
	if (!(gi7 & 2))
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no WDIALOG list boxes (appl_getinfo(7) = 0x%x)", gi7);
		}
		return;
	}
	static int udata;
	static const short ctrl[9] = { L_BOX, L_UP, L_DOWN, L_BACK, L_SLID, L_LEFT, L_RIGHT, L_HBACK, L_HSLID };
	static const short objs[4] = { L_I1, L_I2, L_I3, L_I4 };
	LBOX_ITEM* arr[6];
	build_ltree();
	LBOX_ITEM* items = make_items(6, arr);
	arr[1]->selected = 1;

	t_run("lbox_create");
	s_calls = 0;
	lbox_udata = &udata;
	s_bad = c_calls = c_bad = 0;
	LIST_BOX* box = lbox_create(ltree, slct_c, set_c, items, 4, 0, ctrl, objs,
		LBOX_VERT | LBOX_AUTO | LBOX_SNGL | LBOX_REAL | LBOX_2SLDRS, 20, &udata, 0, 10, 0, 30, 20);
	if (!CHECK_TRUE("lbox_create", box != 0, "lbox_create() = %p", box))
	{
		return;
	}
	// set_item(box, tree, item, obj_index, user_data, rect, first) for each visible item object.
	CHECK_TRUE("lbox_create", s_calls >= 4 && s_bad == 0, "set_item called %d times, %d with wrong arguments (tree, item object, user data)", s_calls, s_bad);

	t_run("lbox_cnt_items");
	CHECK_EQ("lbox_cnt_items", lbox_cnt_items(box), 6);
	t_run("lbox_get_tree");
	CHECK_EQ("lbox_get_tree", (long)lbox_get_tree(box), (long)ltree);
	t_run("lbox_get_udata");
	CHECK_EQ("lbox_get_udata", (long)lbox_get_udata(box), (long)&udata);
	t_run("lbox_get_visible");
	CHECK_EQ("lbox_get_visible", lbox_get_visible(box), 4);
	t_run("lbox_get_avis");
	CHECK_EQ("lbox_get_avis", lbox_get_avis(box), 4);
	t_run("box_get_visible");
	CHECK_EQ("box_get_visible", box_get_visible(box), 4);
	t_run("lbox_get_first");
	CHECK_EQ("lbox_get_first", lbox_get_first(box), 0);
	t_run("lbox_get_afirst");
	CHECK_EQ("lbox_get_afirst", lbox_get_afirst(box), 0);
	t_run("lbox_get_items");
	CHECK_EQ("lbox_get_items", (long)lbox_get_items(box), (long)items);
	t_run("lbox_get_item");
	CHECK_EQ("lbox_get_item", (long)lbox_get_item(box, 3), (long)arr[3]);
	t_run("lbox_get_idx");
	CHECK_EQ("lbox_get_idx", lbox_get_idx(items, arr[4]), 4);
	t_run("lbox_get_slct_idx");
	CHECK_EQ("lbox_get_slct_idx", lbox_get_slct_idx(box), 1);
	t_run("lbox_get_slct_item");
	CHECK_EQ("lbox_get_slct_item", (long)lbox_get_slct_item(box), (long)arr[1]);
	t_run("lbox_get_bvis");
	CHECK_EQ("lbox_get_bvis", lbox_get_bvis(box), 10);
	t_run("lbox_get_bentries");
	CHECK_EQ("lbox_get_bentries", lbox_get_bentries(box), 30);
	t_run("lbox_get_bfirst");
	CHECK_EQ("lbox_get_bfirst", lbox_get_bfirst(box), 0);

	GRECT r = { ltree[L_ROOT].ob_x, ltree[L_ROOT].ob_y, ltree[L_ROOT].ob_width, ltree[L_ROOT].ob_height };
	objc_draw(ltree, ROOT, MAX_DEPTH, r.g_x, r.g_y, r.g_w, r.g_h);
	t_run("lbox_scroll_to");
	lbox_scroll_to(box, 2, &r, &r);
	CHECK_EQ("lbox_scroll_to", lbox_get_first(box), 2);
	t_run("lbox_ascroll_to");
	lbox_ascroll_to(box, 1, &r, &r);
	CHECK_EQ("lbox_ascroll_to", lbox_get_afirst(box), 1);
	t_run("lbox_set_slider");
	lbox_set_slider(box, 2, &r);
	t_check(1, "lbox_set_slider", "called (void)");
	t_run("lbox_set_asldr");
	lbox_set_asldr(box, 1, &r);
	t_check(1, "lbox_set_asldr", "called (void)");
	t_run("lbox_set_bentries");
	lbox_set_bentries(box, 40);
	CHECK_EQ("lbox_set_bentries", lbox_get_bentries(box), 40);
	t_run("lbox_bscroll_to");
	lbox_bscroll_to(box, 5, &r, &r);
	CHECK_EQ("lbox_bscroll_to", lbox_get_bfirst(box), 5);
	t_run("lbox_set_bsldr");
	lbox_set_bsldr(box, 5, &r);
	t_check(1, "lbox_set_bsldr", "called (void)");
	t_run("lbox_update");
	lbox_update(box, &r);
	t_check(1, "lbox_update", "called (void)");

	// A click on the down arrow scrolls one line.
	t_run("lbox_do");
	short f0 = lbox_get_first(box);
	short dr = lbox_do(box, L_DOWN);
	CHECK_TRUE("lbox_do", lbox_get_first(box) == f0 + 1, "lbox_do(down arrow) = %d, first %d -> %d", dr, f0, lbox_get_first(box));
	// A click on an item selects it and calls slct_item.
	lbox_do(box, L_I3);
	CHECK_TRUE("lbox_do", c_calls >= 1 && c_bad == 0, "lbox_do(item) called slct_item %d times (obj %d state %d, obj %d state %d), %d with wrong arguments",
		c_calls, c_objs[0], c_states[0], c_objs[1], c_states[1], c_bad);

	// New items, free the old list, then free the new ones through the box.
	t_run("lbox_set_items");
	LBOX_ITEM* arr2[3];
	LBOX_ITEM* items2 = make_items(3, arr2);
	lbox_set_items(box, items2);
	CHECK_TRUE("lbox_set_items", lbox_cnt_items(box) == 3 && lbox_get_items(box) == items2, "lbox_set_items(): %d items", lbox_cnt_items(box));
	t_run("lbox_free_list");
	long mem0 = (long)Malloc(-1);
	lbox_free_list(items);
	CHECK_TRUE("lbox_free_list", (long)Malloc(-1) >= mem0, "lbox_free_list() released the old items");
	t_run("lbox_free_items");
	mem0 = (long)Malloc(-1);
	lbox_free_items(box);
	CHECK_TRUE("lbox_free_items", (long)Malloc(-1) >= mem0, "lbox_free_items() released the items");
	t_run("lbox_delete");
	CHECK_TRUE("lbox_delete", lbox_delete(box) != 0, "lbox_delete()");
}

/*
	Font dialog.
*/
static void test_fnts(void)
{
	static const char* funcs[] =
	{
		"fnts_create", "fnts_delete", "fnts_open", "fnts_close", "fnts_get_no_styles", "fnts_get_style", "fnts_get_name",
		"fnts_get_info", "fnts_add", "fnts_remove", "fnts_update", "fnts_evnt", "fnts_do", 0
	};
	if (!(gi7 & 4))
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no WDIALOG font dialog (appl_getinfo(7) = 0x%x)", gi7);
		}
		return;
	}
	short work_in[11] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
	static WS work_out;
	short vh = a_vdi;
	v_opnvwk(work_in, &vh, &work_out);
	short nfonts = 1 + vst_load_fonts(vh, 0);

	t_run("fnts_create");
	FNT_DIALOG* fd = fnts_create(vh, nfonts, FNTS_BTMP | FNTS_OUTL | FNTS_MONO | FNTS_PROP, FNTS_3D, "The quick brown fox", 0);
	if (!CHECK_TRUE("fnts_create", fd != 0, "fnts_create(%d fonts) = %p", nfonts, fd))
	{
		v_clsvwk(vh);
		return;
	}
	t_run("fnts_get_no_styles");
	short ns = fnts_get_no_styles(fd, 1);
	CHECK_TRUE("fnts_get_no_styles", ns >= 1, "fnts_get_no_styles(system font) = %d", ns);
	t_run("fnts_get_style");
	int sid = fnts_get_style(fd, 1, 1);
	CHECK_EQ("fnts_get_style", sid, 1);
	t_run("fnts_get_name");
	char full[64], fam[64], sty[64];
	memset(full, 0, sizeof(full));
	short nr = fnts_get_name(fd, 1, full, fam, sty);
	CHECK_TRUE("fnts_get_name", nr != 0 && full[0] != 0, "fnts_get_name(1) = %d \"%s\" family \"%s\"", nr, full, fam);
	t_run("fnts_get_info");
	short mono = -1, outline = -1;
	short idx = fnts_get_info(fd, 1, &mono, &outline);
	CHECK_TRUE("fnts_get_info", idx >= 1 && (mono == 0 || mono == 1) && outline == 0, "fnts_get_info(1) = %d, mono %d outline %d", idx, mono, outline);

	t_run("fnts_add");
	short ar = fnts_add(fd, 0);
	t_check(ar >= 0, "fnts_add", "fnts_add(no user fonts) = %d", ar);
	t_run("fnts_remove");
	fnts_remove(fd);
	t_check(1, "fnts_remove", "called (void)");

	t_run("fnts_open");
	short wh = fnts_open(fd, FNTS_SNAME | FNTS_SSTYLE | FNTS_SSIZE | FNTS_BSET, -1, -1, 1, 10L << 16, 1L << 16);
	CHECK_TRUE("fnts_open", wh > 0, "fnts_open() = %d", wh);
	t_run("fnts_update");
	CHECK_TRUE("fnts_update", fnts_update(fd, FNTS_SNAME | FNTS_SSTYLE | FNTS_SSIZE, 1, 12L << 16, 1L << 16) != 0, "fnts_update()");
	t_run("fnts_evnt");
	EVNT ev;
	memset(&ev, 0, sizeof(ev));
	short button = -1, cb = -1;
	int id = -1, pt = -1, ratio = -1;
	CHECK_EQ("fnts_evnt", fnts_evnt(fd, &ev, &button, &cb, &id, &pt, &ratio), 1);	// No event: continue
	t_run("fnts_close");
	short x = -1, y = -1;
	// The position is the one given to fnts_open (-1, -1: centered).
	CHECK_TRUE("fnts_close", fnts_close(fd, &x, &y) != 0, "fnts_close() position %d,%d", x, y);

	// The modal dialog has no default button (Return does nothing, checked with a screenshot),
	// and NVDI ignores simulated mouse clicks, so it can't be answered here.
	t_skip("fnts_do", "modal dialog without a default button, NVDI ignores simulated clicks (fnts_open/evnt/close tested)");

	t_run("fnts_delete");
	CHECK_TRUE("fnts_delete", fnts_delete(fd, vh) != 0, "fnts_delete()");
	vst_unload_fonts(vh, 0);
	v_clsvwk(vh);
}

/*
	Printer dialog.
*/
static void test_pdlg(void)
{
	static const char* funcs[] =
	{
		"pdlg_create", "pdlg_delete", "pdlg_open", "pdlg_close", "pdlg_get_setsize", "pdlg_add_printers", "pdlg_remove_printers",
		"pdlg_update", "pdlg_add_sub_dialogs", "pdlg_remove_sub_dialogs", "pdlg_new_settings", "pdlg_free_settings",
		"pdlg_dflt_settings", "pdlg_validate_settings", "pdlg_use_settings", "pdlg_save_default_settings", "pdlg_evnt", "pdlg_do", 0
	};
	if (!(gi7 & 16))
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no WDIALOG printer dialog (appl_getinfo(7) = 0x%x)", gi7);
		}
		return;
	}
	t_run("pdlg_create");
	PRN_DIALOG* pd = pdlg_create(PDLG_3D);
	if (!CHECK_TRUE("pdlg_create", pd != 0, "pdlg_create() = %p", pd))
	{
		return;
	}
	t_run("pdlg_get_setsize");
	int size = pdlg_get_setsize();
	CHECK_TRUE("pdlg_get_setsize", size > 100, "pdlg_get_setsize() = %d", size);
	t_run("pdlg_new_settings");
	PRN_SETTINGS* s = pdlg_new_settings(pd);
	if (!CHECK_TRUE("pdlg_new_settings", s != 0, "pdlg_new_settings() = %p", s))
	{
		pdlg_delete(pd);
		return;
	}
	t_run("pdlg_dflt_settings");
	CHECK_TRUE("pdlg_dflt_settings", pdlg_dflt_settings(pd, s) != 0, "pdlg_dflt_settings()");
	t_run("pdlg_validate_settings");
	CHECK_TRUE("pdlg_validate_settings", pdlg_validate_settings(pd, s) != 0, "pdlg_validate_settings()");
	t_run("pdlg_use_settings");
	CHECK_TRUE("pdlg_use_settings", pdlg_use_settings(pd, s) != 0, "pdlg_use_settings()");

	// The remove functions crash WDIALOG when nothing was added, and adding needs a DRV_INFO
	// (v_create_driver_info) or PDLG_SUB dialogs with callbacks.
	t_skip("pdlg_add_printers", "needs a DRV_INFO from v_create_driver_info");
	t_skip("pdlg_remove_printers", "only valid after pdlg_add_printers (crashes WDIALOG otherwise)");
	t_skip("pdlg_add_sub_dialogs", "needs PDLG_SUB dialogs with callbacks");
	t_skip("pdlg_remove_sub_dialogs", "only valid after pdlg_add_sub_dialogs");

	t_run("pdlg_open");
	short wh = pdlg_open(pd, s, "Test document", PDLG_PRINT, -1, -1);
	CHECK_TRUE("pdlg_open", wh > 0, "pdlg_open() = %d", wh);
	t_run("pdlg_update");
	CHECK_TRUE("pdlg_update", pdlg_update(pd, "Other document") != 0, "pdlg_update()");
	t_run("pdlg_evnt");
	EVNT ev;
	memset(&ev, 0, sizeof(ev));
	short button = -1;
	CHECK_EQ("pdlg_evnt", pdlg_evnt(pd, s, &ev, &button), 1);	// No event: continue
	t_run("pdlg_close");
	short x = -1, y = -1;
	CHECK_TRUE("pdlg_close", pdlg_close(pd, &x, &y) != 0, "pdlg_close() position %d,%d", x, y);

	t_skip("pdlg_do", "modal dialog can't be answered under NVDI (simulated clicks ignored), pdlg_open/evnt/close tested");

	// Last before the cleanup: if it calls the wrong subfunction (it used to be 1, pdlg_remove_printers)
	// it crashes WDIALOG.
	t_run("pdlg_save_default_settings");
	short sv = pdlg_save_default_settings(pd, s);
	t_check(sv >= 0, "pdlg_save_default_settings", "pdlg_save_default_settings() = %d returned (whether the settings were saved is not checked)", sv);

	t_run("pdlg_free_settings");
	CHECK_TRUE("pdlg_free_settings", pdlg_free_settings(s) != 0, "pdlg_free_settings()");
	t_run("pdlg_delete");
	CHECK_TRUE("pdlg_delete", pdlg_delete(pd) != 0, "pdlg_delete()");
}

/*
	The trampolines are also called directly here, with a Pure C style argument block (shorts as words),
	for the callbacks that WDIALOG/MagiC can't be made to call in these tests.
*/
__asm__(
	".globl pc_call\n"
	"pc_call:\n\t"				// pc_call(fn, words, nwords): push the words, words[0] lowest, call fn
	"movem.l	%d2/%a2, %a7@-\n\t"
	"move.l	%a7@(12), %a2\n\t"
	"move.l	%a7@(16), %a0\n\t"
	"move.l	%a7@(20), %d2\n\t"
	"move.l	%d2, %d1\n\t"
	"add.l	%d1, %d1\n\t"
	"add.l	%d1, %a0\n\t"
	"move.l	%d2, %d1\n\t"
	"bra.s	2f\n"
	"1:\n\t"
	"move.w	%a0@-, %a7@-\n"
	"2:\n\t"
	"dbra	%d1, 1b\n\t"
	"jsr	%a2@\n\t"
	"add.l	%d2, %d2\n\t"
	"add.l	%d2, %a7\n\t"
	"movem.l	%a7@+, %d2/%a2\n\t"
	"rts\n");
long pc_call(void* fn, const short* words, long nwords);

// Internal library symbols (impl/aes_tramp.c).
void _aes_tramp_hndl_obj(void);
void _aes_tramp_set_item(void);
void _aes_tramp_slct_item(void);
void _aes_tramp_xfrm_popup(void);
void _aes_tramp_pdlg_hndl(void);
void _aes_tramp_utxt(void);
short _aes_hndl_register(DIALOG* dialog, HNDL_OBJ fn);
void _aes_hndl_unregister(DIALOG* dialog);
short _aes_lbox_register(LIST_BOX* box, SET_ITEM set, SLCT_ITEM slct);
void _aes_lbox_unregister(LIST_BOX* box);
void _aes_xfrm_popup_set(XFRM_POPUP_CB fn);
short _aes_pdlg_subs_register(PRN_DIALOG* dialog, PDLG_SUB* subs);
void _aes_pdlg_subs_unregister(PRN_DIALOG* dialog);
short _aes_fnts_register(FNT_DIALOG* dialog, FNTS_ITEM* items);
void _aes_fnts_unregister(FNT_DIALOG* dialog);

#define HI(p) ((short)((long)(p) >> 16))
#define LO(p) ((short)(long)(p))
static long got[8];

static short t_hndl(DIALOG* d, EVNT* e, short obj, short clicks, void* data)
{
	got[0] = (long)d; got[1] = (long)e; got[2] = obj; got[3] = clicks; got[4] = (long)data;
	return 42;
}
static short t_set(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, short obj_index, void* user_data, GRECT* rect, short first)
{
	got[0] = (long)box; got[1] = (long)tree; got[2] = (long)item; got[3] = obj_index; got[4] = (long)user_data;
	got[5] = (long)rect; got[6] = first;
	return 43;
}
static void t_slct(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, void* user_data, short obj_index, short last_state)
{
	got[0] = (long)box; got[1] = (long)tree; got[2] = (long)item; got[3] = (long)user_data; got[4] = obj_index; got[5] = last_state;
}
static void t_xfrm(OBJECT* tree, short scrollpos, short nlines, void* param)
{
	got[0] = (long)tree; got[1] = scrollpos; got[2] = nlines; got[3] = (long)param;
}
static int t_pdlg(PRN_SETTINGS* settings, PDLG_SUB* sub, short exit_obj)
{
	got[0] = (long)settings; got[1] = (long)sub; got[2] = exit_obj;
	return 0x12345678;
}
static void t_utxt(short x, short y, short* clip_rect, int id, int pt, int ratio, char* string)
{
	got[0] = x; got[1] = y; got[2] = (long)clip_rect; got[3] = id; got[4] = pt; got[5] = ratio; got[6] = (long)string;
}

static void test_trampolines(void)
{
	void* A = (void*)0x00123456L;
	void* B = (void*)0x00abcdefL;
	void* C = (void*)0x00fedcbaL;
	void* D = (void*)0x00135790L;

	t_run("wdlg_create");
	memset(got, 0, sizeof(got));
	_aes_hndl_register((DIALOG*)A, t_hndl);
	short w1[] = { HI(A), LO(A), HI(B), LO(B), -1, 77, HI(C), LO(C) };
	long r = pc_call(_aes_tramp_hndl_obj, w1, 8);
	_aes_hndl_unregister((DIALOG*)A);
	CHECK_TRUE("wdlg_create", (short)r == 42 && got[0] == (long)A && got[1] == (long)B && got[2] == -1 && got[3] == 77 && got[4] == (long)C,
		"handle_exit trampoline: returned %d, args %lx %lx %ld %ld %lx", (short)r, got[0], got[1], got[2], got[3], got[4]);

	t_run("lbox_create");
	memset(got, 0, sizeof(got));
	_aes_lbox_register((LIST_BOX*)A, t_set, t_slct);
	short w2[] = { HI(A), LO(A), HI(B), LO(B), HI(C), LO(C), 5, HI(D), LO(D), HI(A), LO(A), -2 };
	r = pc_call(_aes_tramp_set_item, w2, 12);
	CHECK_TRUE("lbox_create", (short)r == 43 && got[0] == (long)A && got[1] == (long)B && got[2] == (long)C && got[3] == 5
		&& got[4] == (long)D && got[5] == (long)A && got[6] == -2, "set_item trampoline: returned %d, obj_index %ld user_data %lx first %ld",
		(short)r, got[3], got[4], got[6]);
	memset(got, 0, sizeof(got));
	short w3[] = { HI(A), LO(A), HI(B), LO(B), HI(C), LO(C), HI(D), LO(D), 6, 1 };
	pc_call(_aes_tramp_slct_item, w3, 10);
	_aes_lbox_unregister((LIST_BOX*)A);
	CHECK_TRUE("lbox_create", got[0] == (long)A && got[3] == (long)D && got[4] == 6 && got[5] == 1,
		"slct_item trampoline: user_data %lx obj_index %ld last_state %ld", got[3], got[4], got[5]);

	t_run("xfrm_popup");
	memset(got, 0, sizeof(got));
	_aes_xfrm_popup_set(t_xfrm);
	short w4[] = { HI(A), LO(A), 3, 9, HI(B), LO(B) };
	pc_call(_aes_tramp_xfrm_popup, w4, 6);
	_aes_xfrm_popup_set(0);
	CHECK_TRUE("xfrm_popup", got[0] == (long)A && got[1] == 3 && got[2] == 9 && got[3] == (long)B,
		"init trampoline: scrollpos %ld nlines %ld param %lx", got[1], got[2], got[3]);

	t_run("pdlg_add_sub_dialogs");
	memset(got, 0, sizeof(got));
	static PDLG_SUB sub;
	memset(&sub, 0, sizeof(sub));
	sub.do_dlg = t_pdlg;
	_aes_pdlg_subs_register((PRN_DIALOG*)C, &sub);
	int replaced = sub.do_dlg == (PDLG_HNDL)_aes_tramp_pdlg_hndl;
	short w5[] = { HI(A), LO(A), HI(&sub), LO(&sub), 12 };
	r = pc_call((void*)sub.do_dlg, w5, 5);
	_aes_pdlg_subs_unregister((PRN_DIALOG*)C);
	CHECK_TRUE("pdlg_add_sub_dialogs", replaced && r == 0x12345678 && got[0] == (long)A && got[1] == (long)&sub && got[2] == 12 && sub.do_dlg == t_pdlg,
		"do_dlg trampoline: returned 0x%lx, exit_obj %ld, handler put back: %s", r, got[2], sub.do_dlg == t_pdlg ? "yes" : "no");

	t_run("fnts_add");
	memset(got, 0, sizeof(got));
	static FNTS_ITEM fi;
	memset(&fi, 0, sizeof(fi));
	fi.display = t_utxt;
	fi.id = 1234;
	_aes_fnts_register((FNT_DIALOG*)C, &fi);
	replaced = fi.display == (UTXT_FN)_aes_tramp_utxt;
	short w6[] = { 10, -3, HI(B), LO(B), 0, 1234, 0, 12, 1, 0, HI(D), LO(D) };
	pc_call((void*)fi.display, w6, 12);
	_aes_fnts_unregister((FNT_DIALOG*)C);
	CHECK_TRUE("fnts_add", replaced && got[0] == 10 && got[1] == -3 && got[2] == (long)B && got[3] == 1234 && got[4] == 12 && got[5] == 0x10000
		&& got[6] == (long)D && fi.display == t_utxt, "display trampoline: x %ld y %ld id %ld pt %ld ratio 0x%lx, put back: %s",
		got[0], got[1], got[3], got[4], got[5], fi.display == t_utxt ? "yes" : "no");
}

static void test_magic_only(void)
{
	static const char* fslx[] = { "fslx_open", "fslx_close", "fslx_getnxtfile", "fslx_evnt", "fslx_do", "fslx_set_flags", 0 };
	static const char* edit[] =
	{
		"edit_create", "edit_open", "edit_close", "edit_delete", "edit_cursor", "edit_evnt", "edit_get_buf", "edit_get_format",
		"edit_get_colour", "edit_get_font", "edit_get_cursor", "edit_get_pos", "edit_get_dirty", "edit_get_sel",
		"edit_get_scrollinfo", "edit_set_buf", "edit_set_format", "edit_set_colour", "edit_set_font", "edit_set_cursor",
		"edit_set_pos", "edit_resized", "edit_set_dirty", "edit_scroll", 0
	};
	for (int i = 0; fslx[i]; i++)
	{
		t_skip(fslx[i], "file selector extension not available (appl_getinfo(7) = 0x%x, MagiC)", gi7);
	}
	for (int i = 0; edit[i]; i++)
	{
		t_skip(edit[i], "editor objects need MagiC");
	}
}

int main(void)
{
	if (!a_begin("A_WDLG"))
	{
		return t_end();
	}
	short d;
	gi7 = 0;
	a_getinfo(7, &gi7, &d, &d, &d);
	t_print("appl_getinfo(7) = 0x%x", gi7);
	test_wdlg();
	test_lbox();
	test_fnts();
	test_trampolines();
	test_magic_only();
	test_pdlg();
	return a_end();
}
