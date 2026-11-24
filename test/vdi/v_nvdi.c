/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	NVDI extensions: colour spaces and tables, offscreen bitmaps, extended font functions.
	Binding tests everywhere, real tests where NVDI is installed.
*/

#include <string.h>
#include "v_util.h"

#define CANARY 0x5aa5

static long ptr_at(const short* w)
{
	return ((long)(unsigned short)w[0] << 16) | (unsigned short)w[1];
}

static const char* set_names[10] =
{
	"vst_fg_color", "vsf_fg_color", "vsl_fg_color", "vsm_fg_color", "vsr_fg_color",
	"vst_bg_color", "vsf_bg_color", "vsl_bg_color", "vsm_bg_color", "vsr_bg_color"
};
static short (*set_fns[10])(short, int, COLOR_ENTRY*) =
{
	vst_fg_color, vsf_fg_color, vsl_fg_color, vsm_fg_color, vsr_fg_color,
	vst_bg_color, vsf_bg_color, vsl_bg_color, vsm_bg_color, vsr_bg_color
};
static const char* get_names[10] =
{
	"vqt_fg_color", "vqf_fg_color", "vql_fg_color", "vqm_fg_color", "vqr_fg_color",
	"vqt_bg_color", "vqf_bg_color", "vql_bg_color", "vqm_bg_color", "vqr_bg_color"
};
static int (*get_fns[10])(short, COLOR_ENTRY*) =
{
	vqt_fg_color, vqf_fg_color, vql_fg_color, vqm_fg_color, vqr_fg_color,
	vqt_bg_color, vqf_bg_color, vql_bg_color, vqm_bg_color, vqr_bg_color
};

// The colour space as 2 words, then the 4 words of a colour entry.
#define CS_WORDS(cs) (short)((cs) >> 16), (short)(cs)

static void binding_tests(void)
{
	short r;
	long l;
	COLOR_ENTRY ce = { .rgb = { 0, 0x1111, 0x2222, 0x3333 } };
	struct { COLOR_ENTRY c; short canary; } out;

	// Colour setters: intin = colour space (long), colour entry; returns intout[0].
	for (int i = 0; i < 10; i++)
	{
		t_run(set_names[i]);
		v_fake(1, 0);
		r = set_fns[i](FH, CSPACE_RGB, &ce);
		v_off();
		V_IN(set_names[i], i < 5 ? 200 : 201, i % 5, 0, 6, W(CS_WORDS(CSPACE_RGB), 0, 0x1111, 0x2222, 0x3333), 0);
		CHECK_EQ(set_names[i], r, 0x100);
	}
	// Colour inquiries: intout[0..1] = colour space (returned), intout[2..5] = the colour entry.
	for (int i = 0; i < 10; i++)
	{
		t_run(get_names[i]);
		memset(&out, 0, sizeof(out));
		out.canary = CANARY;
		v_fake(6, 0);
		l = get_fns[i](FH, &out.c);
		v_off();
		V_IN(get_names[i], i < 5 ? 202 : 203, i % 5, 0, 0, 0, 0);
		CHECK_EQ(get_names[i], l, 0x01000101L);
		v_words(get_names[i], "colour", (short*)&out, W(0x102, 0x103, 0x104, 0x105, CANARY), 5);
	}

	static const char* hs_names[4] = { "vs_hilite_color", "vs_min_color", "vs_max_color", "vs_weight_color" };
	static short (*hs_fns[4])(short, int, COLOR_ENTRY*) = { vs_hilite_color, vs_min_color, vs_max_color, vs_weight_color };
	static const char* hq_names[4] = { "vq_hilite_color", "vq_min_color", "vq_max_color", "vq_weight_color" };
	static int (*hq_fns[4])(short, COLOR_ENTRY*) = { vq_hilite_color, vq_min_color, vq_max_color, vq_weight_color };
	for (int i = 0; i < 4; i++)
	{
		t_run(hs_names[i]);
		v_fake(1, 0);
		r = hs_fns[i](FH, CSPACE_RGB, &ce);
		v_off();
		V_IN(hs_names[i], 207, i, 0, 6, W(CS_WORDS(CSPACE_RGB), 0, 0x1111, 0x2222, 0x3333), 0);
		CHECK_EQ(hs_names[i], r, 0x100);
		t_run(hq_names[i]);
		memset(&out, 0, sizeof(out));
		out.canary = CANARY;
		v_fake(6, 0);
		l = hq_fns[i](FH, &out.c);
		v_off();
		V_IN(hq_names[i], 209, i, 0, 0, 0, 0);
		CHECK_EQ(hq_names[i], l, 0x01000101L);
		v_words(hq_names[i], "colour", (short*)&out, W(0x102, 0x103, 0x104, 0x105, CANARY), 5);
	}

	// Colour tables.
	static struct { COLOR_TAB t; COLOR_ENTRY c[2]; } ct;
	t_run("vq_ctab");
	v_fake(4, 0);
	r = vq_ctab(FH, sizeof(ct), &ct.t);
	v_off();
	V_IN("vq_ctab", 206, 0, 0, 2, W(0, sizeof(ct)), 0);
	t_check(r == 1 && v_rec[0].intout == (short*)&ct.t, "vq_ctab", "vq_ctab() = %d, intout %s the table", r, v_rec[0].intout == (short*)&ct.t ? "is" : "is not");
	v_fake(0, 0);
	r = vq_ctab(FH, sizeof(ct), &ct.t);
	v_off();
	CHECK_EQ("vq_ctab", r, 0);

	// One word only: a binding with a smaller local intout array would have its stack overwritten.
	t_run("vq_dflt_ctab");
	v_fake(1, 0);
	r = vq_dflt_ctab(FH, sizeof(ct), &ct.t);
	v_off();
	V_IN("vq_dflt_ctab", 206, 7, 0, 2, W(0, sizeof(ct)), 0);
	t_check(r == 1 && v_rec[0].intout == (short*)&ct.t, "vq_dflt_ctab", "vq_dflt_ctab() = %d", r);

	t_run("v_get_ctab_id");
	v_fake(2, 0);
	l = v_get_ctab_id(FH);
	v_off();
	V_IN("v_get_ctab_id", 206, 6, 0, 0, 0, 0);
	CHECK_EQ("v_get_ctab_id", l, 0x01000101L);

	t_run("vq_ctab_id");
	v_fake(2, 0);
	l = vq_ctab_id(FH);
	v_off();
	V_IN("vq_ctab_id", 206, 2, 0, 0, 0, 0);
	CHECK_EQ("vq_ctab_id", l, 0x01000101L);

	// v_ctab: the whole table in intin (length / 2 words).
	t_run("v_ctab");
	memset(&ct, 0, sizeof(ct));
	ct.t.magic = COLOR_TAB_MAGIC;
	ct.t.length = sizeof(ct);
	ct.t.no_colors = 2;
	ct.c[1].rgb.blue = 0x7777;
	v_fake(1, 0);
	r = v_ctab(FH, &ct.t);
	v_off();
	V_IN("v_ctab", 205, 0, 0, sizeof(ct) / 2, (short*)&ct, 0);
	CHECK_EQ("v_ctab", r, 0x100);

	t_run("v_ctab_entry");
	v_fake(1, 0);
	r = v_ctab_entry(FH, 9, CSPACE_RGB, &ce);
	v_off();
	V_IN("v_ctab_entry", 205, 1, 0, 7, W(9, CS_WORDS(CSPACE_RGB), 0, 0x1111, 0x2222, 0x3333), 0);
	CHECK_EQ("v_ctab_entry", r, 0x100);

	t_run("vs_dflt_ctab");
	v_fake(1, 0);
	r = vs_dflt_ctab(FH);
	v_off();
	V_IN("vs_dflt_ctab", 205, 2, 0, 0, 0, 0);
	CHECK_EQ("vs_dflt_ctab", r, 0x100);

	t_run("vq_ctab_entry");
	memset(&out, 0, sizeof(out));
	out.canary = CANARY;
	v_fake(6, 0);
	l = vq_ctab_entry(FH, 9, &out.c);
	v_off();
	V_IN("vq_ctab_entry", 206, 1, 0, 1, W(9), 0);
	CHECK_EQ("vq_ctab_entry", l, 0x01000101L);
	v_words("vq_ctab_entry", "colour", (short*)&out, W(0x102, 0x103, 0x104, 0x105, CANARY), 5);

	t_run("v_ctab_idx2vdi");
	v_fake(1, 0);
	r = v_ctab_idx2vdi(FH, 15);
	v_off();
	V_IN("v_ctab_idx2vdi", 206, 3, 0, 1, W(15), 0);
	CHECK_EQ("v_ctab_idx2vdi", r, 0x100);

	t_run("v_ctab_vdi2idx");
	v_fake(1, 0);
	r = v_ctab_vdi2idx(FH, 1);
	v_off();
	V_IN("v_ctab_vdi2idx", 206, 4, 0, 1, W(1), 0);
	CHECK_EQ("v_ctab_vdi2idx", r, 0x100);

	t_run("v_ctab_idx2value");
	v_fake(2, 0);
	l = v_ctab_idx2value(FH, 3);
	v_off();
	V_IN("v_ctab_idx2value", 206, 5, 0, 1, W(3), 0);
	CHECK_EQ("v_ctab_idx2value", l, 0x01000101L);

	t_run("v_create_ctab");
	v_fake(2, 0);
	COLOR_TAB* cp = v_create_ctab(FH, CSPACE_RGB, PX_PREF8);
	v_off();
	V_IN("v_create_ctab", 206, 8, 0, 4, W(CS_WORDS(CSPACE_RGB), CS_WORDS(PX_PREF8)), 0);
	CHECK_EQ("v_create_ctab", (long)cp, 0x01000101L);

	t_run("v_delete_ctab");
	v_fake(1, 0);
	r = v_delete_ctab(FH, (COLOR_TAB*)0x00123456L);
	v_off();
	V_IN("v_delete_ctab", 206, 9, 0, 2, W(0x0012, 0x3456), 0);
	CHECK_EQ("v_delete_ctab", r, 0x100);

	// v_create_itab: intin = table, bits, 2 reserved words.
	t_run("v_create_itab");
	v_fake(2, 0);
	ITAB_REF it = v_create_itab(FH, (COLOR_TAB*)0x00123456L, 5);
	v_off();
	V_IN("v_create_itab", 208, 0, 0, 5, W(0x0012, 0x3456, 5, 0, 0), 0);
	CHECK_EQ("v_create_itab", (long)it, 0x01000101L);

	t_run("v_delete_itab");
	v_fake(1, 0);
	r = v_delete_itab(FH, (ITAB_REF)0x00123457L);
	v_off();
	V_IN("v_delete_itab", 208, 1, 0, 2, W(0x0012, 0x3457), 0);
	CHECK_EQ("v_delete_itab", r, 0x100);

	// Colour values.
	t_run("v_color2value");
	v_fake(2, 0);
	l = v_color2value(FH, CSPACE_RGB, &ce);
	v_off();
	V_IN("v_color2value", 204, 0, 0, 6, W(CS_WORDS(CSPACE_RGB), 0, 0x1111, 0x2222, 0x3333), 0);
	CHECK_EQ("v_color2value", l, 0x01000101L);

	t_run("v_value2color");
	memset(&out, 0, sizeof(out));
	out.canary = CANARY;
	v_fake(6, 0);
	l = v_value2color(FH, 0x00123456L, &out.c);
	v_off();
	V_IN("v_value2color", 204, 1, 0, 2, W(0x0012, 0x3456), 0);
	CHECK_EQ("v_value2color", l, 0x01000101L);
	v_words("v_value2color", "colour", (short*)&out, W(0x102, 0x103, 0x104, 0x105, CANARY), 5);

	t_run("v_color2nearest");
	memset(&out, 0, sizeof(out));
	out.canary = CANARY;
	v_fake(6, 0);
	l = v_color2nearest(FH, CSPACE_RGB, &ce, &out.c);
	v_off();
	V_IN("v_color2nearest", 204, 2, 0, 6, W(CS_WORDS(CSPACE_RGB), 0, 0x1111, 0x2222, 0x3333), 0);
	CHECK_EQ("v_color2nearest", l, 0x01000101L);
	v_words("v_color2nearest", "colour", (short*)&out, W(0x102, 0x103, 0x104, 0x105, CANARY), 5);

	t_run("vq_px_format");
	unsigned int pxf = 0;
	v_fake(4, 0);
	l = vq_px_format(FH, &pxf);
	v_off();
	V_IN("vq_px_format", 204, 3, 0, 0, 0, 0);
	t_check(l == 0x01000101L && pxf == 0x01020103UL, "vq_px_format", "colour space %08lx pixel format %08lx", l, (long)pxf);

	// v_setrgb: the type in contrl[5], intin = r, g, b.
	t_run("v_setrgb");
	v_fake(0, 0);
	v_setrgb(FH, 2, 100, 200, 300);
	v_off();
	V_IN("v_setrgb", 138, 2, 0, 3, W(100, 200, 300), 0);

	// vr_transfer_bits: contrl[7..10] = bitmaps, contrl[11..12] = 0 (mask), ptsin = source and destination rectangles,
	// intin = mode and 3 zero words.
	t_run("vr_transfer_bits");
	GCBITMAP sb, db;
	v_fake(0, 0);
	vr_transfer_bits(FH, &sb, &db, W(1, 2, 3, 4), W(5, 6, 7, 8), T_LOGIC_XOR);
	v_off();
	V_IN("vr_transfer_bits", 170, 0, 4, 4, W(T_LOGIC_XOR, 0, 0, 0), W(1, 2, 3, 4, 5, 6, 7, 8));
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)&sb && ptr_at(&v_rec[0].contrl[9]) == (long)&db && ptr_at(&v_rec[0].contrl[11]) == 0,
		"vr_transfer_bits", "contrl[7..12] = %08lx %08lx %08lx", ptr_at(&v_rec[0].contrl[7]), ptr_at(&v_rec[0].contrl[9]), ptr_at(&v_rec[0].contrl[11]));

	// Bitmaps.
	t_run("v_open_bm");
	v_fake(0, 0);
	v_fake_contrl[6] = 0x77;
	v_fake_cmask = 1 << 6;
	r = v_open_bm(FH, &sb, 1, 0, 278, 278);
	v_off();
	V_IN("v_open_bm", 100, 3, 0, 4, W(1, 0, 278, 278), 0);
	CHECK_EQ("v_open_bm", ptr_at(&v_rec[0].contrl[7]), (long)&sb);
	CHECK_EQ("v_open_bm", r, 0x77);

	t_run("v_close_bm");
	v_fake(0, 0);
	v_close_bm(FH);
	v_off();
	V_IN("v_close_bm", 101, 1, 0, 0, 0, 0);

	t_run("v_clsbm");
	v_fake(0, 0);
	v_clsbm(FH);
	v_off();
	V_IN("v_clsbm", 101, 1, 0, 0, 0, 0);

	t_run("v_opnbm");
	short win[20], wo[60], h = FH;
	MFDB mf;
	for (int i = 0; i < 20; i++) win[i] = (short)(i + 1);
	wo[57] = CANARY;
	v_fake(45, 12);
	v_fake_contrl[6] = 0x78;
	v_fake_cmask = 1 << 6;
	v_opnbm(win, &mf, &h, wo);
	v_off();
	V_IN("v_opnbm", 100, 1, 0, 20, win, 0);
	CHECK_EQ("v_opnbm", ptr_at(&v_rec[0].contrl[7]), (long)&mf);
	CHECK_EQ("v_opnbm", h, 0x78);
	short want[58];
	for (int i = 0; i < 45; i++) want[i] = 0x100 + i;
	for (int i = 0; i < 12; i++) want[45 + i] = 0x200 + i;
	want[57] = CANARY;
	v_words("v_opnbm", "work_out", wo, want, 58);

	t_run("v_resize_bm");
	v_fake(1, 0);
	r = v_resize_bm(FH, 64, 32, 0x00010002L, (unsigned char*)0x00123456L);
	v_off();
	V_IN("v_resize_bm", 100, 2, 0, 6, W(64, 32, 1, 2, 0x0012, 0x3456), 0);
	CHECK_EQ("v_resize_bm", r, 0x100);

	t_run("v_opnprn");
	static PRN_SETTINGS ps;
	ps.driver_id = 21;
	v_fake(45, 12);
	v_fake_contrl[6] = 0x79;
	v_fake_cmask = 1 << 6;
	r = v_opnprn(FH, &ps, wo);
	v_off();
	V_IN("v_opnprn", 1, -1, 0, 16, 0, 0);
	t_check(v_rec[0].intin[0] == 21 && ptr_at(&v_rec[0].contrl[14]) == (long)&ps, "v_opnprn", "intin[0] = %d, contrl[14..15] = %08lx",
		v_rec[0].intin[0], ptr_at(&v_rec[0].contrl[14]));
	CHECK_EQ("v_opnprn", r, 0x79);

	t_run("vq_scrninfo");
	static short si[280];
	si[272] = CANARY;
	v_fake(272, 0);
	vq_scrninfo(FH, si);
	v_off();
	V_IN("vq_scrninfo", 102, 1, 0, 1, W(2), 0);
	t_check(si[0] == 0x100 && si[271] == 0x100 + 271 && si[272] == CANARY, "vq_scrninfo", "work_out[0] %x [271] %x [272] %x",
		si[0], si[271], (unsigned short)si[272]);

	// vq_ext_devinfo: intin = device id and three string pointers.
	t_run("vq_ext_devinfo");
	char p1[10], p2[10], p3[10];
	short ex = 0;
	v_fake(2, 0);
	r = vq_ext_devinfo(FH, 21, &ex, p1, p2, p3);
	v_off();
	V_IN("vq_ext_devinfo", 248, 4242, 0, 7, 0, 0);
	t_check(v_rec[0].intin[0] == 21 && ptr_at(&v_rec[0].intin[1]) == (long)p1 && ptr_at(&v_rec[0].intin[3]) == (long)p2 && ptr_at(&v_rec[0].intin[5]) == (long)p3,
		"vq_ext_devinfo", "intin = %d %08lx %08lx %08lx", v_rec[0].intin[0], ptr_at(&v_rec[0].intin[1]), ptr_at(&v_rec[0].intin[3]), ptr_at(&v_rec[0].intin[5]));
	v_words("vq_ext_devinfo", "exists, return", W(ex, r), W(0x100, 0x101), 2);

	// Fonts.
	t_run("vst_map_mode");
	v_fake(1, 0);
	r = vst_map_mode(FH, 0);
	v_off();
	V_IN("vst_map_mode", 236, 0, 0, 2, W(0, 1), 0);
	CHECK_EQ("vst_map_mode", r, 0x100);
	v_fake(0, 0);
	r = vst_map_mode(FH, 1);
	v_off();
	CHECK_EQ("vst_map_mode", r, 1);

	t_run("vst_width");
	short a = 0, b = 0, c = 0, d = 0;
	v_fake(0, 4);
	vst_width(FH, 12, &a, &b, &c, &d);
	v_off();
	V_IN("vst_width", 231, 0, 1, 0, 0, W(12, 0));
	v_words("vst_width", "sizes", W(a, b, c, d), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vst_track_offset");
	a = b = 0;
	v_fake(2, 0);
	vst_track_offset(FH, 0x00028000L, 1, &a, &b);
	v_off();
	V_IN("vst_track_offset", 237, 0, 0, 4, W(255, 1, 2, (short)0x8000), 0);
	v_words("vst_track_offset", "tracks, pairs", W(a, b), W(0x100, 0x101), 2);

	// vst_name / vqt_name_and_id: intin = font format, name; intout = id, name.
	char name[90];
	t_run("vst_name");
	memset(name, 'X', sizeof(name));
	v_fake(5, 0);
	memcpy(&v_fake_intout[1], W('A', 'r', 0xe4, 0), 8);
	r = vst_name(FH, 2, "Ar\x84", name);
	v_off();
	V_IN("vst_name", 230, 0, 0, 4, W(2, 'A', 'r', 0x84), 0);
	t_check(r == 0x100 && strcmp(name, "Ar\xe4") == 0, "vst_name", "vst_name() = %x \"%.20s\"", r, name);

	t_run("vqt_name_and_id");
	memset(name, 'X', sizeof(name));
	v_fake(4, 0);
	memcpy(&v_fake_intout[1], W('B', 'c', 0), 6);
	r = vqt_name_and_id(FH, 2, "Bc", name);
	v_off();
	V_IN("vqt_name_and_id", 230, 100, 0, 3, W(2, 'B', 'c'), 0);
	t_check(r == 0x100 && strcmp(name, "Bc") == 0, "vqt_name_and_id", "vqt_name_and_id() = %x \"%.20s\"", r, name);

	t_run("vqt_char_index");
	v_fake(1, 0);
	r = vqt_char_index(FH, 0x84, 1, 2);
	v_off();
	V_IN("vqt_char_index", 190, 0, 0, 3, W(0x84, 1, 2), 0);
	CHECK_EQ("vqt_char_index", r, 0x100);

	// v_get_outline: intin = index, max points, points pointer, flags pointer, x and y offset.
	t_run("v_get_outline");
	short pts[4];
	char fl[2];
	v_fake(1, 0);
	r = v_get_outline(FH, 33, 5, 6, pts, fl, 2);
	v_off();
	V_IN("v_get_outline", 243, 1, 0, 8, 0, 0);
	t_check(v_rec[0].intin[0] == 33 && v_rec[0].intin[1] == 2 && ptr_at(&v_rec[0].intin[2]) == (long)pts && ptr_at(&v_rec[0].intin[4]) == (long)fl &&
		v_rec[0].intin[6] == 5 && v_rec[0].intin[7] == 6, "v_get_outline", "intin = %d %d %08lx %08lx %d %d", v_rec[0].intin[0], v_rec[0].intin[1],
		ptr_at(&v_rec[0].intin[2]), ptr_at(&v_rec[0].intin[4]), v_rec[0].intin[6], v_rec[0].intin[7]);
	CHECK_EQ("v_get_outline", r, 0x100);

	t_run("vqt_xfntinfo");
	static XFNT_INFO xi;
	v_fake(3, 0);
	r = vqt_xfntinfo(FH, 0xff, 3, 4, &xi);
	v_off();
	V_IN("vqt_xfntinfo", 229, 0, 0, 5, 0, 0);
	t_check(v_rec[0].intin[0] == 0xff && v_rec[0].intin[1] == 3 && v_rec[0].intin[2] == 4 && ptr_at(&v_rec[0].intin[3]) == (long)&xi,
		"vqt_xfntinfo", "intin = %x %d %d %08lx", v_rec[0].intin[0], v_rec[0].intin[1], v_rec[0].intin[2], ptr_at(&v_rec[0].intin[3]));
	CHECK_EQ("vqt_xfntinfo", xi.size, (long)sizeof(XFNT_INFO));
	CHECK_EQ("vqt_xfntinfo", r, 0x101);

	// Text with 16 bit characters.
	t_run("v_ftext_wide");
	v_fake(0, 0);
	v_ftext_wide(FH, 10, 20, (unsigned short*)W('A', 0x20ac, 0));
	v_off();
	V_IN("v_ftext_wide", 241, 0, 1, 2, W('A', 0x20ac), W(10, 20));

	t_run("vqt_extent_wide");
	short ext[9];
	ext[8] = CANARY;
	v_fake(0, 8);
	vqt_extent_wide(FH, (unsigned short*)W('A', 0x20ac, 0), ext);
	v_off();
	V_IN("vqt_extent_wide", 240, 0, 0, 2, W('A', 0x20ac), 0);
	v_words("vqt_extent_wide", "extent", ext, W(0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, CANARY), 9);

	t_run("vqt_real_extent");
	ext[8] = CANARY;
	v_fake(0, 8);
	vqt_real_extent(FH, 10, 20, "Ab\x84", ext);
	v_off();
	V_IN("vqt_real_extent", 240, 4200, 1, 3, W('A', 'b', 0x84), W(10, 20));
	v_words("vqt_real_extent", "extent", ext, W(0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, CANARY), 9);

	t_run("vqt_real_extent_wide");
	ext[8] = CANARY;
	v_fake(0, 8);
	vqt_real_extent_wide(FH, 11, 21, W('A', 0x20ac, 0), ext);
	v_off();
	V_IN("vqt_real_extent_wide", 240, 4200, 1, 2, W('A', 0x20ac), W(11, 21));
	v_words("vqt_real_extent_wide", "extent", ext, W(0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, CANARY), 9);

	// vqt_ext_name: intin = index, 0; intout = id, 32 name characters, flags (high byte) and format (low byte) in intout[34].
	t_run("vqt_ext_name");
	short fmt = 0, flg = 0;
	memset(name, 'X', sizeof(name));
	v_fake(35, 0);
	for (int i = 0; i < 32; i++) v_fake_intout[1 + i] = 'a' + i % 26;
	v_fake_intout[1 + 10] = 0;
	v_fake_intout[34] = 0x0102;
	r = vqt_ext_name(FH, 5, name, &fmt, &flg);
	v_off();
	V_IN("vqt_ext_name", 130, 1, 0, 2, W(5, 0), 0);
	t_check(r == 0x100 && strcmp(name, "abcdefghij") == 0 && flg == 1 && fmt == 2, "vqt_ext_name", "id %x \"%.32s\" format %d flags %d", r, name, fmt, flg);
}

static void real_tests(void)
{
	short h = v_handle, r;
	long l;

	// vq_scrninfo: EmuTOS and NVDI implement it.
	t_run("vq_scrninfo");
	static short si[272];
	memset(si, 0, sizeof(si));
	v_spy();
	vq_scrninfo(h, si);
	v_off();
	v_out_fits("vq_scrninfo", 0, 272, 16);
	if (v_rec[0].nintout != 272)
	{
		t_skip("vq_scrninfo", "not implemented (%d intout words returned)", v_rec[0].nintout);
	}
	else
	{
		t_check(si[2] == v_scr_planes && si[5] == v_scr_bpl, "vq_scrninfo", "format %d clut %d planes %d colours %ld line %d",
			si[0], si[1], si[2], ptr_at(&si[3]), si[5]);
	}

	if (!v_nvdi)
	{
		t_skip("vq_px_format", "no NVDI");
		return;
	}

	t_run("vq_px_format");
	unsigned int pxf = 0;
	l = vq_px_format(h, &pxf);
	unsigned int want = v_scr_planes == 1 ? PX_ATARI1 : v_scr_planes == 2 ? PX_ATARI2 : v_scr_planes == 4 ? PX_ATARI4 : PX_ATARI8;
	t_check(l == CSPACE_RGB && pxf == want, "vq_px_format", "colour space %ld, pixel format %08lx (expected %08lx)", l, (long)pxf, (long)want);

	t_run("vq_ctab_id");
	l = vq_ctab_id(h);
	t_check(l != 0, "vq_ctab_id", "id %ld", l);
	t_run("v_get_ctab_id");
	long l2 = v_get_ctab_id(h);
	long l3 = v_get_ctab_id(h);
	t_check(l2 != 0 && l2 != l3, "v_get_ctab_id", "new ids %ld %ld", l2, l3);

	// Colour table of the workstation.
	static struct { COLOR_TAB t; COLOR_ENTRY c[256]; } ct;
	t_run("vq_ctab");
	memset(&ct, 0, sizeof(ct));
	v_spy();
	r = vq_ctab(h, sizeof(ct), &ct.t);
	v_off();
	t_print("INFO vq_ctab: contrl[2] %d contrl[4] %d, %d intout words written", v_rec[0].contrl_out[2], v_rec[0].contrl_out[4], v_rec[0].nintout);
	t_check(r == 1 && ct.t.magic == COLOR_TAB_MAGIC && ct.t.no_colors == v_work_out[13] && ct.t.color_space == CSPACE_RGB, "vq_ctab",
		"vq_ctab() = %d magic %08lx %ld colours, colour space %ld", r, (long)ct.t.magic, (long)ct.t.no_colors, (long)ct.t.color_space);
	t_run("vq_dflt_ctab");
	static struct { COLOR_TAB t; COLOR_ENTRY c[256]; } dt;
	// The table is returned in intout, the binding must hand the table to the VDI.
	v_spy();
	v_fake(1, 0);		// only to see which intout array the binding passes
	vq_dflt_ctab(h, sizeof(dt), &dt.t);
	v_off();
	if (v_rec[0].intout != (short*)&dt.t)
	{
		t_check(0, "vq_dflt_ctab", "the binding doesn't pass the table as intout (the VDI writes the table there), not called");
	}
	else
	{
		v_spy();
		r = vq_dflt_ctab(h, sizeof(dt), &dt.t);
		v_off();
		t_print("INFO vq_dflt_ctab: contrl[2] %d contrl[4] %d, %d intout words written", v_rec[0].contrl_out[2], v_rec[0].contrl_out[4], v_rec[0].nintout);
		t_check(r == 1 && dt.t.magic == COLOR_TAB_MAGIC && dt.t.no_colors == v_work_out[13], "vq_dflt_ctab", "vq_dflt_ctab() = %d, %ld colours", r, (long)dt.t.no_colors);
	}

	// Index conversion: VDI index 1 (black) and back.
	t_run("v_ctab_vdi2idx");
	short ci = v_ctab_vdi2idx(h, 1);
	t_check(ci >= 0 && ci < v_work_out[13], "v_ctab_vdi2idx", "VDI 1 = table index %d", ci);
	t_run("v_ctab_idx2vdi");
	r = v_ctab_idx2vdi(h, ci);
	CHECK_EQ("v_ctab_idx2vdi", r, 1);
	t_run("v_ctab_idx2value");
	unsigned int val = v_ctab_idx2value(h, ci);
	t_check(val == (unsigned int)ci, "v_ctab_idx2value", "table index %d = pixel value %lu", ci, (unsigned long)val);
	t_run("vq_ctab_entry");
	COLOR_ENTRY ce;
	memset(&ce, 0xff, sizeof(ce));
	l = vq_ctab_entry(h, ci, &ce);
	t_check(l == CSPACE_RGB && ce.rgb.red < 0x1000 && ce.rgb.green < 0x1000 && ce.rgb.blue < 0x1000, "vq_ctab_entry", "index %d: space %ld rgb %04x %04x %04x",
		ci, l, ce.rgb.red, ce.rgb.green, ce.rgb.blue);

	// Colour values: black and back.
	t_run("v_color2value");
	COLOR_ENTRY black = { .rgb = { 0, 0, 0, 0 } };
	val = v_color2value(h, CSPACE_RGB, &black);
	CHECK_EQ("v_color2value", (long)val, (long)ci);
	t_run("v_value2color");
	memset(&ce, 0xff, sizeof(ce));
	l = v_value2color(h, val, &ce);
	t_check(l == CSPACE_RGB && ce.rgb.red < 0x1000 && ce.rgb.green < 0x1000 && ce.rgb.blue < 0x1000, "v_value2color", "value %lu: space %ld rgb %04x %04x %04x",
		(unsigned long)val, l, ce.rgb.red, ce.rgb.green, ce.rgb.blue);
	t_run("v_color2nearest");
	COLOR_ENTRY dark = { .rgb = { 0, 0x0800, 0x0800, 0x0800 } };
	memset(&ce, 0xff, sizeof(ce));
	l = v_color2nearest(h, CSPACE_RGB, &dark, &ce);
	t_check(l == CSPACE_RGB && ce.rgb.red < 0x1000 && ce.rgb.green < 0x1000, "v_color2nearest", "nearly black: space %ld rgb %04x %04x %04x",
		l, ce.rgb.red, ce.rgb.green, ce.rgb.blue);

	// Foreground/background colours: set black, read back.
	for (int i = 0; i < 10; i++)
	{
		t_run(set_names[i]);
		r = set_fns[i](h, CSPACE_RGB, &black);
		t_check(r == 1, set_names[i], "%s() = %d", set_names[i], r);
		t_run(get_names[i]);
		memset(&ce, 0x55, sizeof(ce));
		l = get_fns[i](h, &ce);
		t_check(l == CSPACE_RGB && ce.rgb.red < 0x1000 && ce.rgb.green < 0x1000 && ce.rgb.blue < 0x1000, get_names[i], "space %ld rgb %04x %04x %04x",
			l, ce.rgb.red, ce.rgb.green, ce.rgb.blue);
	}

	static const char* hs_names[4] = { "vs_hilite_color", "vs_min_color", "vs_max_color", "vs_weight_color" };
	static short (*hs_fns[4])(short, int, COLOR_ENTRY*) = { vs_hilite_color, vs_min_color, vs_max_color, vs_weight_color };
	static const char* hq_names[4] = { "vq_hilite_color", "vq_min_color", "vq_max_color", "vq_weight_color" };
	static int (*hq_fns[4])(short, COLOR_ENTRY*) = { vq_hilite_color, vq_min_color, vq_max_color, vq_weight_color };
	for (int i = 0; i < 4; i++)
	{
		t_run(hq_names[i]);
		COLOR_ENTRY old;
		memset(&old, 0x55, sizeof(old));
		l = hq_fns[i](h, &old);
		t_check(l == CSPACE_RGB, hq_names[i], "space %ld rgb %04x %04x %04x", l, old.rgb.red, old.rgb.green, old.rgb.blue);
		t_run(hs_names[i]);
		r = hs_fns[i](h, CSPACE_RGB, &old);
		t_check(r == 1, hs_names[i], "%s() = %d", hs_names[i], r);
	}

	// Set the colour table back, then the default one.
	t_run("v_ctab");
	COLOR_ENTRY old5 = ct.c[5];
	ct.c[5].rgb.red = 0xffff;
	ct.c[5].rgb.green = 0;
	ct.c[5].rgb.blue = 0xffff;
	r = v_ctab(h, &ct.t);		// returns the number of colours set
	COLOR_ENTRY e5;
	memset(&e5, 0x55, sizeof(e5));
	vq_ctab_entry(h, 5, &e5);
	t_check(r == ct.t.no_colors && e5.rgb.red == 0xffff && e5.rgb.green == 0 && e5.rgb.blue == 0xffff, "v_ctab",
		"v_ctab() = %d, entry 5 = %04x %04x %04x", r, e5.rgb.red, e5.rgb.green, e5.rgb.blue);
	ct.c[5] = old5;
	v_ctab(h, &ct.t);
	t_run("v_ctab_entry");
	r = v_ctab_entry(h, ci, CSPACE_RGB, &black);
	CHECK_EQ("v_ctab_entry", r, 1);
	t_run("vs_dflt_ctab");
	r = vs_dflt_ctab(h);
	CHECK_EQ("vs_dflt_ctab", r, 1);

	t_run("v_create_ctab");
	COLOR_TAB* nt = v_create_ctab(h, CSPACE_RGB, PX_PREF8);
	t_check(nt != 0 && nt->magic == COLOR_TAB_MAGIC && nt->no_colors == 256, "v_create_ctab", "table %08lx, %ld colours", (long)nt, nt ? (long)nt->no_colors : 0L);
	if (nt)
	{
		t_run("v_create_itab");
		ITAB_REF it = v_create_itab(h, nt, 4);
		t_check(it != 0, "v_create_itab", "inverse table %08lx", (long)it);
		if (it)
		{
			t_run("v_delete_itab");
			r = v_delete_itab(h, it);
			CHECK_EQ("v_delete_itab", r, 1);
		}
		t_run("v_delete_ctab");
		r = v_delete_ctab(h, nt);
		if (r == 0)
		{
			// Its handler for 206/9 calls the inverse table routine (the one behind v_delete_itab).
			t_skip("v_delete_ctab", "NVDI can't delete colour tables (returns 0, frees nothing)");
		}
		else
		{
			CHECK_EQ("v_delete_ctab", r, 1);
		}
	}

	// v_setrgb sets the foreground colour of a group: 1 = fill.
	t_run("v_setrgb");
	v_setrgb(h, 1, 0, 0, 0);
	memset(&ce, 0x55, sizeof(ce));
	l = vqf_fg_color(h, &ce);
	t_check(l == CSPACE_RGB && ce.rgb.red < 0x1000, "v_setrgb", "fill colour space %ld rgb %04x %04x %04x", l, ce.rgb.red, ce.rgb.green, ce.rgb.blue);

	// An offscreen bitmap compatible with the screen, NVDI allocates the memory.
	t_run("v_open_bm");
	GCBITMAP bm;
	memset(&bm, 0, sizeof(bm));
	bm.magic = CBITMAP_MAGIC;
	bm.length = sizeof(bm);
	bm.xmax = 63;
	bm.ymax = 31;
	short bh = v_open_bm(h, &bm, 0, 0, 278, 278);
	if (t_check(bh > 0 && bm.addr != 0 && bm.width > 0, "v_open_bm", "handle %d, %ld bytes per line, %ld bits at %08lx", bh, (long)bm.width, (long)bm.bits, (long)bm.addr))
	{
		memset(bm.addr, 0, bm.width * 32);
		vsf_interior(bh, FIS_SOLID);
		vsf_color(bh, 1);
		vswr_mode(bh, MD_REPLACE);
		vr_recfl(bh, W(0, 0, 63, 31));
		int set = 0;
		for (long i = 0; i < bm.width * 32; i++)
		{
			set += ((unsigned char*)bm.addr)[i] != 0;
		}
		t_check(set > 0, "v_open_bm", "drawing in the bitmap: %d non zero bytes", set);
		t_run("v_resize_bm");
		static unsigned char nb[32 * 16 * 8];
		r = v_resize_bm(bh, 32, 16, 16 * v_scr_planes / 8 * 2, nb);
		t_check(r == 1, "v_resize_bm", "v_resize_bm() = %d", r);
		t_run("v_close_bm");
		v_close_bm(bh);
		t_check(1, "v_close_bm", "returned");
	}

	// v_opnbm: a screen compatible bitmap described by an MFDB.
	t_run("v_opnbm");
	short win[20], wo[57], oh = h;
	for (int i = 0; i < 20; i++) win[i] = 1;
	win[10] = 2;
	win[11] = 63;
	win[12] = 31;
	win[13] = v_work_out[3];
	win[14] = v_work_out[4];
	for (int i = 15; i < 20; i++) win[i] = 0;
	MFDB mf = { 0, 64, 32, 4, 0, 0, 0, 0, 0 };
	v_opnbm(win, &mf, &oh, wo);
	if (t_check(oh > 0 && oh != h && mf.fd_addr != 0 && wo[0] == 63 && wo[1] == 31, "v_opnbm", "handle %d, %dx%d at %08lx, work_out %d %d",
		oh, mf.fd_w, mf.fd_h, (long)mf.fd_addr, wo[0], wo[1]))
	{
		t_run("v_clsbm");
		v_clsbm(oh);
		t_check(1, "v_clsbm", "returned");
	}

	// vr_transfer_bits between two 1 bit bitmaps.
	t_run("vr_transfer_bits");
	static unsigned char sbits[4 * 16], dbits[4 * 16];
	for (int i = 0; i < 64; i++)
	{
		sbits[i] = (unsigned char)(i * 37 + 1);
		dbits[i] = 0;
	}
	GCBITMAP sb, db;
	memset(&sb, 0, sizeof(sb));
	sb.magic = CBITMAP_MAGIC;
	sb.length = sizeof(sb);
	sb.addr = sbits;
	sb.width = 4;
	sb.bits = 1;
	sb.px_format = PX_PREF1;
	sb.xmax = 32;
	sb.ymax = 16;
	db = sb;
	db.addr = dbits;
	vr_transfer_bits(h, &sb, &db, W(0, 0, 31, 15), W(0, 0, 31, 15), T_LOGIC_COPY);
	t_check(memcmp(sbits, dbits, 64) == 0, "vr_transfer_bits", "1 bit copy %s", memcmp(sbits, dbits, 64) == 0 ? "matches" : "differs");

	// Fonts by name and index.
	short n = vst_load_fonts(h, 0);
	char name[90], name2[90];
	short fmt = -1, flg = -1;
	t_run("vqt_ext_name");
	short id = 0, vid = 0, vfmt = 0;
	int ok = 1;
	for (short i = 1; i <= 1 + n; i++)
	{
		static V_RAW raw;
		memset(&raw, 0, sizeof(raw));
		raw.contrl[0] = 130;
		raw.contrl[3] = 1;
		raw.contrl[6] = h;
		raw.intin[0] = i;
		v_raw(&raw);
		memset(name, 0, sizeof(name));
		id = vqt_ext_name(h, i, name, &fmt, &flg);
		for (int j = 0; j < 32 && ok; j++)
		{
			if (name[j] != (char)raw.intout[1 + j])
			{
				ok = t_check(0, "vqt_ext_name", "font %d name[%d] = %d, vqt_name gives %d", i, j, name[j], raw.intout[1 + j]);
			}
		}
		if (id != raw.intout[0])
		{
			ok = t_check(0, "vqt_ext_name", "font %d id %d, vqt_name gives %d", i, id, raw.intout[0]);
		}
		t_print("INFO font %d: id %d \"%.32s\" format %d flags %x", i, id, name, fmt, flg);
		if (fmt > 1 && !vid)
		{
			vid = id;
			vfmt = fmt;
			strcpy(name2, name);
		}
	}
	if (ok)
	{
		t_check(1, "vqt_ext_name", "%d fonts match vqt_name", 1 + n);
	}
	if (!vid)
	{
		t_skip("vst_name", "no vector font");
		vst_unload_fonts(h, 0);
		return;
	}
	// Trailing spaces are not part of the name to search for.
	for (int j = (int)strlen(name2) - 1; j >= 0 && name2[j] == ' '; j--)
	{
		name2[j] = 0;
	}
	t_run("vst_name");
	memset(name, 0, sizeof(name));
	r = vst_name(h, vfmt, name2, name);
	t_check(r == vid, "vst_name", "\"%s\" = id %d (expected %d), \"%.40s\"", name2, r, vid, name);
	t_run("vqt_name_and_id");
	memset(name, 0, sizeof(name));
	r = vqt_name_and_id(h, vfmt, name2, name);
	t_check(r == vid, "vqt_name_and_id", "\"%s\" = id %d (expected %d), \"%.40s\"", name2, r, vid, name);

	t_run("vqt_xfntinfo");
	static XFNT_INFO xi;
	memset(&xi, 0, sizeof(xi));
	r = vqt_xfntinfo(h, 0xff, vid, 0, &xi);
	t_check(r == vid && xi.id == vid && xi.font_name[0] != 0 && xi.format > 1, "vqt_xfntinfo", "id %d format %d \"%.40s\" family \"%.40s\" file \"%.40s\"",
		r, xi.format, xi.font_name, xi.family_name, xi.file_name1);

	vst_font(h, vid);
	short a, b, c, d;
	vst_arbpt(h, 20, &a, &b, &c, &d);

	t_run("vst_map_mode");
	r = vst_map_mode(h, 1);
	CHECK_EQ("vst_map_mode", r, 1);
	t_run("vqt_char_index");
	r = vqt_char_index(h, 0x84, 1, 2);
	CHECK_EQ("vqt_char_index", r, 0xe4);
	t_run("vst_width");
	a = b = c = d = -1;
	vst_width(h, 10, &a, &b, &c, &d);
	t_check(a > 0 && b > 0 && c > 0 && d > 0, "vst_width", "char %dx%d cell %dx%d", a, b, c, d);
	vst_width(h, 0, &a, &b, &c, &d);
	t_run("vst_track_offset");
	a = b = -1;
	vst_track_offset(h, 0x00010000L, 0, &a, &b);
	t_check(a >= 0, "vst_track_offset", "tracks %d pairs %d", a, b);
	vst_track_offset(h, 0, 0, &a, &b);

	t_run("v_get_outline");
	static short pts[2 * 400];
	static char fl[400];
	short gi = vqt_char_index(h, 'H', 1, 0);
	r = v_get_outline(h, gi, 0, 0, pts, fl, 400);
	t_check(r > 4 && r <= 400, "v_get_outline", "'H' (index %d): %d points", gi, r);

	short dummy;
	vst_alignment(h, 0, 5, &dummy, &dummy);
	v_scr_begin();
	vst_font(h, vid);
	vst_arbpt(h, 20, &a, &b, &c, &d);
	vst_alignment(h, 0, 5, &dummy, &dummy);		// v_scr_begin sets the default
	short x = v_ox, y = v_oy;
	v_clear(x, y, 100, 40);
	v_ftext(h, x + 2, y + 2, "Hi");
	int narrow = v_count_ink(x, y, 100, 40);
	t_run("v_ftext_wide");
	v_clear(x, y, 100, 40);
	v_ftext_wide(h, x + 2, y + 2, (unsigned short*)W('H', 'i', 0));
	int ink = v_count_ink(x, y, 100, 40);
	t_check(ink > 20 && ink == narrow, "v_ftext_wide", "\"Hi\": %d pixels, v_ftext %d", ink, narrow);
	t_run("vqt_extent_wide");
	short e1[8], e2[8];
	vqt_extent_wide(h, (unsigned short*)W('H', 'i', 0), e1);
	vqt_f_extent(h, "Hi", e2);
	t_check(e1[2] > 10 && memcmp(e1, e2, sizeof(e1)) == 0, "vqt_extent_wide", "\"Hi\": width %d (vqt_f_extent %d)", e1[2] - e1[0], e2[2] - e2[0]);
	t_run("vqt_real_extent");
	vqt_real_extent(h, x, y, "Hi", e1);
	t_check(e1[2] - e1[0] > 10 && e1[0] >= x - 2, "vqt_real_extent", "\"Hi\" at %d,%d: %d,%d %d,%d %d,%d %d,%d", x, y, e1[0], e1[1], e1[2], e1[3], e1[4], e1[5], e1[6], e1[7]);
	t_run("vqt_real_extent_wide");
	vqt_real_extent_wide(h, x, y, W('H', 'i', 0), e2);
	t_check(memcmp(e1, e2, sizeof(e1)) == 0, "vqt_real_extent_wide", "same as vqt_real_extent: %d,%d %d,%d", e2[0], e2[1], e2[4], e2[5]);
	v_scr_end();

	// Device 31 is META.SYS.
	t_run("vq_ext_devinfo");
	char path[130], file[40], dname[80];
	short ex = -1;
	memset(path, 0, sizeof(path));
	memset(file, 0, sizeof(file));
	memset(dname, 0, sizeof(dname));
	r = vq_ext_devinfo(h, 31, &ex, path, file, dname);
	t_check(strncmp(file, "META", 4) == 0, "vq_ext_devinfo", "device 31: %d exists %d path \"%.40s\" file \"%.20s\" name \"%.40s\"", r, ex, path, file, dname);

	vst_unload_fonts(h, 0);
}

int main(void)
{
	if (v_begin("V_NVDI"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
