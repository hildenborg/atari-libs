/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Output functions.
*/

#include "v_util.h"

static void binding_tests(void)
{
	short xy[8] = { 1, 2, 30, 40, 50, 6, 7, 80 };

	t_run("v_pline");
	v_fake(0, 0);
	v_pline(FH, 3, xy);
	v_off();
	V_IN("v_pline", 6, -1, 3, 0, 0, xy);

	t_run("v_pmarker");
	v_fake(0, 0);
	v_pmarker(FH, 4, xy);
	v_off();
	V_IN("v_pmarker", 7, -1, 4, 0, 0, xy);

	t_run("v_fillarea");
	v_fake(0, 0);
	v_fillarea(FH, 4, xy);
	v_off();
	V_IN("v_fillarea", 9, -1, 4, 0, 0, xy);

	// Characters are 0..255 in intin, 0x84 must not become 0xff84.
	t_run("v_gtext");
	v_fake(0, 0);
	v_gtext(FH, 10, 20, "Ab\x84");
	v_off();
	V_IN("v_gtext", 8, -1, 1, 3, W('A', 'b', 0x84), W(10, 20));

	// v_cellarray: contrl[3] = row_length * num_rows, contrl[7..10] = row_length, elements used, rows, mode.
	t_run("v_cellarray");
	short col[8] = { 1, 2, 3, -1, 4, 5, 6, -1 };
	v_fake(0, 0);
	v_cellarray(FH, xy, 4, 3, 2, MD_TRANS, col);
	v_off();
	V_IN("v_cellarray", 10, -1, 2, 8, col, xy);
	v_words("v_cellarray", "contrl[7..10]", &v_rec[0].contrl[7], W(4, 3, 2, MD_TRANS), 4);

	t_run("v_contourfill");
	v_fake(0, 0);
	v_contourfill(FH, 5, 6, 7);
	v_off();
	V_IN("v_contourfill", 103, -1, 1, 1, W(7), W(5, 6));

	t_run("vr_recfl");
	v_fake(0, 0);
	vr_recfl(FH, xy);
	v_off();
	V_IN("vr_recfl", 114, -1, 2, 0, 0, xy);

	t_run("v_bar");
	v_fake(0, 0);
	v_bar(FH, xy);
	v_off();
	V_IN("v_bar", 11, 1, 2, 0, 0, xy);

	// Arcs: ptsin = x, y, 0, 0, 0, 0, radius, 0; intin = start and end angle.
	t_run("v_arc");
	v_fake(0, 0);
	v_arc(FH, 100, 50, 30, 450, 1800);
	v_off();
	V_IN("v_arc", 11, 2, 4, 2, W(450, 1800), W(100, 50, 0, 0, 0, 0, 30, 0));

	t_run("v_pieslice");
	v_fake(0, 0);
	v_pieslice(FH, 101, 51, 31, 451, 1801);
	v_off();
	V_IN("v_pieslice", 11, 3, 4, 2, W(451, 1801), W(101, 51, 0, 0, 0, 0, 31, 0));

	// v_circle: ptsin = x, y, 0, 0, radius, 0.
	t_run("v_circle");
	v_fake(0, 0);
	v_circle(FH, 102, 52, 32);
	v_off();
	V_IN("v_circle", 11, 4, 3, 0, 0, W(102, 52, 0, 0, 32, 0));

	t_run("v_ellipse");
	v_fake(0, 0);
	v_ellipse(FH, 103, 53, 33, 23);
	v_off();
	V_IN("v_ellipse", 11, 5, 2, 0, 0, W(103, 53, 33, 23));

	t_run("v_ellarc");
	v_fake(0, 0);
	v_ellarc(FH, 104, 54, 34, 24, 100, 2000);
	v_off();
	V_IN("v_ellarc", 11, 6, 2, 2, W(100, 2000), W(104, 54, 34, 24));

	t_run("v_ellpie");
	v_fake(0, 0);
	v_ellpie(FH, 105, 55, 35, 25, 101, 2001);
	v_off();
	V_IN("v_ellpie", 11, 7, 2, 2, W(101, 2001), W(105, 55, 35, 25));

	t_run("v_rbox");
	v_fake(0, 0);
	v_rbox(FH, xy);
	v_off();
	V_IN("v_rbox", 11, 8, 2, 0, 0, xy);

	t_run("v_rfbox");
	v_fake(0, 0);
	v_rfbox(FH, xy);
	v_off();
	V_IN("v_rfbox", 11, 9, 2, 0, 0, xy);

	// v_justified: ptsin = x, y, length, 0; intin = word space, char space, characters.
	t_run("v_justified");
	v_fake(0, 0);
	v_justified(FH, 10, 20, "A b\x84", 100, 1, 0);
	v_off();
	V_IN("v_justified", 11, 10, 2, 6, W(1, 0, 'A', ' ', 'b', 0x84), W(10, 20, 100, 0));
}

// Ink in the four quadrants around (cx, cy), r pixels out. Order: upper right, upper left, lower left, lower right.
static void quadrants(short cx, short cy, short r, int* q)
{
	q[0] = v_count_ink(cx + 1, cy - r, r, r);
	q[1] = v_count_ink(cx - r, cy - r, r, r);
	q[2] = v_count_ink(cx - r, cy + 1, r, r);
	q[3] = v_count_ink(cx + 1, cy + 1, r, r);
}

static void real_tests(void)
{
	short h = v_handle;
	short x = v_ox, y = v_oy;
	int n, q[4];

	v_scr_begin();

	t_run("v_pline");
	v_clear(x, y, 40, 10);
	v_pline(h, 2, W(x + 5, y + 3, x + 24, y + 3));
	n = v_count_ink(x, y, 40, 10);
	t_check(n == 20 && v_ink(x + 5, y + 3) && v_ink(x + 24, y + 3), "v_pline", "20 pixel line: %d pixels", n);

	t_run("v_pmarker");
	v_clear(x, y, 40, 10);
	vsm_type(h, 1);
	v_pmarker(h, 2, W(x + 5, y + 3, x + 20, y + 6));
	n = v_count_ink(x, y, 40, 10);
	t_check(n == 2 && v_ink(x + 5, y + 3) && v_ink(x + 20, y + 6), "v_pmarker", "2 dot markers: %d pixels", n);

	t_run("v_fillarea");
	v_clear(x, y, 40, 20);
	v_fillarea(h, 4, W(x + 5, y + 2, x + 14, y + 2, x + 14, y + 11, x + 5, y + 11));
	n = v_count_ink(x, y, 40, 20);
	t_check(n == 100, "v_fillarea", "10x10 polygon: %d pixels", n);

	t_run("vr_recfl");
	v_clear(x, y, 40, 20);
	vr_recfl(h, W(x + 5, y + 2, x + 14, y + 11));
	n = v_count_ink(x, y, 40, 20);
	CHECK_EQ("vr_recfl", n, 100);

	t_run("v_bar");
	v_clear(x, y, 40, 20);
	v_bar(h, W(x + 5, y + 2, x + 24, y + 11));
	n = v_count_ink(x, y, 40, 20);
	CHECK_EQ("v_bar", n, 200);

	// Hollow bar: only the perimeter.
	vsf_interior(h, FIS_HOLLOW);
	v_clear(x, y, 40, 20);
	v_bar(h, W(x + 5, y + 2, x + 24, y + 11));
	n = v_count_ink(x, y, 40, 20);
	t_check(n == 56 && !v_ink(x + 15, y + 6), "v_bar", "hollow 20x10 bar: %d pixels (expected 56)", n);
	vsf_interior(h, FIS_SOLID);

	// Round shapes, centre (cx, cy). Circles are corrected for the pixel aspect ratio, so only rough checks.
	short cx = x + 30, cy = y + 25;
	t_run("v_circle");
	v_clear(x, y, 60, 50);
	v_circle(h, cx, cy, 12);
	quadrants(cx, cy, 16, q);
	t_check(v_ink(cx, cy) && v_ink(cx + 11, cy) && v_ink(cx - 11, cy) && !v_ink(cx + 14, cy) && q[0] > 60 && q[1] > 60 && q[2] > 60 && q[3] > 60,
		"v_circle", "filled circle r 12: quadrants %d %d %d %d", q[0], q[1], q[2], q[3]);

	t_run("v_ellipse");
	v_clear(x, y, 60, 50);
	v_ellipse(h, cx, cy, 15, 5);
	t_check(v_ink(cx, cy) && v_ink(cx + 14, cy) && !v_ink(cx + 17, cy) && v_ink(cx, cy + 4) && !v_ink(cx, cy + 7),
		"v_ellipse", "filled ellipse 15x5");

	// Arcs from 0 to 90 degrees: upper right quadrant only.
	t_run("v_arc");
	v_clear(x, y, 60, 50);
	v_arc(h, cx, cy, 12, 0, 900);
	quadrants(cx, cy, 16, q);
	t_check(q[0] > 5 && q[1] == 0 && q[2] == 0 && q[3] == 0 && !v_ink(cx + 3, cy - 3), "v_arc", "0-90 degree arc: quadrants %d %d %d %d",
		q[0], q[1], q[2], q[3]);

	t_run("v_pieslice");
	v_clear(x, y, 60, 50);
	v_pieslice(h, cx, cy, 12, 900, 1800);
	quadrants(cx, cy, 16, q);
	t_check(q[1] > 40 && q[0] <= 1 && q[2] <= 1 && q[3] == 0 && v_ink(cx - 3, cy - 3), "v_pieslice", "90-180 degree pie: quadrants %d %d %d %d",
		q[0], q[1], q[2], q[3]);

	t_run("v_ellarc");
	v_clear(x, y, 60, 50);
	v_ellarc(h, cx, cy, 15, 8, 1800, 2700);
	quadrants(cx, cy, 18, q);
	t_check(q[2] > 5 && q[0] == 0 && q[1] == 0 && q[3] == 0, "v_ellarc", "180-270 degree arc: quadrants %d %d %d %d", q[0], q[1], q[2], q[3]);

	t_run("v_ellpie");
	v_clear(x, y, 60, 50);
	v_ellpie(h, cx, cy, 15, 8, 2700, 3600);
	quadrants(cx, cy, 18, q);
	t_check(q[3] > 40 && q[0] <= 1 && q[1] == 0 && q[2] <= 1 && v_ink(cx + 3, cy + 3), "v_ellpie", "270-360 degree pie: quadrants %d %d %d %d",
		q[0], q[1], q[2], q[3]);

	// Rounded boxes: the corner pixel is not set.
	short rb[4] = { (short)(x + 5), (short)(y + 5), (short)(x + 54), (short)(y + 34) };
	t_run("v_rbox");
	v_clear(x, y, 60, 50);
	v_rbox(h, rb);
	t_check(!v_ink(x + 5, y + 5) && v_ink(x + 30, y + 5) && v_ink(x + 5, y + 20) && !v_ink(x + 30, y + 20) && !v_ink(x + 30, y + 3),
		"v_rbox", "rounded box outline");
	t_run("v_rfbox");
	v_clear(x, y, 60, 50);
	v_rfbox(h, rb);
	t_check(!v_ink(x + 5, y + 5) && v_ink(x + 30, y + 5) && v_ink(x + 30, y + 20) && !v_ink(x + 30, y + 36),
		"v_rfbox", "filled rounded box");

	// Contour fill inside a closed outline (index -1: fill the seed colour).
	t_run("v_contourfill");
	v_clear(x, y, 60, 50);
	v_pline(h, 5, W(x + 5, y + 5, x + 24, y + 5, x + 24, y + 24, x + 5, y + 24, x + 5, y + 5));
	v_contourfill(h, x + 10, y + 10, -1);
	n = v_count_ink(x + 5, y + 5, 20, 20);
	int out = v_count_ink(x, y, 60, 50) - n;
	t_check(n == 400 && out == 0, "v_contourfill", "filled outline: %d of 400 pixels, %d outside", n, out);

	// Text, top aligned so the cell starts at y.
	short d;
	vst_alignment(h, 0, 5, &d, &d);
	short ext[8];
	vqt_extent(h, "Hi", ext);
	short tw = ext[2] - ext[0], th = ext[5] - ext[3];
	t_run("v_gtext");
	v_clear(x, y, 100, 30);
	v_gtext(h, x + 4, y + 4, "Hi");
	n = v_count_ink(x + 4, y + 4, tw, th);
	int all = v_count_ink(x, y, 100, 30);
	t_check(n > 10 && n == all, "v_gtext", "\"Hi\" %dx%d: %d pixels in the text cell, %d in total", tw, th, n, all);

	// Justified text, stretched to 90 pixels: there is ink near the end.
	t_run("v_justified");
	v_clear(x, y, 100, 30);
	v_justified(h, x + 4, y + 4, "A b", 90, 1, 1);
	int end = v_count_ink(x + 74, y + 4, 20, th);
	int start = v_count_ink(x + 4, y + 4, 10, th);
	t_check(end > 3 && start > 3, "v_justified", "\"A b\" stretched to 90: %d pixels at the start, %d at the end", start, end);

	// Cell array: 2x2 cells of black/white in a 20x20 rectangle. Not all drivers implement it.
	t_run("v_cellarray");
	v_clear(x, y, 40, 30);
	short cells[4] = { 1, 0, 0, 1 };
	v_cellarray(h, W(x + 4, y + 23, x + 23, y + 4), 2, 2, 2, MD_REPLACE, cells);
	n = v_count_ink(x, y, 40, 30);
	if (n == 0)
	{
		t_skip("v_cellarray", "not implemented by the driver");
	}
	else
	{
		int c[4] = { v_count_ink(x + 4, y + 4, 10, 10), v_count_ink(x + 14, y + 4, 10, 10),
			v_count_ink(x + 4, y + 14, 10, 10), v_count_ink(x + 14, y + 14, 10, 10) };
		t_check(n == 200 && c[0] + c[3] == 200, "v_cellarray", "cells: %d %d %d %d, total %d", c[0], c[1], c[2], c[3], n);
	}

	v_scr_end();
}

int main(void)
{
	if (v_begin("V_OUT"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
