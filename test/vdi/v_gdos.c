/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	GDOS: Bezier curves and the SpeedoGDOS (vector font) functions.
	Real tests with NVDI, which is a GDOS with vector fonts.
*/

#include <string.h>
#include "v_util.h"

#define CANARY 0x5aa5

static long ptr_at(const short* w)
{
	return ((long)(unsigned short)w[0] << 16) | (unsigned short)w[1];
}

static void binding_tests(void)
{
	short r, a, b, c, d;
	short ext[9];
	char buf[64];

	t_run("v_bez_on");
	v_fake(1, 0);
	r = v_bez_on(FH);
	v_off();
	V_IN("v_bez_on", 11, 13, 1, 0, 0, 0);
	CHECK_EQ("v_bez_on", r, 0x100);

	t_run("v_bez_off");
	v_fake(0, 0);
	v_bez_off(FH);
	v_off();
	V_IN("v_bez_off", 11, 13, 0, 0, 0, 0);

	// v_set_app_buff: opcode -1, intin = address, size in paragraphs.
	t_run("v_set_app_buff");
	v_fake(0, 0);
	v_set_app_buff(FH, (void*)0x00123456L, 64);
	v_off();
	V_IN("v_set_app_buff", -1, 6, 0, 3, W(0x0012, 0x3456, 64), 0);

	// v_bez: contrl[1] = points, contrl[3] = (points + 1) / 2, the flag bytes packed two per word, low byte first.
	short xy[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	char bez[6] = { 1, 0, 0, 2, 0, 3 };
	t_run("v_bez");
	a = b = 0;
	ext[4] = CANARY;
	v_fake(2, 4);
	v_bez(FH, 6, xy, bez, ext, &a, &b);
	v_off();
	V_IN("v_bez", 6, 13, 6, 3, W(0x0001, 0x0200, 0x0300), xy);
	v_words("v_bez", "extent, points, moves", W(ext[0], ext[1], ext[2], ext[3], ext[4], a, b), W(0x200, 0x201, 0x202, 0x203, CANARY, 0x100, 0x101), 7);

	t_run("v_bez_fill");
	a = b = 0;
	v_fake(2, 4);
	v_bez_fill(FH, 6, xy, bez, ext, &a, &b);
	v_off();
	V_IN("v_bez_fill", 9, 13, 6, 3, W(0x0001, 0x0200, 0x0300), xy);
	v_words("v_bez_fill", "extent, points, moves", W(ext[0], ext[1], ext[2], ext[3], a, b), W(0x200, 0x201, 0x202, 0x203, 0x100, 0x101), 6);

	t_run("v_bez_qual");
	a = 0;
	v_fake(1, 0);
	r = v_bez_qual(FH, 75, &a);
	v_off();
	V_IN("v_bez_qual", 5, 99, 0, 3, W(32, 1, 75), 0);
	v_words("v_bez_qual", "return, actual", W(r, a), W(0x100, 0x100), 2);

	t_run("vqt_f_extent");
	ext[8] = CANARY;
	v_fake(0, 8);
	vqt_f_extent(FH, "Ab\x84", ext);
	v_off();
	V_IN("vqt_f_extent", 240, -1, 0, 3, W('A', 'b', 0x84), 0);
	v_words("vqt_f_extent", "extent", ext, W(0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, CANARY), 9);

	t_run("v_ftext");
	v_fake(0, 0);
	v_ftext(FH, 10, 20, "Ab\x84");
	v_off();
	V_IN("v_ftext", 241, -1, 1, 3, W('A', 'b', 0x84), W(10, 20));

	// v_ftext_offset: ptsin = x, y, then an x/y offset per character.
	t_run("v_ftext_offset");
	v_fake(0, 0);
	v_ftext_offset(FH, 10, 20, "Ab", W(1, 2, 3, 4));
	v_off();
	V_IN("v_ftext_offset", 241, -1, 3, 2, W('A', 'b'), W(10, 20, 1, 2, 3, 4));

	// v_getoutline: intin = character, maxverts, xyarray pointer, bezarray pointer. This is the layout NVDI uses
	// (the SpeedoGDOS documentation has the pointers first and maxverts last).
	t_run("v_getoutline");
	short oxy[8];
	char obez[4];
	a = 0;
	v_fake(1, 0);
	v_getoutline(FH, 'A', oxy, obez, 4, &a);
	v_off();
	V_IN("v_getoutline", 243, -1, 0, 6, 0, 0);
	t_check(v_rec[0].intin[0] == 'A' && v_rec[0].intin[1] == 4 && ptr_at(&v_rec[0].intin[2]) == (long)oxy && ptr_at(&v_rec[0].intin[4]) == (long)obez,
		"v_getoutline", "intin = %d, %d, %08lx, %08lx; expected 'A', 4, %08lx, %08lx", v_rec[0].intin[0], v_rec[0].intin[1],
		ptr_at(&v_rec[0].intin[2]), ptr_at(&v_rec[0].intin[4]), (long)oxy, (long)obez);
	CHECK_EQ("v_getoutline", a, 0x100);

	t_run("vst_scratch");
	v_fake(0, 0);
	vst_scratch(FH, 2);
	v_off();
	V_IN("vst_scratch", 244, -1, 0, 1, W(2), 0);

	t_run("vst_error");
	short err;
	v_fake(0, 0);
	vst_error(FH, 1, &err);
	v_off();
	V_IN("vst_error", 245, -1, 0, 3, 0, 0);
	t_check(v_rec[0].intin[0] == 1 && ptr_at(&v_rec[0].intin[1]) == (long)&err, "vst_error", "intin = %d, %08lx", v_rec[0].intin[0], ptr_at(&v_rec[0].intin[1]));

	t_run("vqt_advance");
	a = b = c = d = 0;
	v_fake(0, 8);
	vqt_advance(FH, 'W', &a, &b, &c, &d);
	v_off();
	V_IN("vqt_advance", 247, -1, 0, 1, W('W'), 0);
	v_words("vqt_advance", "outputs", W(a, b, c, d), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vqt_advance32");
	fix31 fx = 0, fy = 0;
	v_fake(0, 8);
	vqt_advance32(FH, 'W', &fx, &fy);
	v_off();
	V_IN("vqt_advance32", 247, -1, 0, 1, W('W'), 0);
	t_check(fx == 0x02040205L && fy == 0x02060207L, "vqt_advance32", "advance %08lx %08lx, expected ptsout[4..5] ptsout[6..7]", (long)fx, (long)fy);

	t_run("vst_arbpt");
	a = b = c = d = 0;
	v_fake(1, 4);
	r = vst_arbpt(FH, 15, &a, &b, &c, &d);
	v_off();
	V_IN("vst_arbpt", 246, -1, 0, 1, W(15), 0);
	v_words("vst_arbpt", "return, sizes", W(r, a, b, c, d), W(0x100, 0x200, 0x201, 0x202, 0x203), 5);

	t_run("vst_arbpt32");
	a = b = c = d = 0;
	v_fake(2, 4);
	fx = vst_arbpt32(FH, 0x000f8000L, &a, &b, &c, &d);
	v_off();
	V_IN("vst_arbpt32", 246, -1, 0, 2, W(0x000f, (short)0x8000), 0);
	t_check(fx == 0x01000101L, "vst_arbpt32", "returned %08lx, expected intout[0..1]", (long)fx);
	v_words("vst_arbpt32", "sizes", W(a, b, c, d), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vst_setsize");
	a = b = c = d = 0;
	v_fake(1, 4);
	r = vst_setsize(FH, 18, &a, &b, &c, &d);
	v_off();
	V_IN("vst_setsize", 252, -1, 0, 1, W(18), 0);
	v_words("vst_setsize", "return, sizes", W(r, a, b, c, d), W(0x100, 0x200, 0x201, 0x202, 0x203), 5);

	t_run("vst_setsize32");
	a = b = c = d = 0;
	v_fake(2, 4);
	fx = vst_setsize32(FH, 0x00128000L, &a, &b, &c, &d);
	v_off();
	V_IN("vst_setsize32", 252, -1, 0, 2, W(0x0012, (short)0x8000), 0);
	t_check(fx == 0x01000101L, "vst_setsize32", "returned %08lx, expected intout[0..1]", (long)fx);
	v_words("vst_setsize32", "sizes", W(a, b, c, d), W(0x200, 0x201, 0x202, 0x203), 4);

	t_run("vst_skew");
	v_fake(1, 0);
	r = vst_skew(FH, -200);
	v_off();
	V_IN("vst_skew", 253, -1, 0, 1, W(-200), 0);
	CHECK_EQ("vst_skew", r, 0x100);

	t_run("vst_charmap");
	v_fake(0, 0);
	vst_charmap(FH, 0);
	v_off();
	V_IN("vst_charmap", 236, -1, 0, 1, W(0), 0);

	// v_getbitmap_info: intout = width, height, advance x, y, offset x, y (fix31), bitmap pointer.
	t_run("v_getbitmap_info");
	fix31 ax = 0, ay = 0, ox = 0, oy = 0;
	short* bm = 0;
	a = b = 0;
	v_fake(12, 0);
	v_getbitmap_info(FH, 'g', &ax, &ay, &ox, &oy, &a, &b, &bm);
	v_off();
	V_IN("v_getbitmap_info", 239, -1, 0, 1, W('g'), 0);
	t_check(a == 0x100 && b == 0x101 && ax == 0x01020103L && ay == 0x01040105L && ox == 0x01060107L && oy == 0x01080109L && (long)bm == 0x010a010bL,
		"v_getbitmap_info", "w %x h %x adv %08lx %08lx off %08lx %08lx bitmap %08lx", a, b, (long)ax, (long)ay, (long)ox, (long)oy, (long)bm);

	t_run("vqt_pairkern");
	fx = fy = 0;
	v_fake(4, 0);
	vqt_pairkern(FH, 'A', 'V', &fx, &fy);
	v_off();
	V_IN("vqt_pairkern", 235, -1, 0, 2, W('A', 'V'), 0);
	t_check(fx == 0x01000101L && fy == 0x01020103L, "vqt_pairkern", "%08lx %08lx", (long)fx, (long)fy);

	t_run("vqt_trackkern");
	fx = fy = 0;
	v_fake(4, 0);
	vqt_trackkern(FH, &fx, &fy);
	v_off();
	V_IN("vqt_trackkern", 234, -1, 0, 0, 0, 0);
	t_check(fx == 0x01000101L && fy == 0x01020103L, "vqt_trackkern", "%08lx %08lx", (long)fx, (long)fy);

	// vqt_fontheader: intin = buffer pointer, the file name comes back in intout.
	t_run("vqt_fontheader");
	char hdr[16], path[40];
	memset(path, 'X', sizeof(path));
	v_fake(6, 0);
	memcpy(v_fake_intout, W('F', '.', 'S', 'P', 'D', 0), 12);
	vqt_fontheader(FH, hdr, path);
	v_off();
	V_IN("vqt_fontheader", 232, -1, 0, 2, 0, 0);
	t_check(ptr_at(&v_rec[0].intin[0]) == (long)hdr, "vqt_fontheader", "intin[0..1] = %08lx, expected %08lx", ptr_at(&v_rec[0].intin[0]), (long)hdr);
	t_check(strcmp(path, "F.SPD") == 0, "vqt_fontheader", "path \"%.20s\"", path);

	t_run("vst_kern");
	a = b = 0;
	v_fake(2, 0);
	vst_kern(FH, 1, 1, &a, &b);
	v_off();
	V_IN("vst_kern", 237, -1, 0, 2, W(1, 1), 0);
	v_words("vst_kern", "tracks, pairs", W(a, b), W(0x100, 0x101), 2);

	t_run("vq_devinfo");
	char f[20], dev[20];
	a = 0;
	v_fake(0, 0);
	vq_devinfo(FH, 21, &a, f, dev);
	v_off();
	V_IN("vq_devinfo", 248, -1, 0, 1, W(21), 0);
	t_run("vqt_devinfo");
	v_fake(0, 0);
	vqt_devinfo(FH, 22, &a, dev);
	v_off();
	V_IN("vqt_devinfo", 248, -1, 0, 1, W(22), 0);

	t_run("v_flushcache");
	v_fake(1, 0);
	r = v_flushcache(FH);
	v_off();
	V_IN("v_flushcache", 251, -1, 0, 0, 0, 0);
	CHECK_EQ("v_flushcache", r, 0x100);

	t_run("vqt_cachesize");
	int size = 0;
	v_fake(2, 0);
	vqt_cachesize(FH, 1, &size);
	v_off();
	V_IN("vqt_cachesize", 255, -1, 0, 1, W(1), 0);
	CHECK_EQ("vqt_cachesize", size, 0x01000101L);

	t_run("vqt_get_table");
	short* map = 0;
	v_fake(2, 0);
	vqt_get_table(FH, &map);
	v_off();
	V_IN("vqt_get_table", 254, -1, 0, 0, 0, 0);
	CHECK_EQ("vqt_get_table", (long)map, 0x01000101L);

	t_run("v_loadcache");
	v_fake(1, 0);
	r = v_loadcache(FH, "C.DAT", 1);
	v_off();
	V_IN("v_loadcache", 250, -1, 0, 6, W(1, 'C', '.', 'D', 'A', 'T'), 0);
	CHECK_EQ("v_loadcache", r, 0x100);

	t_run("v_savecache");
	v_fake(1, 0);
	r = v_savecache(FH, "C.DAT");
	v_off();
	V_IN("v_savecache", 249, -1, 0, 5, W('C', '.', 'D', 'A', 'T'), 0);
	CHECK_EQ("v_savecache", r, 0x100);
	(void)buf;
}

// Find a loaded vector font with a direct vqt_name call (intout[33] = 1 for vector fonts). Returns the font id or 0.
static short vector_font(short h, short nfonts, char* name)
{
	static V_RAW raw;
	for (short i = 1; i <= nfonts; i++)
	{
		memset(&raw, 0, sizeof(raw));
		raw.contrl[0] = 130;
		raw.contrl[3] = 1;
		raw.contrl[6] = h;
		raw.intin[0] = i;
		v_raw(&raw);
		if (raw.contrl[4] > 33 && raw.intout[33] != 0)
		{
			for (int j = 0; j < 32; j++)
			{
				name[j] = (char)raw.intout[1 + j];
			}
			name[32] = 0;
			return raw.intout[0];
		}
	}
	return 0;
}

static void real_tests(void)
{
	short h = v_handle, r, a, b, c, d;
	static V_RAW raw;
	if (v_gdos == GDOS_NONE)
	{
		t_skip("v_bez", "no GDOS");
		return;
	}
	v_scr_begin();
	short x = v_ox, y = v_oy;

	// Bezier: a curve from (x, y + 40) to (x + 60, y + 40) with control points above.
	t_run("v_bez_on");
	r = v_bez_on(h);
	t_check(r > 0, "v_bez_on", "v_bez_on() = %d (quality)", r);
	t_run("v_bez_qual");
	short q = -1;
	r = v_bez_qual(h, 100, &q);
	t_check(q >= 0 && q <= 100, "v_bez_qual", "v_bez_qual(100) = %d, actual %d", r, q);
	short bxy[8] = { x, (short)(y + 40), x, y, (short)(x + 60), y, (short)(x + 60), (short)(y + 40) };
	char bf[4] = { 1, 0, 0, 0 };
	short ext[4] = { 0 }, tp = -1, tm = -1;
	t_run("v_bez");
	v_clear(x, y, 70, 50);
	v_bez(h, 4, bxy, bf, ext, &tp, &tm);
	int top = v_count_ink(x + 20, y, 20, 15), ends = v_ink(x, y + 40) + v_ink(x + 60, y + 40);
	t_check(tp > 4 && top > 0 && ends == 2 && ext[0] == x && ext[2] == x + 60 && ext[3] == y + 40, "v_bez",
		"%d points %d moves, extent %d,%d %d,%d, %d pixels near the top", tp, tm, ext[0], ext[1], ext[2], ext[3], top);
	t_run("v_bez_fill");
	v_clear(x, y, 70, 50);
	v_bez_fill(h, 4, bxy, bf, ext, &tp, &tm);
	int inside = v_count_ink(x + 25, y + 25, 10, 10);
	CHECK_EQ("v_bez_fill", inside, 100);
	t_run("v_bez_off");
	v_bez_off(h);
	t_check(1, "v_bez_off", "returned");

	// Vector fonts.
	short n = vst_load_fonts(h, 0);
	char name[34];
	short id = vector_font(h, 1 + n, name);
	if (!id)
	{
		t_skip("vst_arbpt", "no vector font found in %d fonts", 1 + n);
		v_scr_end();
		return;
	}
	t_print("INFO vector font id %d \"%s\"", id, name);
	vst_font(h, id);

	t_run("vst_arbpt");
	r = vst_arbpt(h, 20, &a, &b, &c, &d);
	t_check(r == 20 && b > 5 && d >= b, "vst_arbpt", "vst_arbpt(20) = %d: char %dx%d cell %dx%d", r, a, b, c, d);
	t_run("vst_arbpt32");
	fix31 f = vst_arbpt32(h, 0x00168000L, &a, &b, &c, &d);
	t_check(f == 0x00168000L && b > 5, "vst_arbpt32", "vst_arbpt32(22.5) = %08lx: char %dx%d cell %dx%d", (long)f, a, b, c, d);
	t_run("vst_setsize");
	r = vst_setsize(h, 18, &a, &b, &c, &d);
	t_check(r == 18 && b > 5, "vst_setsize", "vst_setsize(18) = %d: char %dx%d cell %dx%d", r, a, b, c, d);
	t_run("vst_setsize32");
	f = vst_setsize32(h, 0x00140000L, &a, &b, &c, &d);
	t_check(f == 0x00140000L && b > 5, "vst_setsize32", "vst_setsize32(20) = %08lx: char %dx%d cell %dx%d", (long)f, a, b, c, d);
	t_run("vst_skew");
	r = vst_skew(h, 100);
	CHECK_EQ("vst_skew", r, 100);
	vst_skew(h, 0);

	t_run("vqt_advance");
	short ax = -1, ay = -1, rx, ry;
	vqt_advance(h, 'W', &ax, &ay, &rx, &ry);
	t_check(ax > 5 && ay == 0, "vqt_advance", "'W': advance %d,%d remainder %d,%d", ax, ay, rx, ry);
	t_run("vqt_advance32");
	fix31 fx = 0, fy = -1;
	vqt_advance32(h, 'W', &fx, &fy);
	t_check((fx >> 16) == ax && fy == 0, "vqt_advance32", "'W': advance %08lx %08lx", (long)fx, (long)fy);

	t_run("vqt_f_extent");
	short fe[8];
	vqt_f_extent(h, "Hello", fe);
	t_check(fe[2] - fe[0] > 20 && fe[7] - fe[1] > 5, "vqt_f_extent", "\"Hello\": %d,%d %d,%d %d,%d %d,%d", fe[0], fe[1], fe[2], fe[3], fe[4], fe[5], fe[6], fe[7]);

	short dummy;
	vst_alignment(h, 0, 5, &dummy, &dummy);
	t_run("v_ftext");
	v_clear(x, y, 100, 40);
	v_ftext(h, x + 2, y + 2, "Hello");
	int ink = v_count_ink(x, y, 100, 40);
	t_check(ink > 30, "v_ftext", "\"Hello\": %d pixels", ink);
	t_run("v_ftext_offset");
	v_clear(x, y, 100, 40);
	v_ftext_offset(h, x + 2, y + 2, "HH", W(0, 0, 40, 0));
	int left = v_count_ink(x, y, 30, 40), right = v_count_ink(x + 40, y, 40, 40);
	t_check(left > 10, "v_ftext_offset", "\"HH\" with offsets 0,0 40,0: %d pixels left, %d right", left, right);

	t_run("vst_kern");
	short tr = -1, pr = -1;
	vst_kern(h, 1, 1, &tr, &pr);
	t_check(tr >= 0 && pr >= 0, "vst_kern", "tracks %d pairs %d", tr, pr);
	t_run("vqt_trackkern");
	fx = fy = -1;
	vqt_trackkern(h, &fx, &fy);
	t_check(fy == 0, "vqt_trackkern", "%08lx %08lx", (long)fx, (long)fy);
	t_run("vqt_pairkern");
	fx = fy = -1;
	vqt_pairkern(h, 'A', 'V', &fx, &fy);
	t_check(fy == 0 && fx <= 0, "vqt_pairkern", "'AV': %08lx %08lx", (long)fx, (long)fy);
	vst_kern(h, 0, 0, &tr, &pr);

	t_run("vqt_fontheader");
	static char hdr[1024];
	char path[260];
	memset(path, 0, sizeof(path));
	vqt_fontheader(h, hdr, path);
	t_check(path[0] != 0, "vqt_fontheader", "path \"%.60s\"", path);

	t_run("vqt_cachesize");
	int size = -1;
	v_spy();
	vqt_cachesize(h, 0, &size);
	v_off();
	if (v_rec[0].nintout < 2)
	{
		t_skip("vqt_cachesize", "not implemented (%d intout words)", v_rec[0].nintout);
	}
	else
	{
		t_check(size >= 0, "vqt_cachesize", "largest free block %d", size);
	}
	t_run("vqt_get_table");
	short* map = 0;
	v_spy();
	vqt_get_table(h, &map);
	v_off();
	if (v_rec[0].nintout < 2)
	{
		t_skip("vqt_get_table", "not implemented (%d intout words)", v_rec[0].nintout);
	}
	else
	{
		t_check(map != 0, "vqt_get_table", "table at %08lx", (long)map);
	}

	t_run("v_flushcache");
	r = v_flushcache(h);
	t_check(r == 0, "v_flushcache", "v_flushcache() = %d", r);

	t_run("vst_error");
	short err = -1;
	vst_error(h, 1, &err);
	t_check(1, "vst_error", "error variable %d", err);
	vst_error(h, 0, &err);
	t_run("vst_scratch");
	vst_scratch(h, 0);
	t_check(1, "vst_scratch", "returned");
	t_run("vst_charmap");
	vst_charmap(h, 1);
	t_check(1, "vst_charmap", "returned");

	// Outline of 'H' (straight lines only): a few points inside the cell.
	t_run("v_getoutline");
	static short oxy[2 * 400];
	static char obez[400];
	short nv = -1;
	oxy[0] = -12345;
	v_getoutline(h, 'H', oxy, obez, 400, &nv);
	t_check(nv > 4 && nv <= 400 && oxy[0] != -12345, "v_getoutline", "'H': %d points, first %d,%d", nv, oxy[0], oxy[1]);

	// Devices: 31 is META.SYS in ASSIGN.SYS (not open, so "exists" is 0).
	t_run("vq_devinfo");
	char file[40], dev[40];
	short ex = -1;
	memset(file, 0, sizeof(file));
	memset(dev, 0, sizeof(dev));
	// The file name comes back in intout, the device name packed in ptsout.
	v_spy();
	vq_devinfo(h, 31, &ex, file, dev);
	v_off();
	t_print("INFO vq_devinfo(31): open %d file \"%.30s\" device \"%.30s\", VDI wrote %d intout %d ptsout words", ex, file, dev,
		v_rec[0].nintout, v_rec[0].nptsout * 2);
	t_check(strcmp(file, "META.SYS") == 0 && strncmp(dev, "Metafile", 8) == 0 && ex == 0, "vq_devinfo",
		"device 31: open %d file \"%.30s\" (expected META.SYS), device \"%.30s\"", ex, file, dev);

	// vqt_devinfo returns the device name. devexists is a single short, the words after it must stay untouched.
	t_run("vqt_devinfo");
	struct { short ex; short after[24]; } de;
	de.ex = -1;
	for (int i = 0; i < 24; i++) de.after[i] = CANARY;
	memset(dev, 0, sizeof(dev));
	vqt_devinfo(h, 31, &de.ex, dev);
	int over = 0;
	for (int i = 0; i < 24; i++) if (de.after[i] != CANARY) over = i + 1;
	t_print("INFO vqt_devinfo(31): exists %d device \"%.30s\"", de.ex, dev);
	t_check(over == 0, "vqt_devinfo", "%d words written after devexists", over);
	t_check(strncmp(dev, "Metafile", 8) == 0, "vqt_devinfo", "device 31: \"%.30s\"", dev);

	t_run("v_getbitmap_info");
	fix31 bx = 0, by = 0, bxo = 0, byo = 0;
	short bw = -1, bh = -1, *bm = 0;
	vst_arbpt(h, 20, &a, &b, &c, &d);
	// Direct call first, the binding has room for 12 intout words.
	memset(&raw, 0, sizeof(raw));
	raw.contrl[0] = 239;
	raw.contrl[3] = 1;
	raw.contrl[6] = h;
	raw.intin[0] = 'W';
	v_raw(&raw);
	t_print("INFO v_getbitmap_info direct: %d intout %d ptsout words", raw.contrl[4], raw.contrl[2]);
	v_getbitmap_info(h, 'W', &bx, &by, &bxo, &byo, &bw, &bh, &bm);
	t_check(bw > 5 && bh > 5 && bm != 0 && (bx >> 16) > 5, "v_getbitmap_info", "'W': %dx%d at %08lx, advance %08lx %08lx offset %08lx %08lx",
		bw, bh, (long)bm, (long)bx, (long)by, (long)bxo, (long)byo);

	vst_unload_fonts(h, 0);
	vst_font(h, 1);
	v_scr_end();
}

int main(void)
{
	if (v_begin("V_GDOS"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
