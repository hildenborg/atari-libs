/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Control functions: open/close workstations, clear/update, fonts, clipping, GDOS queries.
*/

#include <string.h>
#include "v_util.h"

static const short wkin[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2 };

static void check_work_out(const char* fn, const short* wo)
{
	short want[57];
	for (int i = 0; i < 45; i++)
	{
		want[i] = 0x100 + i;
	}
	for (int i = 0; i < 12; i++)
	{
		want[45 + i] = 0x200 + i;
	}
	v_words(fn, "work_out", wo, want, 57);
}

static void binding_tests(void)
{
	short wo[60], h;

	// v_opnwk: intin = work_in (11 words), work_out = 45 intout words + 6 ptsout points, handle in contrl[6].
	t_run("v_opnwk");
	wo[57] = 0x5555;
	v_fake(45, 12);
	v_fake_contrl[6] = 0x77;
	v_fake_cmask = 1 << 6;
	v_opnwk((short*)wkin, &h, (WS*)wo);
	v_off();
	v_in("v_opnwk", 0, 1, -1, 0, 11, FH_ANY, wkin, 0);
	CHECK_EQ("v_opnwk", h, 0x77);
	check_work_out("v_opnwk", wo);
	CHECK_EQ("v_opnwk", wo[57], 0x5555);

	t_run("v_opnvwk");
	h = FH;
	v_fake(45, 12);
	v_fake_contrl[6] = 0x78;
	v_fake_cmask = 1 << 6;
	v_opnvwk((short*)wkin, &h, (WS*)wo);
	v_off();
	V_IN("v_opnvwk", 100, -1, 0, 11, wkin, 0);
	CHECK_EQ("v_opnvwk", h, 0x78);
	check_work_out("v_opnvwk", wo);

	t_run("v_clswk");
	v_fake(0, 0);
	v_clswk(FH);
	v_off();
	V_IN("v_clswk", 2, -1, 0, 0, 0, 0);

	t_run("v_clsvwk");
	v_fake(0, 0);
	v_clsvwk(FH);
	v_off();
	V_IN("v_clsvwk", 101, -1, 0, 0, 0, 0);

	t_run("v_clrwk");
	v_fake(0, 0);
	v_clrwk(FH);
	v_off();
	V_IN("v_clrwk", 3, -1, 0, 0, 0, 0);

	t_run("v_updwk");
	v_fake(0, 0);
	v_updwk(FH);
	v_off();
	V_IN("v_updwk", 4, -1, 0, 0, 0, 0);

	t_run("vst_load_fonts");
	v_fake(1, 0);
	short r = vst_load_fonts(FH, 0x55);
	v_off();
	V_IN("vst_load_fonts", 119, -1, 0, 1, W(0x55), 0);
	CHECK_EQ("vst_load_fonts", r, 0x100);

	t_run("vst_unload_fonts");
	v_fake(0, 0);
	vst_unload_fonts(FH, 0x56);
	v_off();
	V_IN("vst_unload_fonts", 120, -1, 0, 1, W(0x56), 0);

	t_run("vs_clip");
	short xy[4] = { 10, 20, 300, 150 };
	v_fake(0, 0);
	vs_clip(FH, 1, xy);
	v_off();
	V_IN("vs_clip", 129, -1, 2, 1, W(1), xy);
	v_fake(0, 0);
	vs_clip(FH, 0, 0);
	v_off();
	V_IN("vs_clip", 129, -1, 2, 1, W(0), 0);
}

static void real_tests(void)
{
	// GDOS queries: NVDI is a GDOS, the plain TOS/EmuTOS configurations have none.
	t_run("vq_gdos");
	short g = vq_gdos();
	t_run("vq_vgdos");
	long vg = vq_vgdos();
	if (v_nvdi)
	{
		t_check(g != 0, "vq_gdos", "vq_gdos() = %d with NVDI", g);
		t_check(vg != GDOS_NONE, "vq_vgdos", "vq_vgdos() = 0x%lx with NVDI", vg);
	}
	else
	{
		CHECK_EQ("vq_gdos", g, 0);
		CHECK_EQ("vq_vgdos", vg, GDOS_NONE);
	}

	// A second virtual workstation, with the same size as the first one.
	t_run("v_opnvwk");
	short wo[57], h = a_vdi;
	for (int i = 0; i < 57; i++)
	{
		wo[i] = -1;
	}
	short work_in[11] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
	v_opnvwk(work_in, &h, (WS*)wo);
	if (t_check(h > 0 && h != v_handle, "v_opnvwk", "second workstation handle = %d (first %d)", h, v_handle))
	{
		CHECK_EQ("v_opnvwk", wo[0], v_scr_w - 1);
		CHECK_EQ("v_opnvwk", wo[1], v_scr_h - 1);
		t_check(wo[13] >= 2, "v_opnvwk", "colours = %d", wo[13]);
		// ptsout part: minimum character height (work_out[46]) and maximum (work_out[48]).
		t_check(wo[46] > 0 && wo[48] >= wo[46], "v_opnvwk", "char heights %d..%d", wo[46], wo[48]);
		t_run("v_clsvwk");
		v_spy();
		v_clsvwk(h);
		v_off();
		v_in("v_clsvwk", 0, 101, -1, 0, 0, h, 0, 0);
	}

	// Clipping: fill a rectangle larger than the clip rectangle.
	v_scr_begin();
	short x = v_ox, y = v_oy;
	v_clear(x, y, 60, 40);
	t_run("vs_clip");
	short clip[4] = { (short)(x + 10), (short)(y + 10), (short)(x + 29), (short)(y + 19) };
	vs_clip(v_handle, 1, clip);
	short box[4] = { x, y, (short)(x + 59), (short)(y + 39) };
	vr_recfl(v_handle, box);
	vs_clip(v_handle, 0, 0);
	int in = v_count_ink(x + 10, y + 10, 20, 10);
	int all = v_count_ink(x, y, 60, 40);
	t_check(in == 200 && all == 200, "vs_clip", "clipped fill: %d of 200 pixels inside, %d in total", in, all);
	// Off again: the whole rectangle.
	vr_recfl(v_handle, box);
	all = v_count_ink(x, y, 60, 40);
	CHECK_EQ("vs_clip", all, 2400);

	// v_clrwk clears the whole screen (the desktop is redrawn afterwards).
	t_run("v_clrwk");
	v_clrwk(v_handle);
	all = v_count_ink(x, y, 60, 40);
	CHECK_EQ("v_clrwk", all, 0);
	t_run("v_updwk");
	v_updwk(v_handle);
	t_check(1, "v_updwk", "returned");
	v_scr_end();

	// Fonts need GDOS (EmuTOS crashes on vst_load_fonts without it).
	if (v_gdos == GDOS_NONE)
	{
		t_skip("vst_load_fonts", "no GDOS");
		t_skip("vst_unload_fonts", "no GDOS");
		return;
	}
	t_run("vst_load_fonts");
	short n = vst_load_fonts(v_handle, 0);
	t_check(n > 0, "vst_load_fonts", "vst_load_fonts() = %d", n);
	// The loaded fonts are now in the list after the system font (checked with a direct vqt_name call).
	static V_RAW raw;
	memset(&raw, 0, sizeof(raw));
	raw.contrl[0] = 130;
	raw.contrl[3] = 1;
	raw.contrl[6] = v_handle;
	raw.intin[0] = 1 + n;
	v_raw(&raw);
	t_check(raw.intout[0] > 1, "vst_load_fonts", "font %d after loading: id %d", 1 + n, raw.intout[0]);
	t_run("vst_unload_fonts");
	vst_unload_fonts(v_handle, 0);
	t_check(1, "vst_unload_fonts", "returned");
}

int main(void)
{
	if (v_begin("V_CTRL"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
