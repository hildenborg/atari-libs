/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Inquire functions (the attribute inquiries are in v_attr.c).
*/

#include <string.h>
#include "v_util.h"

#define CANARY 0x5aa5

static void binding_tests(void)
{
	short wo[60];

	t_run("vq_extnd");
	wo[57] = CANARY;
	v_fake(45, 12);
	vq_extnd(FH, 1, wo);
	v_off();
	V_IN("vq_extnd", 102, -1, 0, 1, W(1), 0);
	short want[57];
	for (int i = 0; i < 45; i++) want[i] = 0x100 + i;
	for (int i = 0; i < 12; i++) want[45 + i] = 0x200 + i;
	v_words("vq_extnd", "work_out", wo, want, 57);
	CHECK_EQ("vq_extnd", wo[57], CANARY);

	t_run("vq_color");
	short rgb[4] = { 0, 0, 0, CANARY };
	v_fake(4, 0);
	short r = vq_color(FH, 3, 1, rgb);
	v_off();
	V_IN("vq_color", 26, -1, 0, 2, W(3, 1), 0);
	CHECK_EQ("vq_color", r, 0x100);
	v_words("vq_color", "rgb", rgb, W(0x101, 0x102, 0x103, CANARY), 4);

	t_run("vqt_extent");
	short ext[9];
	ext[8] = CANARY;
	v_fake(0, 8);
	vqt_extent(FH, "Ab\x84", ext);
	v_off();
	V_IN("vqt_extent", 116, -1, 0, 3, W('A', 'b', 0x84), 0);
	v_words("vqt_extent", "extent", ext, W(0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, CANARY), 9);

	// vqt_width: returns intout[0], cell width = ptsout[0], left delta = ptsout[2], right delta = ptsout[4].
	// Each output is a single short (canaries between them).
	t_run("vqt_width");
	struct { short cw, c1, ld, c2, rd, c3; } w = { 0, CANARY, 0, CANARY, 0, CANARY };
	v_fake(1, 6);
	r = vqt_width(FH, 'W', &w.cw, &w.ld, &w.rd);
	v_off();
	V_IN("vqt_width", 117, -1, 0, 1, W('W'), 0);
	CHECK_EQ("vqt_width", r, 0x100);
	v_words("vqt_width", "outputs", &w.cw, W(0x200, CANARY, 0x202, CANARY, 0x204, CANARY), 6);

	t_run("vqt_name");
	char name[40];
	memset(name, 'X', sizeof(name));
	v_fake(33, 0);
	for (int i = 0; i < 32; i++) v_fake_intout[1 + i] = 'a' + (i % 26);
	v_fake_intout[1 + 20] = 0;
	r = vqt_name(FH, 3, name);
	v_off();
	V_IN("vqt_name", 130, -1, 0, 1, W(3), 0);
	CHECK_EQ("vqt_name", r, 0x100);
	t_check(memcmp(name, "abcdefghijklmnopqrst", 21) == 0, "vqt_name", "name \"%.32s\"", name);

	// vq_cellarray: contrl[7] = row length, contrl[8] = rows; returns elements used, rows used, status in
	// contrl[9..11] and the colour indices in intout.
	t_run("vq_cellarray");
	short xy[4] = { 1, 2, 3, 4 }, col[9], el = 0, rows = 0, st = 0;
	col[8] = CANARY;
	v_fake(8, 0);
	v_fake_contrl[9] = 3;
	v_fake_contrl[10] = 2;
	v_fake_contrl[11] = 1;
	v_fake_cmask = (1 << 9) | (1 << 10) | (1 << 11);
	vq_cellarray(FH, xy, 4, 2, &el, &rows, &st, col);
	v_off();
	V_IN("vq_cellarray", 27, -1, 2, 0, 0, xy);
	v_words("vq_cellarray", "contrl[7..8]", &v_rec[0].contrl[7], W(4, 2), 2);
	v_words("vq_cellarray", "el_used, rows_used, status", W(el, rows, st), W(3, 2, 1), 3);
	v_words("vq_cellarray", "colarray", col, W(0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, CANARY), 9);

	t_run("vqin_mode");
	short m = 0;
	v_fake(1, 0);
	vqin_mode(FH, 2, &m);
	v_off();
	V_IN("vqin_mode", 115, -1, 0, 1, W(2), 0);
	CHECK_EQ("vqin_mode", m, 0x100);

	// vqt_fontinfo: intout[0..1] = first, last; ptsout[0] = max width, distances in ptsout[1,3,5,7,9],
	// effects in ptsout[2,4,6].
	t_run("vqt_fontinfo");
	short first = 0, last = 0, dist[6], width = 0, eff[4];
	dist[5] = CANARY;
	eff[3] = CANARY;
	v_fake(2, 10);
	vqt_fontinfo(FH, &first, &last, dist, &width, eff);
	v_off();
	V_IN("vqt_fontinfo", 131, -1, 0, 0, 0, 0);
	v_words("vqt_fontinfo", "first, last, width", W(first, last, width), W(0x100, 0x101, 0x200), 3);
	v_words("vqt_fontinfo", "distances", dist, W(0x201, 0x203, 0x205, 0x207, 0x209, CANARY), 6);
	v_words("vqt_fontinfo", "effects", eff, W(0x202, 0x204, 0x206, CANARY), 4);
}

static void real_tests(void)
{
	short h = v_handle, wo[60], r;

	t_run("vq_extnd");
	wo[57] = CANARY;
	vq_extnd(h, 0, wo);
	v_words("vq_extnd", "work_out (same as v_opnvwk)", wo, v_work_out, 57);
	CHECK_EQ("vq_extnd", wo[57], CANARY);
	vq_extnd(h, 1, wo);
	CHECK_EQ("vq_extnd", wo[4], v_scr_planes);

	t_run("vq_color");
	short rgb[3];
	r = vq_color(h, 0, 1, rgb);
	t_check(r == 0 && rgb[0] >= 900 && rgb[1] >= 900 && rgb[2] >= 900, "vq_color", "colour 0 = %d: %d,%d,%d (white)", r, rgb[0], rgb[1], rgb[2]);
	r = vq_color(h, 1, 1, rgb);
	t_check(r == 1 && rgb[0] <= 100 && rgb[1] <= 100 && rgb[2] <= 100, "vq_color", "colour 1 = %d: %d,%d,%d (black)", r, rgb[0], rgb[1], rgb[2]);

	// The default font is the system font, with the AES character cell size.
	t_run("vqt_extent");
	short ext[8];
	vqt_extent(h, "Hello", ext);
	t_check(ext[0] == 0 && ext[1] == 0 && ext[2] == 5 * a_wchar && ext[3] == 0 && ext[4] == 5 * a_wchar && ext[5] == a_hchar && ext[6] == 0 && ext[7] == a_hchar,
		"vqt_extent", "\"Hello\": %d,%d %d,%d %d,%d %d,%d (cell %dx%d)",
		ext[0], ext[1], ext[2], ext[3], ext[4], ext[5], ext[6], ext[7], a_wchar, a_hchar);

	t_run("vqt_width");
	struct { short cw, c1, ld, c2, rd, c3; } w = { 0, CANARY, 0, CANARY, 0, CANARY };
	v_spy();
	r = vqt_width(h, 'W', &w.cw, &w.ld, &w.rd);
	v_off();
	v_maps("vqt_width", 0, "cell width, deltas", W(w.cw, w.ld, w.rd), W(0, 2, 4), 3, 0);
	t_check(w.c1 == CANARY && w.c2 == CANARY && w.c3 == CANARY, "vqt_width", "words after the outputs: %x %x %x, expected %x",
		(unsigned short)w.c1, (unsigned short)w.c2, (unsigned short)w.c3, CANARY);
	t_check(r == 'W' && w.cw == a_wchar, "vqt_width", "'W' = %d: cell width %d (expected %d)", r, w.cw, a_wchar);

	t_run("vqt_name");
	char name[40];
	memset(name, 'X', sizeof(name));
	v_spy_lim(34, -1);
	r = vqt_name(h, 1, name);
	v_off();
	v_out_fits("vqt_name", 0, 34, 16);
	short got[32];
	for (int i = 0; i < 32; i++) got[i] = (unsigned char)name[i];
	short ix[32];
	for (int i = 0; i < 32; i++) ix[i] = 1 + i;
	v_maps("vqt_name", 0, "name", got, ix, 32, 1);
	t_check(r == 1 && name[32] == 0, "vqt_name", "font 1: id %d \"%.32s\"", r, name);

	t_run("vqt_fontinfo");
	short first = -1, last = -1, dist[5], width = -1, eff[3];
	v_spy();
	vqt_fontinfo(h, &first, &last, dist, &width, eff);
	v_off();
	v_maps("vqt_fontinfo", 0, "first, last", W(first, last), W(0, 1), 2, 1);
	v_maps("vqt_fontinfo", 0, "width, distances, effects", W(width, dist[0], dist[1], dist[2], dist[3], dist[4], eff[0], eff[1], eff[2]),
		W(0, 1, 3, 5, 7, 9, 2, 4, 6), 9, 0);
	t_check(first >= 0 && first <= 32 && last == 255 && width == a_wchar && dist[4] >= dist[3] && dist[3] > dist[2] && dist[0] >= dist[1],
		"vqt_fontinfo", "chars %d..%d width %d distances %d %d %d %d %d effects %d %d %d", first, last, width,
		dist[0], dist[1], dist[2], dist[3], dist[4], eff[0], eff[1], eff[2]);

	// All fonts (with GDOS after loading them): vqt_name must have room for what the VDI returns.
	if (v_gdos != GDOS_NONE)
	{
		short n = vst_load_fonts(h, 0);
		int fits = 1;
		for (short i = 1; i <= 1 + n && fits; i++)
		{
			v_spy_lim(34, -1);
			r = vqt_name(h, i, name);
			v_off();
			if (v_rec[0].nintout > 34)
			{
				fits = v_out_fits("vqt_name", 0, 34, 16);
				t_print("INFO font %d id %d \"%.32s\" intout[33] = %d", i, r, name, v_rec[0].intout_val[33]);
			}
		}
		if (fits)
		{
			t_check(1, "vqt_name", "%d fonts, at most 34 intout words", 1 + n);
		}
		vst_unload_fonts(h, 0);
	}

	// Cell array of a black/white area. Not all drivers implement it.
	// The ROM VDIs (TOS 2.06, EmuTOS) overwrite memory in vq_cellarray, also when called directly.
	if (!v_nvdi)
	{
		t_skip("vq_cellarray", "the ROM VDI corrupts memory in vq_cellarray");
		return;
	}
	v_scr_begin();
	short x = v_ox, y = v_oy;
	v_clear(x, y, 8, 4);
	vr_recfl(h, W(x, y, x + 3, y + 3));
	t_run("vq_cellarray");
	short col[16], el = -1, rows = -1, st = -1;
	for (int i = 0; i < 16; i++) col[i] = -1;
	// Ask the VDI directly first: the binding hands it a 16 word dummy array for intout.
	static V_RAW raw;
	memset(&raw, 0, sizeof(raw));
	raw.contrl[0] = 27;
	raw.contrl[1] = 2;
	raw.contrl[6] = h;
	raw.contrl[7] = 8;
	raw.contrl[8] = 4;
	memcpy(raw.ptsin, W(x, y + 3, x + 7, y), 8);
	for (int i = 0; i < 300; i++) raw.intout[i] = 0x7e7e;
	v_raw(&raw);
	int written = 0;
	for (int i = 0; i < 300; i++)
	{
		if (raw.intout[i] != 0x7e7e) written = i + 1;
	}
	if (written > 16)
	{
		t_check(0, "vq_cellarray", "the VDI writes %d intout words (contrl[4] = %d), more than the binding's 16 word dummy array, not called",
			written, raw.contrl[4]);
		v_scr_end();
		return;
	}
	if (raw.contrl[9] <= 0)
	{
		t_skip("vq_cellarray", "not implemented by the driver (direct call: elements %d rows %d, %d intout words)", raw.contrl[9], raw.contrl[10], written);
		v_scr_end();
		return;
	}
	vq_cellarray(h, W(x, y + 3, x + 7, y), 8, 4, &el, &rows, &st, col);
	if (0)
	{
	}
	else
	{
		t_check(el == 8 && rows == 4 && col[0] == 1 && col[4] == 0, "vq_cellarray", "elements %d rows %d status %d, row 0: %d %d %d %d %d %d %d %d",
			el, rows, st, col[0], col[1], col[2], col[3], col[4], col[5], col[6], col[7]);
	}
	v_scr_end();
}

int main(void)
{
	if (v_begin("V_INQ"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
