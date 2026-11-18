/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Window functions, including the wind_get_.../wind_set_... helpers.
*/

#include <string.h>
#include "a_util.h"

#define KIND (NAME | CLOSER | MOVER | INFO | HSLIDE | VSLIDE | UPARROW | DNARROW | LFARROW | RTARROW | SIZER)

static char title[] = " Test window ";
static char info[] = " Info line ";

// Wait for a WM_REDRAW for this window (the AES sends one when it opens).
static int got_redraw(short h)
{
	short msg[8], d;
	for (int i = 0; i < 10; i++)
	{
		short r = evnt_multi(MU_MESAG | MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 300, 0, &d, &d, &d, &d, &d, &d);
		if (!(r & MU_MESAG))
		{
			return 0;
		}
		if (msg[0] == WM_REDRAW && msg[3] == h)
		{
			return 1;
		}
	}
	return 0;
}

static void test_windows(void)
{
	short x, y, w, h, x2, y2, w2, h2;
	t_run("wind_get");
	CHECK_TRUE("wind_get", wind_get(0, WF_WORKXYWH, &x, &y, &w, &h) != 0 && w == a_desk.g_w && h > 100, "desktop work area %d,%d %dx%d", x, y, w, h);
	// The desktop's WF_CURRXYWH is the whole screen, but TOS 4.04 returns garbage for it.
	if (t_env.tos != 0x0404)
	{
		CHECK_TRUE("wind_get", wind_get(0, WF_CURRXYWH, &x2, &y2, &w2, &h2) != 0 && x2 == 0 && y2 == 0 && w2 == w && h2 > h,
			"desktop window %d,%d %dx%d", x2, y2, w2, h2);
	}

	GRECT wk = { (short)(a_desk.g_x + 40), (short)(a_desk.g_y + 40), (short)(a_desk.g_w / 2), (short)(a_desk.g_h / 2) };
	t_run("wind_calc");
	GRECT b;
	CHECK_TRUE("wind_calc", wind_calc(WC_BORDER, KIND, wk.g_x, wk.g_y, wk.g_w, wk.g_h, &b.g_x, &b.g_y, &b.g_w, &b.g_h) != 0
		&& b.g_x < wk.g_x && b.g_y < wk.g_y && b.g_w > wk.g_w && b.g_h > wk.g_h, "border %d,%d %dx%d around work %d,%d %dx%d",
		b.g_x, b.g_y, b.g_w, b.g_h, wk.g_x, wk.g_y, wk.g_w, wk.g_h);
	GRECT wk2;
	wind_calc(WC_WORK, KIND, b.g_x, b.g_y, b.g_w, b.g_h, &wk2.g_x, &wk2.g_y, &wk2.g_w, &wk2.g_h);
	CHECK_TRUE("wind_calc", memcmp(&wk, &wk2, sizeof(wk)) == 0, "WC_WORK of the border gives the work area back: %d,%d %dx%d",
		wk2.g_x, wk2.g_y, wk2.g_w, wk2.g_h);

	t_run("wind_create");
	short wh = wind_create(KIND, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	if (!CHECK_TRUE("wind_create", wh > 0, "wind_create() = %d", wh))
	{
		return;
	}
	t_run("wind_set_string");
	CHECK_TRUE("wind_set_string", wind_set_string(wh, WF_NAME, title) != 0, "wind_set_string(WF_NAME)");
	t_run("wind_set_ptr_int");
	CHECK_TRUE("wind_set_ptr_int", wind_set_ptr_int(wh, WF_INFO, info, 0) != 0, "wind_set_ptr_int(WF_INFO)");

	t_run("wind_open");
	a_drain_keys();
	CHECK_TRUE("wind_open", wind_open(wh, b.g_x, b.g_y, b.g_w, b.g_h) != 0, "wind_open()");
	CHECK_TRUE("wind_open", got_redraw(wh), "WM_REDRAW received for the new window");

	wind_get(wh, WF_CURRXYWH, &x, &y, &w, &h);
	CHECK_TRUE("wind_get", x == b.g_x && y == b.g_y && w == b.g_w && h == b.g_h, "WF_CURRXYWH %d,%d %dx%d", x, y, w, h);
	wind_get(wh, WF_WORKXYWH, &x, &y, &w, &h);
	CHECK_TRUE("wind_get", x == wk.g_x && y == wk.g_y && w == wk.g_w && h == wk.g_h, "WF_WORKXYWH %d,%d %dx%d", x, y, w, h);
	t_run("wind_get_grect");
	GRECT g;
	CHECK_TRUE("wind_get_grect", wind_get_grect(wh, WF_CURRXYWH, &g) != 0 && memcmp(&g, &b, sizeof(g)) == 0,
		"wind_get_grect(WF_CURRXYWH) %d,%d %dx%d", g.g_x, g.g_y, g.g_w, g.g_h);
	t_run("wind_get_int");
	short top = 0;
	CHECK_TRUE("wind_get_int", wind_get_int(0, WF_TOP, &top) != 0 && top == wh, "wind_get_int(WF_TOP) = %d", top);
	t_run("wind_get_ptr");
	void* scr = 0;
	CHECK_TRUE("wind_get_ptr", wind_get_ptr(0, WF_SCREEN, &scr) != 0 && scr != 0, "wind_get_ptr(WF_SCREEN) = %p", scr);

	t_run("wind_find");
	CHECK_EQ("wind_find", wind_find(wk.g_x + 10, wk.g_y + 10), wh);
	CHECK_EQ("wind_find", wind_find(a_desk.g_x + a_desk.g_w - 2, a_desk.g_y + a_desk.g_h - 2), 0);

	// Rectangle list: one rectangle, the whole work area (nothing covers it).
	short n = 0;
	int covered = 0;
	wind_get(wh, WF_FIRSTXYWH, &x, &y, &w, &h);
	while (w && h && n < 20)
	{
		covered += w * h;
		n++;
		wind_get(wh, WF_NEXTXYWH, &x, &y, &w, &h);
	}
	CHECK_TRUE("wind_get", n >= 1 && covered == wk.g_w * wk.g_h, "WF_FIRSTXYWH/WF_NEXTXYWH: %d rectangles covering %d of %d pixels",
		n, covered, wk.g_w * wk.g_h);

	t_run("wind_set");
	CHECK_TRUE("wind_set", wind_set(wh, WF_CURRXYWH, b.g_x + 16, b.g_y + 8, b.g_w - 32, b.g_h - 16) != 0, "wind_set(WF_CURRXYWH)");
	wind_get(wh, WF_CURRXYWH, &x, &y, &w, &h);
	CHECK_TRUE("wind_set", x == b.g_x + 16 && y == b.g_y + 8 && w == b.g_w - 32 && h == b.g_h - 16, "window moved to %d,%d %dx%d", x, y, w, h);
	wind_set(wh, WF_HSLIDE, 500, 0, 0, 0);
	wind_set(wh, WF_VSLSIZE, 300, 0, 0, 0);
	short hs = 0, vs = 0;
	wind_get(wh, WF_HSLIDE, &hs, &x, &x, &x);
	wind_get(wh, WF_VSLSIZE, &vs, &x, &x, &x);
	CHECK_TRUE("wind_set", hs == 500 && vs == 300, "WF_HSLIDE %d, WF_VSLSIZE %d", hs, vs);
	t_run("wind_set_grect");
	CHECK_TRUE("wind_set_grect", wind_set_grect(wh, WF_CURRXYWH, &b) != 0, "wind_set_grect(WF_CURRXYWH)");
	wind_get_grect(wh, WF_CURRXYWH, &g);
	CHECK_TRUE("wind_set_grect", memcmp(&g, &b, sizeof(g)) == 0, "window back at %d,%d %dx%d", g.g_x, g.g_y, g.g_w, g.g_h);
	t_run("wind_set_int");
	CHECK_TRUE("wind_set_int", wind_set_int(wh, WF_VSLIDE, 250) != 0, "wind_set_int(WF_VSLIDE)");
	wind_get_int(wh, WF_VSLIDE, &vs);
	CHECK_EQ("wind_set_int", vs, 250);

	// A second window on top, then top the first again.
	short wh2 = wind_create(NAME | MOVER, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	wind_set_string(wh2, WF_NAME, title);
	wind_open(wh2, b.g_x + 30, b.g_y + 30, b.g_w / 2, b.g_h / 2);
	got_redraw(wh2);
	wind_get_int(0, WF_TOP, &top);
	CHECK_EQ("wind_open", top, wh2);
	wind_set(wh, WF_TOP, 0, 0, 0, 0);
	wind_get_int(0, WF_TOP, &top);
	CHECK_EQ("wind_set", top, wh);

	t_run("wind_update");
	CHECK_TRUE("wind_update", wind_update(BEG_UPDATE) != 0 && wind_update(END_UPDATE) != 0, "BEG_UPDATE / END_UPDATE");
	CHECK_TRUE("wind_update", wind_update(BEG_MCTRL) != 0 && wind_update(END_MCTRL) != 0, "BEG_MCTRL / END_MCTRL");

	t_run("wind_close");
	CHECK_TRUE("wind_close", wind_close(wh2) != 0, "wind_close(second window)");
	t_run("wind_delete");
	CHECK_TRUE("wind_delete", wind_delete(wh2) != 0, "wind_delete(second window)");
	wind_get_int(0, WF_TOP, &top);
	CHECK_EQ("wind_close", top, wh);

	// wind_new closes and deletes all windows.
	t_run("wind_new");
	CHECK_TRUE("wind_new", wind_new() != 0, "wind_new()");
	short wh3 = wind_create(NAME, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	CHECK_TRUE("wind_new", wh3 > 0, "a window can be created after wind_new: %d", wh3);
	if (wh3 > 0)
	{
		wind_delete(wh3);
	}
	a_drain_keys();
}

int main(void)
{
	if (!a_begin("A_WIND"))
	{
		return t_end();
	}
	test_windows();
	return a_end();
}
