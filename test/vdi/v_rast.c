/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Raster functions.
*/

#include "v_util.h"

static long ptr_at(const short* w)
{
	return ((long)(unsigned short)w[0] << 16) | (unsigned short)w[1];
}

static void binding_tests(void)
{
	MFDB src, dst;
	short xy[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	t_run("vro_cpyfm");
	v_fake(0, 0);
	vro_cpyfm(FH, S_XOR_D, xy, &src, &dst);
	v_off();
	V_IN("vro_cpyfm", 109, -1, 4, 1, W(S_XOR_D), xy);
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)&src && ptr_at(&v_rec[0].contrl[9]) == (long)&dst, "vro_cpyfm",
		"contrl[7..10] = %08lx %08lx, expected %08lx %08lx", ptr_at(&v_rec[0].contrl[7]), ptr_at(&v_rec[0].contrl[9]), (long)&src, (long)&dst);

	// vrt_cpyfm: intin = mode, the two colour indices.
	t_run("vrt_cpyfm");
	v_fake(0, 0);
	vrt_cpyfm(FH, MD_TRANS, xy, &src, &dst, W(4, 5));
	v_off();
	V_IN("vrt_cpyfm", 121, -1, 4, 3, W(MD_TRANS, 4, 5), xy);
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)&src && ptr_at(&v_rec[0].contrl[9]) == (long)&dst, "vrt_cpyfm",
		"contrl[7..10] = %08lx %08lx, expected %08lx %08lx", ptr_at(&v_rec[0].contrl[7]), ptr_at(&v_rec[0].contrl[9]), (long)&src, (long)&dst);

	t_run("vr_trnfm");
	v_fake(0, 0);
	vr_trnfm(FH, &src, &dst);
	v_off();
	V_IN("vr_trnfm", 110, -1, 0, 0, 0, 0);
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)&src && ptr_at(&v_rec[0].contrl[9]) == (long)&dst, "vr_trnfm",
		"contrl[7..10] = %08lx %08lx, expected %08lx %08lx", ptr_at(&v_rec[0].contrl[7]), ptr_at(&v_rec[0].contrl[9]), (long)&src, (long)&dst);

	// v_get_pixel: intout[0] = pixel value, intout[1] = VDI colour index.
	t_run("v_get_pixel");
	short p = 0, v = 0;
	v_fake(2, 0);
	v_get_pixel(FH, 5, 6, &p, &v);
	v_off();
	V_IN("v_get_pixel", 105, -1, 1, 0, 0, W(5, 6));
	v_words("v_get_pixel", "outputs", W(p, v), W(0x100, 0x101), 2);
}

// Pixel from a memory form in device format (same layout as the screen) or standard format.
static unsigned short form_pixel(const MFDB* m, short x, short y)
{
	const unsigned short* base = (const unsigned short*)m->fd_addr;
	if (m->fd_nplanes >= 16)
	{
		return base[(long)y * m->fd_wdwidth * 16 + x];
	}
	unsigned short bit = 0x8000 >> (x & 15), v = 0;
	for (int p = 0; p < m->fd_nplanes; p++)
	{
		unsigned short w;
		if (m->fd_stand)
		{
			w = base[(long)p * m->fd_wdwidth * m->fd_h + (long)y * m->fd_wdwidth + (x >> 4)];
		}
		else
		{
			w = base[((long)y * m->fd_wdwidth + (x >> 4)) * m->fd_nplanes + p];
		}
		if (w & bit)
		{
			v |= 1 << p;
		}
	}
	return v;
}

static unsigned short buf1[32 * 16 * 16], buf2[32 * 16 * 16];

static void real_tests(void)
{
	short h = v_handle;
	short x = v_ox, y = v_oy;
	int bad;

	v_scr_begin();

	// A test picture: a diagonal line and a filled box.
	unsigned short white = v_clear(x, y, 80, 40);
	v_pline(h, 2, W(x, y, x + 15, y + 15));
	vr_recfl(h, W(x + 20, y + 2, x + 27, y + 9));

	t_run("v_get_pixel");
	short pel = -1, idx = -1;
	v_get_pixel(h, x + 3, y + 3, &pel, &idx);
	if (v_scr_planes <= 8)
	{
		t_check(pel == (short)v_pixel(x + 3, y + 3) && idx == 1, "v_get_pixel", "black pixel: value %d (screen %d), index %d", pel, v_pixel(x + 3, y + 3), idx);
		v_get_pixel(h, x + 3, y + 4, &pel, &idx);
		t_check(pel == (short)white && idx == 0, "v_get_pixel", "white pixel: value %d (screen %d), index %d", pel, white, idx);
	}
	else
	{
		t_check(pel == (short)v_pixel(x + 3, y + 3), "v_get_pixel", "black pixel: value 0x%x (screen 0x%x)", (unsigned short)pel, v_pixel(x + 3, y + 3));
	}

	// Screen to memory in device format.
	MFDB scr = { 0 };
	MFDB dev = { buf1, 32, 16, 2, 0, v_scr_planes, 0, 0, 0 };
	t_run("vro_cpyfm");
	vro_cpyfm(h, S_ONLY, W(x, y, x + 31, y + 15, 0, 0, 31, 15), &scr, &dev);
	bad = 0;
	for (short j = 0; j < 16; j++)
	{
		for (short i = 0; i < 32; i++)
		{
			bad += form_pixel(&dev, i, j) != v_pixel(x + i, y + j);
		}
	}
	t_check(bad == 0, "vro_cpyfm", "screen to memory: %d of 512 pixels differ", bad);

	// Device to standard format.
	MFDB std = { buf2, 32, 16, 2, 1, v_scr_planes, 0, 0, 0 };
	t_run("vr_trnfm");
	if (v_scr_planes >= 16)
	{
		t_skip("vr_trnfm", "no standard format check for %d planes", v_scr_planes);
	}
	else
	{
		vr_trnfm(h, &dev, &std);
		bad = 0;
		for (short j = 0; j < 16; j++)
		{
			for (short i = 0; i < 32; i++)
			{
				bad += form_pixel(&std, i, j) != form_pixel(&dev, i, j);
			}
		}
		t_check(bad == 0, "vr_trnfm", "device to standard: %d of 512 pixels differ", bad);
	}

	// Memory back to the screen, at x + 40.
	vro_cpyfm(h, S_ONLY, W(0, 0, 31, 15, x + 40, y, x + 71, y + 15), &dev, &scr);
	bad = 0;
	for (short j = 0; j < 16; j++)
	{
		for (short i = 0; i < 32; i++)
		{
			bad += v_pixel(x + 40 + i, y + j) != v_pixel(x + i, y + j);
		}
	}
	t_check(bad == 0, "vro_cpyfm", "memory to screen: %d of 512 pixels differ", bad);

	// Monochrome form to the screen with colours: set bits become colour 1, clear bits colour 0.
	t_run("vrt_cpyfm");
	unsigned short mono[16 * 2];
	for (int j = 0; j < 16; j++)
	{
		mono[j * 2] = (unsigned short)(0xffff >> j);
		mono[j * 2 + 1] = 0;
	}
	MFDB m1 = { mono, 32, 16, 2, 0, 1, 0, 0, 0 };
	vsf_color(h, 1);
	vr_recfl(h, W(x, y + 20, x + 31, y + 35));		// black background, clear bits must make it white
	vrt_cpyfm(h, MD_REPLACE, W(0, 0, 31, 15, x, y + 20, x + 31, y + 35), &m1, &scr, W(1, 0));
	bad = 0;
	for (short j = 0; j < 16; j++)
	{
		for (short i = 0; i < 32; i++)
		{
			int set = i < 16 && i >= j;
			bad += v_ink(x + i, y + 20 + j) != set;
		}
	}
	t_check(bad == 0, "vrt_cpyfm", "monochrome to screen: %d of 512 pixels wrong", bad);

	v_scr_end();
}

int main(void)
{
	if (v_begin("V_RAST"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
