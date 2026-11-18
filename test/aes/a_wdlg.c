/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	WDIALOG extensions (NVDI): window dialogs, list boxes, font and printer dialogs.
	Availability comes from appl_getinfo(7). fslx_ (file selector) and edit_ (editor) need MagiC.

	WDIALOG calls back into the program (handle_exit, list box callbacks) with Pure C conventions:
	short parameters are 2 bytes on the stack. Each callback goes through a stub that saves the
	raw stack, so the tests can show what WDIALOG passed and what the C code received.
*/

#include <string.h>
#include <vdi.h>
#include "a_tree.h"

static short gi7;	// appl_getinfo(7) bits: 1 wdlg, 2 lbox, 4 fnts, 8 fslx, 16 pdlg

/*
	Callback stubs: copy 24 bytes of arguments from the stack, then jump to the C function.
*/
volatile short raw_args[12];
__asm__(
	".globl save_args\n"
	"save_args:\n\t"
	"lea	raw_args, %a0\n\t"
	"move.l	%a7@(8), %a0@+\n\t"
	"move.l	%a7@(12), %a0@+\n\t"
	"move.l	%a7@(16), %a0@+\n\t"
	"move.l	%a7@(20), %a0@+\n\t"
	"move.l	%a7@(24), %a0@+\n\t"
	"move.l	%a7@(28), %a0@+\n\t"
	"rts\n"
	".globl hndl_stub\n"
	"hndl_stub:\n\t"
	"jsr	save_args\n\t"
	"jmp	hndl_c\n"
	".globl set_stub\n"
	"set_stub:\n\t"
	"jsr	save_args\n\t"
	"jmp	set_c\n"
	".globl slct_stub\n"
	"slct_stub:\n\t"
	"jsr	save_args\n\t"
	"jmp	slct_c\n");
short hndl_stub(DIALOG* dialog, EVNT* events, short obj, short clicks, void* data);
short set_stub(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, short obj_index, void* user_data, GRECT* rect, short first);
void slct_stub(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, void* user_data, short obj_index, short last_state);

/*
	Window dialog.
*/
static int h_calls;
static short h_obj, h_clicks, h_raw[12];
static void* h_data;
static int h_closed;

short hndl_c(DIALOG* dialog, EVNT* events, short obj, short clicks, void* data)
{
	if (h_calls++ == 0)
	{
		memcpy(h_raw, (const void*)raw_args, sizeof(h_raw));
		h_obj = obj;
		h_clicks = clicks;
		h_data = data;
	}
	// Pure C passes obj as the first word after the two pointers.
	if (raw_args[4] == HNDL_CLSD)
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
	DIALOG* d = wdlg_create(hndl_stub, tree, &udata, 77, &cdata, 0);
	if (!CHECK_TRUE("wdlg_create", d != 0, "wdlg_create() = %p", d))
	{
		return;
	}
	CHECK_TRUE("wdlg_create", h_calls >= 1 && h_raw[4] == HNDL_INIT && h_raw[5] == 77, "WDIALOG called handle_exit %d times, raw stack obj %d clicks %d",
		h_calls, h_raw[4], h_raw[5]);
	CHECK_TRUE("wdlg_create", h_obj == HNDL_INIT && h_clicks == 77 && h_data == &cdata,
		"handle_exit(obj, clicks, data) as declared in aes.h receives obj %d clicks %d data %p (WDIALOG passed %d, 77, %p)",
		h_obj, h_clicks, h_data, HNDL_INIT, &cdata);

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
static int s_calls, c_calls;
static short s_raw[12];

short set_c(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, short obj_index, void* user_data, GRECT* rect, short first)
{
	if (s_calls++ == 0)
	{
		memcpy(s_raw, (const void*)raw_args, sizeof(s_raw));
	}
	return 0;
}

void slct_c(LIST_BOX* box, OBJECT* tree, LBOX_ITEM* item, void* user_data, short obj_index, short last_state)
{
	c_calls++;
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
	LIST_BOX* box = lbox_create(ltree, slct_stub, set_stub, items, 4, 0, ctrl, objs,
		LBOX_VERT | LBOX_AUTO | LBOX_SNGL | LBOX_REAL | LBOX_2SLDRS, 20, &udata, 0, 10, 0, 30, 20);
	if (!CHECK_TRUE("lbox_create", box != 0, "lbox_create() = %p", box))
	{
		return;
	}
	// set_item(box, tree, item, obj_index, user_data, rect, first): obj_index is the word after the 3 pointers.
	CHECK_TRUE("lbox_create", s_calls >= 4 && s_raw[6] >= L_I1 && s_raw[6] <= L_I4, "set_item called %d times, raw stack obj_index %d", s_calls, s_raw[6]);

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
	test_magic_only();
	test_pdlg();
	return a_end();
}
