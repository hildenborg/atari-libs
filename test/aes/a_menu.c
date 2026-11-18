/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Menu functions. A menu bar is built in memory and an entry is picked with the simulated mouse,
	which must give an MN_SELECTED message. Pop up menus (AES 3.3) are clicked the same way.
*/

#include <string.h>
#include "a_util.h"

enum
{
	M_ROOT, M_BAR, M_ACTIVE, M_TDESK, M_TFILE, M_SCREEN,
	M_DESKBOX, M_ABOUT, M_SEP, M_ACC1, M_ACC2, M_ACC3, M_ACC4, M_ACC5, M_ACC6,
	M_FILEBOX, M_OPEN, M_QUIT, M_COUNT
};

static OBJECT menu[M_COUNT];
static char t_desk[] = " Desk ", t_file[] = " File ";
static char s_about[] = "  About test...    ", s_sep[] = "-------------------";
static char s_acc[6][20] = { "  Desk Accessory 1 ", "  Desk Accessory 2 ", "  Desk Accessory 3 ", "  Desk Accessory 4 ",
	"  Desk Accessory 5 ", "  Desk Accessory 6 " };
static char s_open[] = "  Open   ", s_quit[] = "  Quit   ";

static void build_menu(void)
{
	short cols = a_desk.g_w / a_wchar;
	a_obj(menu, M_ROOT, -1, M_BAR, M_SCREEN, G_IBOX, NONE, NORMAL, 0, 0, 0, cols, 25);
	a_obj(menu, M_BAR, M_SCREEN, M_ACTIVE, M_ACTIVE, G_BOX, NONE, NORMAL, (void*)0x00001100L, 0, 0, cols, 1);
	a_obj(menu, M_ACTIVE, M_BAR, M_TDESK, M_TFILE, G_IBOX, NONE, NORMAL, 0, 2, 0, 12, 1);
	a_obj(menu, M_TDESK, M_TFILE, -1, -1, G_TITLE, NONE, NORMAL, t_desk, 0, 0, 6, 1);
	a_obj(menu, M_TFILE, M_ACTIVE, -1, -1, G_TITLE, NONE, NORMAL, t_file, 6, 0, 6, 1);
	a_obj(menu, M_SCREEN, M_ROOT, M_DESKBOX, M_FILEBOX, G_IBOX, NONE, NORMAL, 0, 0, 1, cols, 19);
	a_obj(menu, M_DESKBOX, M_FILEBOX, M_ABOUT, M_ACC6, G_BOX, NONE, NORMAL, (void*)0x00ff1100L, 2, 0, 19, 8);
	a_obj(menu, M_ABOUT, M_SEP, -1, -1, G_STRING, NONE, NORMAL, s_about, 0, 0, 19, 1);
	a_obj(menu, M_SEP, M_ACC1, -1, -1, G_STRING, NONE, DISABLED, s_sep, 0, 1, 19, 1);
	for (int i = 0; i < 6; i++)
	{
		a_obj(menu, (short)(M_ACC1 + i), (short)(i < 5 ? M_ACC1 + i + 1 : M_DESKBOX), -1, -1, G_STRING, NONE, NORMAL, s_acc[i], 0, (short)(2 + i), 19, 1);
	}
	a_obj(menu, M_FILEBOX, M_SCREEN, M_OPEN, M_QUIT, G_BOX, NONE, NORMAL, (void*)0x00ff1100L, 8, 0, 9, 2);
	a_obj(menu, M_OPEN, M_QUIT, -1, -1, G_STRING, NONE, NORMAL, s_open, 0, 0, 9, 1);
	a_obj(menu, M_QUIT, M_FILEBOX, -1, -1, G_STRING, LASTOB, NORMAL, s_quit, 0, 1, 9, 1);
	a_fix_tree(menu, M_COUNT);
}

static void center(OBJECT* tree, short obj, short* x, short* y)
{
	objc_offset(tree, obj, x, y);
	*x += tree[obj].ob_width / 2;
	*y += tree[obj].ob_height / 2;
}

static short wait_message(short* msg, short ms)
{
	short d;
	for (int i = 0; i < 10; i++)
	{
		short r = evnt_multi(MU_MESAG | MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, ms, 0, &d, &d, &d, &d, &d, &d);
		if (!(r & MU_MESAG))
		{
			return 0;
		}
		if (msg[0] == MN_SELECTED)
		{
			return 1;
		}
	}
	return 0;
}

static void test_menu_bar(void)
{
	t_run("menu_bar");
	CHECK_TRUE("menu_bar", menu_bar(menu, MENU_SHOW) != 0, "menu_bar(show)");

	t_run("menu_icheck");
	menu_icheck(menu, M_OPEN, 1);
	CHECK_TRUE("menu_icheck", menu[M_OPEN].ob_state & CHECKED, "menu_icheck(Open, 1) state 0x%x", menu[M_OPEN].ob_state);
	menu_icheck(menu, M_OPEN, 0);
	CHECK_EQ("menu_icheck", menu[M_OPEN].ob_state & CHECKED, 0);

	t_run("menu_ienable");
	menu_ienable(menu, M_QUIT, 0);
	CHECK_TRUE("menu_ienable", menu[M_QUIT].ob_state & DISABLED, "menu_ienable(Quit, 0) state 0x%x", menu[M_QUIT].ob_state);
	menu_ienable(menu, M_QUIT, 1);
	CHECK_EQ("menu_ienable", menu[M_QUIT].ob_state & DISABLED, 0);

	t_run("menu_tnormal");
	menu_tnormal(menu, M_TFILE, 0);
	CHECK_TRUE("menu_tnormal", menu[M_TFILE].ob_state & SELECTED, "menu_tnormal(File, 0) shows the title selected");
	menu_tnormal(menu, M_TFILE, 1);
	CHECK_EQ("menu_tnormal", menu[M_TFILE].ob_state & SELECTED, 0);

	t_run("menu_text");
	CHECK_TRUE("menu_text", menu_text(menu, M_OPEN, "  Load   ") != 0 && strcmp(s_open, "  Load   ") == 0, "menu_text(Open) = \"%s\"", s_open);

	// Pick File / Load with the mouse: point at the title, the menu drops down, click the entry.
	t_run("menu_bar");
	short tx, ty, ix, iy, msg[8];
	center(menu, M_TFILE, &tx, &ty);
	center(menu, M_OPEN, &ix, &iy);
	a_mouse_to(tx, ty);
	a_drain_keys();
	a_later_mouse(300, ix - tx, iy - ty, 0);
	a_later_mouse(300, 0, 0, 1);
	a_later_mouse(100, 0, 0, 0);
	a_later_start();
	memset(msg, 0, sizeof(msg));
	short got = wait_message(msg, 1500);
	a_later_done();
	CHECK_TRUE("menu_bar", got && msg[3] == M_TFILE && msg[4] == M_OPEN, "MN_SELECTED title %d item %d (expected %d %d)", msg[3], msg[4], M_TFILE, M_OPEN);
	menu_tnormal(menu, M_TFILE, 1);
	a_mouse_to(a_desk.g_x + a_desk.g_w / 2, a_desk.g_y + a_desk.g_h - 10);

	t_run("menu_register");
	short id = menu_register(a_apid, "  Test entry");
	CHECK_TRUE("menu_register", id >= -1 && id <= 5, "menu_register() = %d", id);

	if (a_aesver >= 0x410)
	{
		t_run("menu_unregister");
		CHECK_TRUE("menu_unregister", menu_unregister(id) != 0, "menu_unregister()");
	}
	else
	{
		t_skip("menu_unregister", "AES 0x%04x has no menu_unregister", a_aesver);
	}
	t_skip("menu_click", "MagiC only");

	CHECK_TRUE("menu_bar", menu_bar(menu, MENU_HIDE) != 0, "menu_bar(hide)");
}

/*
	Pop up menu (AES 3.3).
*/
enum { P_ROOT, P_ONE, P_TWO, P_THREE, P_COUNT };
static OBJECT popup[P_COUNT];

static void build_popup(void)
{
	a_obj(popup, P_ROOT, -1, P_ONE, P_THREE, G_BOX, NONE, NORMAL, (void*)0x00ff1100L, 0, 0, 10, 3);
	a_obj(popup, P_ONE, P_TWO, -1, -1, G_STRING, SELECTABLE, NORMAL, "  One", 0, 0, 10, 1);
	a_obj(popup, P_TWO, P_THREE, -1, -1, G_STRING, SELECTABLE, NORMAL, "  Two", 0, 1, 10, 1);
	a_obj(popup, P_THREE, P_ROOT, -1, -1, G_STRING, SELECTABLE | LASTOB, NORMAL, "  Three", 0, 2, 10, 1);
	a_fix_tree(popup, P_COUNT);
}

static void test_popup(void)
{
	static const char* funcs[] = { "menu_popup", "menu_attach", "menu_istart", "menu_settings", 0 };
	if (a_aesver < 0x330)
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "AES 0x%04x has no pop up menus (AES 3.3)", a_aesver);
		}
		return;
	}
	build_popup();
	MENU m = { popup, P_ROOT, P_ONE, 0, 0 }, out;
	memset(&out, 0, sizeof(out));
	// The pop up opens with item One at 100,60. Move to Two and click.
	t_run("menu_popup");
	short px = 100, py = 60;
	a_mouse_to(px + 20, py + a_hchar / 2);
	a_later_mouse(300, 0, a_hchar, 0);
	a_later_mouse(200, 0, 0, 1);
	a_later_mouse(100, 0, 0, 0);
	a_later_key(2000, 0x01, 0x1b);	// Escape, in case the click is lost
	a_later_start();
	short r = menu_popup(&m, px, py, &out);
	a_later_done();
	CHECK_TRUE("menu_popup", r == 1 && out.mn_item == P_TWO && out.mn_tree == popup, "menu_popup() = %d, item %d (expected %d)", r, out.mn_item, P_TWO);
	a_drain_keys();

	t_run("menu_settings");
	MN_SET set, set2;
	memset(&set, 0, sizeof(set));
	r = menu_settings(0, &set);
	CHECK_TRUE("menu_settings", r != 0 && set.display > 0 && set.height > 0,
		"menu_settings(get) display %d drag %d delay %d speed %d height %d", set.display, set.drag, set.delay, set.speed, set.height);
	set2 = set;
	set2.height = 5;
	menu_settings(1, &set2);
	memset(&set2, 0, sizeof(set2));
	menu_settings(0, &set2);
	CHECK_EQ("menu_settings", set2.height, 5);
	menu_settings(1, &set);

	// Attach the pop up as a submenu of File/Quit, inquire, set the start item, remove.
	// TOS 4.04 crashes on submenus built in memory (in inquire, menu_istart and remove), and a
	// submenu left attached breaks the AES for later programs. The bindings are tested on EmuTOS.
	if (!t_env.emutos)
	{
		t_skip("menu_attach", "TOS 0x%04x crashes with in memory submenus, tested on EmuTOS", t_env.tos);
		t_skip("menu_istart", "TOS 0x%04x crashes with in memory submenus, tested on EmuTOS", t_env.tos);
		return;
	}
	build_menu();
	t_run("menu_attach");
	MENU sub = { popup, P_ROOT, P_ONE, 0, 0 }, q;
	r = menu_attach(1, menu, M_QUIT, &sub);
	CHECK_TRUE("menu_attach", r != 0 && (menu[M_QUIT].ob_flags & SUBMENU), "menu_attach(attach) = %d, flags 0x%x", r, menu[M_QUIT].ob_flags);
	memset(&q, 0, sizeof(q));
	r = menu_attach(0, menu, M_QUIT, &q);
	CHECK_TRUE("menu_attach", r != 0 && q.mn_tree == popup && q.mn_menu == P_ROOT, "menu_attach(inquire) = %d, tree %p", r, q.mn_tree);

	t_run("menu_istart");
	CHECK_EQ("menu_istart", menu_istart(1, popup, P_ROOT, P_THREE), P_THREE);
	CHECK_EQ("menu_istart", menu_istart(0, popup, P_ROOT, 0), P_THREE);

	r = menu_attach(2, menu, M_QUIT, &sub);
	CHECK_TRUE("menu_attach", r != 0 && !(menu[M_QUIT].ob_flags & SUBMENU), "menu_attach(remove) = %d", r);
}

int main(void)
{
	if (!a_begin("A_MENU"))
	{
		return t_end();
	}
	build_menu();
	test_menu_bar();
	test_popup();
	return a_end();
}
