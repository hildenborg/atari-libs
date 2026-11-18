/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Graphics functions, on the dialog tree of a_tree.h.
	The mouse tracking functions (graf_watchbox, graf_dragbox...) are driven by simulated mouse
	packets. They run in their own program, before any test ends a dialog with simulated keys:
	after that the AES can lose track of the button state (EmuTOS always, TOS after the file selector).
*/

#include "a_tree.h"

static short mform_data[37] =
{
	0, 0, 1, 0, 1,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001
};

static void test_graf(void)
{
	short x, y, w, h, mx, my, b, k;
	form_center(tree, &x, &y, &w, &h);

	t_run("graf_handle");
	CHECK_TRUE("graf_handle", a_vdi > 0 && a_wchar == 8 && (a_hchar == 8 || a_hchar == 16) && a_wbox >= a_wchar && a_hbox >= a_hchar,
		"graf_handle() = %d, char %dx%d, box %dx%d", a_vdi, a_wchar, a_hchar, a_wbox, a_hbox);
	t_run("graf_xhandle");
	short xw, xh, xbw, xbh, dev = -1;
	short v = graf_xhandle(&xw, &xh, &xbw, &xbh, &dev);
	CHECK_TRUE("graf_xhandle", v == a_vdi && xw == a_wchar && xh == a_hchar && xbw == a_wbox && xbh == a_hbox,
		"graf_xhandle() = %d, same values as graf_handle, device %d", v, dev);

	t_run("graf_mouse");
	CHECK_TRUE("graf_mouse", graf_mouse(M_OFF, 0) != 0 && graf_mouse(M_ON, 0) != 0 && graf_mouse(BUSYBEE, 0) != 0
		&& graf_mouse(USER_DEF, (MFORM*)mform_data) != 0 && graf_mouse(ARROW, 0) != 0, "graf_mouse(off, on, busy, user, arrow)");

	// NVDI handles the mouse itself and doesn't see the button state of simulated mouse packets.
	if (t_cookie(0x4E564449, 0))
	{
		static const char* drag[] = { "graf_mkstate", "graf_watchbox", "graf_dragbox", "graf_rubberbox", "graf_slidebox", 0 };
		for (int i = 0; drag[i]; i++)
		{
			t_skip(drag[i], "NVDI doesn't see simulated mouse buttons (tested without NVDI)");
		}
		return;
	}
	t_run("graf_mkstate");
	a_mouse_to(77, 66);
	a_mouse_button(1);
	graf_mkstate(&mx, &my, &b, &k);
	a_mouse_button(0);
	CHECK_TRUE("graf_mkstate", mx == 77 && my == 66 && (b & 1), "graf_mkstate() %d,%d buttons %d", mx, my, b);
	graf_mkstate(&mx, &my, &b, &k);
	CHECK_EQ("graf_mkstate", b, 0);

	t_run("graf_growbox");
	CHECK_TRUE("graf_growbox", graf_growbox(10, 10, 16, 16, x, y, w, h) != 0, "graf_growbox()");
	t_run("graf_shrinkbox");
	CHECK_TRUE("graf_shrinkbox", graf_shrinkbox(10, 10, 16, 16, x, y, w, h) != 0, "graf_shrinkbox()");
	t_run("graf_movebox");
	CHECK_TRUE("graf_movebox", graf_movebox(32, 16, 10, 10, 100, 80) != 0, "graf_movebox()");
	t_run("graf_mbox");
	CHECK_TRUE("graf_mbox", graf_mbox(32, 16, 100, 80, 10, 10) != 0, "graf_mbox()");

	// These track the mouse while the button is held. The button is pressed here, the timer
	// interrupt moves the mouse and releases the button while the call runs.
	objc_draw(tree, ROOT, MAX_DEPTH, x, y, w, h);
	short bx, by;
	center_of(D_OK, &bx, &by);
	t_run("graf_watchbox");
	a_mouse_to(bx, by);
	a_mouse_button(1);
	a_later_mouse(300, 0, 0, 0);
	a_later_start();
	short r = graf_watchbox(tree, D_OK, SELECTED, NORMAL);
	a_later_done();
	CHECK_EQ("graf_watchbox", r, 1);	// Released inside
	tree[D_OK].ob_state = NORMAL;
	a_mouse_to(bx, by);
	a_mouse_button(1);
	a_later_mouse(300, -100, -60, 1);
	a_later_mouse(200, 0, 0, 0);
	a_later_start();
	r = graf_watchbox(tree, D_OK, SELECTED, NORMAL);
	a_later_done();
	CHECK_EQ("graf_watchbox", r, 0);	// Released outside
	tree[D_OK].ob_state = NORMAL;

	t_run("graf_dragbox");
	short ex = -1, ey = -1;
	a_mouse_to(bx, by);
	a_mouse_button(1);
	a_later_mouse(300, 30, 20, 1);
	a_later_mouse(200, 0, 0, 0);
	a_later_start();
	graf_dragbox(40, 20, bx - 5, by - 5, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h, &ex, &ey);
	a_later_done();
	CHECK_TRUE("graf_dragbox", ex == bx - 5 + 30 && ey == by - 5 + 20, "graf_dragbox() moved the box to %d,%d, expected %d,%d",
		ex, ey, bx - 5 + 30, by - 5 + 20);

	t_run("graf_rubberbox");
	short lw = -1, lh = -1;
	a_mouse_to(bx + 10, by + 10);
	a_mouse_button(1);
	a_later_mouse(300, 40, 30, 1);
	a_later_mouse(200, 0, 0, 0);
	a_later_start();
	graf_rubberbox(bx, by, 8, 8, &lw, &lh);
	a_later_done();
	CHECK_TRUE("graf_rubberbox", lw >= 49 && lw <= 51 && lh >= 39 && lh <= 41, "graf_rubberbox() size %dx%d, expected about 50x40", lw, lh);

	// Slider: the char box in the dialog, moved horizontally.
	t_run("graf_slidebox");
	short sx, sy;
	center_of(D_CHAR, &sx, &sy);
	a_mouse_to(sx, sy);
	a_mouse_button(1);
	a_later_mouse(300, 0, 0, 0);
	a_later_start();
	short p0 = graf_slidebox(tree, ROOT, D_CHAR, 0);
	a_later_done();
	a_mouse_to(sx, sy);
	a_mouse_button(1);
	a_later_mouse(300, -80, 0, 1);
	a_later_mouse(200, 0, 0, 0);
	a_later_start();
	short p1 = graf_slidebox(tree, ROOT, D_CHAR, 0);
	a_later_done();
	CHECK_TRUE("graf_slidebox", p0 > 800 && p0 <= 1000 && p1 >= 0 && p1 < p0 - 200, "graf_slidebox() = %d unmoved, %d after 80 pixels left", p0, p1);
	form_dial(FMD_FINISH, x, y, w, h, x, y, w, h);
}

int main(void)
{
	if (!a_begin("A_GRAF"))
	{
		return t_end();
	}
	build_tree();
	test_graf();
	return a_end();
}
