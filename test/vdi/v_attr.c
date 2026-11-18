/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Attribute functions and the attribute inquiries.
*/

#include "v_util.h"

#define CANARY 0x5aa5

// Simple attribute setter: one intin word in, intout[0] returned.
static void set1(const char* fn, short (*f)(short, short), short op, short val)
{
	t_run(fn);
	v_fake(1, 0);
	short r = f(FH, val);
	v_off();
	V_IN(fn, op, -1, 0, 1, W(val), 0);
	CHECK_EQ(fn, r, 0x100);
}

static void binding_tests(void)
{
	set1("vswr_mode", vswr_mode, 32, MD_XOR);
	set1("vsl_type", vsl_type, 15, USERLINE);
	set1("vsl_color", vsl_color, 17, 3);
	set1("vsm_type", vsm_type, 18, 4);
	set1("vsm_color", vsm_color, 20, 5);
	set1("vst_rotation", vst_rotation, 13, 900);
	set1("vst_font", vst_font, 21, 2);
	set1("vst_color", vst_color, 22, 6);
	set1("vst_effects", vst_effects, 106, 5);
	set1("vsf_interior", vsf_interior, 23, FIS_HATCH);
	set1("vsf_style", vsf_style, 24, 7);
	set1("vsf_color", vsf_color, 25, 8);
	set1("vsf_perimeter", vsf_perimeter, 104, 0);

	t_run("vs_color");
	short rgb[3] = { 100, 200, 300 };
	v_fake(0, 0);
	vs_color(FH, 5, rgb);
	v_off();
	V_IN("vs_color", 14, -1, 0, 4, W(5, 100, 200, 300), 0);

	t_run("vsl_udsty");
	v_fake(0, 0);
	vsl_udsty(FH, (short)0xf0f0);
	v_off();
	V_IN("vsl_udsty", 113, -1, 0, 1, W((short)0xf0f0), 0);

	// vsl_width: ptsin[0] = width, returns ptsout[0].
	t_run("vsl_width");
	v_fake(0, 2);
	short r = vsl_width(FH, 7);
	v_off();
	V_IN("vsl_width", 16, -1, 1, 0, 0, 0);
	CHECK_EQ("vsl_width", v_rec[0].ptsin[0], 7);
	CHECK_EQ("vsl_width", r, 0x200);

	t_run("vsl_ends");
	v_fake(0, 0);
	vsl_ends(FH, ARROWED, ROUND);
	v_off();
	V_IN("vsl_ends", 108, -1, 0, 2, W(ARROWED, ROUND), 0);

	// vsm_height: ptsin[1] = height (ptsin[0] = 0), returns ptsout[1].
	t_run("vsm_height");
	v_fake(0, 2);
	r = vsm_height(FH, 9);
	v_off();
	V_IN("vsm_height", 19, -1, 1, 0, 0, W(0, 9));
	CHECK_EQ("vsm_height", r, 0x201);

	// vst_height: ptsin[1] = height, ptsout[0..3] = char width, char height, cell width, cell height.
	t_run("vst_height");
	short cw = 0, ch = 0, clw = 0, clh = 0;
	v_fake(0, 4);
	vst_height(FH, 13, &cw, &ch, &clw, &clh);
	v_off();
	V_IN("vst_height", 12, -1, 1, 0, 0, W(0, 13));
	v_words("vst_height", "outputs", W(cw, ch, clw, clh), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vst_point");
	cw = ch = clw = clh = 0;
	v_fake(1, 4);
	r = vst_point(FH, 10, &cw, &ch, &clw, &clh);
	v_off();
	V_IN("vst_point", 107, -1, 0, 1, W(10), 0);
	CHECK_EQ("vst_point", r, 0x100);
	v_words("vst_point", "outputs", W(cw, ch, clw, clh), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vst_alignment");
	short ho = 0, vo = 0;
	v_fake(2, 0);
	vst_alignment(FH, 1, 2, &ho, &vo);
	v_off();
	V_IN("vst_alignment", 39, -1, 0, 2, W(1, 2), 0);
	v_words("vst_alignment", "outputs", W(ho, vo), W(0x100, 0x101), 2);

	// vsf_udpat: 16 words per plane.
	t_run("vsf_udpat");
	short pat[32];
	for (int i = 0; i < 32; i++)
	{
		pat[i] = (short)(0x1111 * (i & 7) + i);
	}
	v_fake(0, 0);
	vsf_udpat(FH, pat, 1);
	v_off();
	V_IN("vsf_udpat", 112, -1, 0, 16, pat, 0);
	v_fake(0, 0);
	vsf_udpat(FH, pat, 2);
	v_off();
	V_IN("vsf_udpat", 112, -1, 0, 32, pat, 0);

	// Inquiries. attrib arrays are 6 (line), 5 (marker), 5 (fill) and 10 (text) words.
	short at[12];
	t_run("vql_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_fake(5, 2);
	vql_attributes(FH, at);
	v_off();
	V_IN("vql_attributes", 35, -1, 0, 0, 0, 0);
	v_words("vql_attributes", "attrib", at, W(0x100, 0x101, 0x102, 0x103, 0x104, 0x200), 6);
	t_check(at[6] == CANARY, "vql_attributes", "attrib[6] = 0x%x, written past the 6 word array", (unsigned short)at[6]);

	t_run("vqm_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_fake(3, 2);
	vqm_attributes(FH, at);
	v_off();
	V_IN("vqm_attributes", 36, -1, 0, 0, 0, 0);
	v_words("vqm_attributes", "attrib", at, W(0x100, 0x101, 0x102, 0x200, 0x201), 5);
	t_check(at[5] == CANARY, "vqm_attributes", "attrib[5] = 0x%x, written past the 5 word array", (unsigned short)at[5]);

	t_run("vqf_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_fake(5, 0);
	vqf_attributes(FH, at);
	v_off();
	V_IN("vqf_attributes", 37, -1, 0, 0, 0, 0);
	v_words("vqf_attributes", "attrib", at, W(0x100, 0x101, 0x102, 0x103, 0x104), 5);
	t_check(at[5] == CANARY, "vqf_attributes", "attrib[5] = 0x%x, written past the 5 word array", (unsigned short)at[5]);

	t_run("vqt_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_fake(6, 4);
	vqt_attributes(FH, at);
	v_off();
	V_IN("vqt_attributes", 38, -1, 0, 0, 0, 0);
	v_words("vqt_attributes", "attrib", at, W(0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x200, 0x201, 0x202, 0x203), 10);
	t_check(at[10] == CANARY, "vqt_attributes", "attrib[10] = 0x%x, written past the 10 word array", (unsigned short)at[10]);
}

static void real_tests(void)
{
	short h = v_handle, at[12], r;

	t_run("vswr_mode");
	CHECK_EQ("vswr_mode", vswr_mode(h, MD_XOR), MD_XOR);

	// Lines.
	t_run("vsl_type");
	CHECK_EQ("vsl_type", vsl_type(h, DASH), DASH);
	t_run("vsl_color");
	CHECK_EQ("vsl_color", vsl_color(h, 1), 1);
	t_run("vsl_width");
	CHECK_EQ("vsl_width", vsl_width(h, 3), 3);
	t_run("vsl_ends");
	vsl_ends(h, ROUND, ARROWED);
	// The inquiries are checked against the words the VDI returned (EmuTOS returns fewer than TOS and NVDI),
	// then for the values set.
	t_run("vql_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_spy();
	vql_attributes(h, at);
	v_off();
	v_maps("vql_attributes", 0, "attrib[0..4]", at, W(0, 1, 2, 3, 4), 5, 1);
	v_maps("vql_attributes", 0, "attrib[5]", &at[5], W(0), 1, 0);
	t_check(at[6] == CANARY, "vql_attributes", "attrib[6] = 0x%x, written past the 6 word array", (unsigned short)at[6]);
	v_words("vql_attributes", "type, colour, mode", at, W(DASH, 1, MD_XOR), 3);
	CHECK_EQ("vql_attributes", at[5], 3);
	if (v_rec[0].nintout >= 5)
	{
		v_words("vql_attributes", "line ends", &at[3], W(ROUND, ARROWED), 2);
	}

	// Markers.
	t_run("vsm_type");
	CHECK_EQ("vsm_type", vsm_type(h, 3), 3);
	t_run("vsm_color");
	CHECK_EQ("vsm_color", vsm_color(h, 1), 1);
	t_run("vsm_height");
	short mh = vsm_height(h, 11);
	t_check(mh >= 9 && mh <= 11, "vsm_height", "vsm_height(11) = %d", mh);
	t_run("vqm_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_spy();
	vqm_attributes(h, at);
	v_off();
	v_maps("vqm_attributes", 0, "attrib[0..2]", at, W(0, 1, 2), 3, 1);
	v_maps("vqm_attributes", 0, "attrib[3..4]", &at[3], W(0, 1), 2, 0);
	t_check(at[5] == CANARY, "vqm_attributes", "attrib[5] = 0x%x, written past the 5 word array", (unsigned short)at[5]);
	if (!v_nvdi)
	{
		// The ROM VDIs (TOS and EmuTOS) return the marker type - 1 and width 0.
		t_check(at[0] == 3 - 1 && at[4] == mh, "vqm_attributes", "ROM VDI: type %d (set 3) height %d", at[0], at[4]);
	}
	else
	{
		v_words("vqm_attributes", "type, colour, mode", at, W(3, 1, MD_XOR), 3);
		t_check(at[4] == mh && at[3] > 0, "vqm_attributes", "marker width %d height %d, expected height %d", at[3], at[4], mh);
	}

	// Fill.
	t_run("vsf_interior");
	CHECK_EQ("vsf_interior", vsf_interior(h, FIS_PATTERN), FIS_PATTERN);
	t_run("vsf_style");
	CHECK_EQ("vsf_style", vsf_style(h, 4), 4);
	t_run("vsf_color");
	CHECK_EQ("vsf_color", vsf_color(h, 1), 1);
	t_run("vsf_perimeter");
	CHECK_EQ("vsf_perimeter", vsf_perimeter(h, 0), 0);
	t_run("vqf_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_spy();
	vqf_attributes(h, at);
	v_off();
	v_maps("vqf_attributes", 0, "attrib", at, W(0, 1, 2, 3, 4), 5, 1);
	v_words("vqf_attributes", "attrib", at, W(FIS_PATTERN, 1, 4, MD_XOR, 0), v_rec[0].nintout < 5 ? v_rec[0].nintout : 5);
	t_check(at[5] == CANARY, "vqf_attributes", "attrib[5] = 0x%x, written past the 5 word array", (unsigned short)at[5]);

	// Text.
	t_run("vst_font");
	CHECK_EQ("vst_font", vst_font(h, 1), 1);
	t_run("vst_color");
	CHECK_EQ("vst_color", vst_color(h, 1), 1);
	t_run("vst_rotation");
	CHECK_EQ("vst_rotation", vst_rotation(h, 900), 900);
	t_run("vst_effects");
	CHECK_EQ("vst_effects", vst_effects(h, 1), 1);
	t_run("vst_alignment");
	short ho = -1, vo = -1;
	vst_alignment(h, 1, 3, &ho, &vo);
	v_words("vst_alignment", "realized", W(ho, vo), W(1, 3), 2);
	t_run("vst_height");
	short cw = 0, ch = 0, clw = 0, clh = 0;
	vst_height(h, 6, &cw, &ch, &clw, &clh);
	t_check(ch == 6 && cw > 0 && clw >= cw && clh > ch, "vst_height", "vst_height(6): char %dx%d cell %dx%d", cw, ch, clw, clh);
	t_run("vqt_attributes");
	for (int i = 0; i < 12; i++) at[i] = CANARY;
	v_spy();
	vqt_attributes(h, at);
	v_off();
	v_maps("vqt_attributes", 0, "attrib[0..5]", at, W(0, 1, 2, 3, 4, 5), 6, 1);
	v_maps("vqt_attributes", 0, "attrib[6..9]", &at[6], W(0, 1, 2, 3), 4, 0);
	// TOS (not EmuTOS) returns the writing mode - 1.
	short tmode = (!v_nvdi && !t_env.emutos) ? MD_XOR - 1 : MD_XOR;
	v_words("vqt_attributes", "attrib", at, W(1, 1, 900, 1, 3, tmode, cw, ch, clw, clh), 10);
	t_check(at[10] == CANARY, "vqt_attributes", "attrib[10] = 0x%x, written past the 10 word array", (unsigned short)at[10]);
	t_run("vst_point");
	short cw2 = 0, ch2 = 0, clw2 = 0, clh2 = 0;
	r = vst_point(h, 10, &cw2, &ch2, &clw2, &clh2);
	t_check(r > 0 && r <= 10 && ch2 > 0 && clh2 > ch2, "vst_point", "vst_point(10) = %d: char %dx%d cell %dx%d", r, cw2, ch2, clw2, clh2);
	vst_rotation(h, 0);
	vst_effects(h, 0);

	// Palette: change a colour and read it back (vq_color with set_flag 0 returns the requested values).
	if (v_work_out[13] >= 4)
	{
		short idx = (short)(v_work_out[13] > 8 ? 8 : 2);
		short old[3], got[3] = { -1, -1, -1 };
		vq_color(h, idx, 0, old);
		t_run("vs_color");
		vs_color(h, idx, W(1000, 500, 0));
		t_run("vq_color");
		r = vq_color(h, idx, 0, got);
		CHECK_EQ("vq_color", r, idx);
		// The ST palette has 8 or 16 levels per gun, allow for rounding.
		t_check(got[0] >= 930 && got[1] >= 430 && got[1] <= 570 && got[2] <= 70, "vs_color", "colour %d = %d,%d,%d, set 1000,500,0",
			idx, got[0], got[1], got[2]);
		vs_color(h, idx, old);
	}
	else
	{
		t_skip("vs_color", "monochrome");
		t_skip("vq_color", "monochrome");
	}

	v_scr_begin();
	short x = v_ox, y = v_oy;

	// User defined line style: 0xff00 on a 32 pixel line gives 16 pixels.
	v_clear(x, y, 40, 4);
	t_run("vsl_udsty");
	vsl_type(h, USERLINE);
	vsl_udsty(h, (short)0xff00);
	short line[4] = { x, (short)(y + 1), (short)(x + 31), (short)(y + 1) };
	v_pline(h, 2, line);
	vsl_type(h, SOLID);
	int n = v_count_ink(x, y, 40, 4);
	CHECK_EQ("vsl_udsty", n, 16);

	// User defined fill pattern: every other line set (the pattern is aligned to the screen).
	t_run("vsf_udpat");
	short pat[16];
	for (int i = 0; i < 16; i++)
	{
		pat[i] = (i & 1) ? 0 : (short)0xffff;
	}
	v_clear(x, y, 32, 16);
	vsf_udpat(h, pat, 1);
	vsf_interior(h, FIS_USER);
	short box[4] = { x, y, (short)(x + 31), (short)(y + 15) };
	vr_recfl(h, box);
	int even = 0, odd = 0;
	for (short j = y; j < y + 16; j++)
	{
		if (j & 1)
		{
			odd += v_count_ink(x, j, 32, 1);
		}
		else
		{
			even += v_count_ink(x, j, 32, 1);
		}
	}
	t_check(even == 256 && odd == 0, "vsf_udpat", "user pattern: %d pixels on even lines (expected 256), %d on odd (expected 0)", even, odd);
	v_scr_end();
}

int main(void)
{
	if (v_begin("V_ATTR"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
