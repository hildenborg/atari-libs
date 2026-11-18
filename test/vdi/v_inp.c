/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Input functions: modes, mouse, keyboard state, the vex_ vectors and the mouse form.
	Request and sample mode input with simulated keys is in v_req.c.
*/

#include <string.h>
#include "v_util.h"

static long ptr_at(const short* w)
{
	return ((long)(unsigned short)w[0] << 16) | (unsigned short)w[1];
}

/*
	Vector routines: count the calls, remember d0/d1, and continue with the old routine.
*/
volatile long tim_count, but_count, mot_count, cur_count;
volatile short but_state, mot_x, mot_y, cur_x, cur_y;
INT16_VDI_CB tim_old, but_old, mot_old, cur_old;
__asm__(
	".globl tim_handler\n"
	"tim_handler:\n\t"
	"addq.l	#1, tim_count\n\t"
	"move.l	tim_old, %sp@-\n\t"
	"rts\n"
	".globl but_handler\n"
	"but_handler:\n\t"
	"addq.l	#1, but_count\n\t"
	"move.w	%d0, but_state\n\t"
	"move.l	but_old, %sp@-\n\t"
	"rts\n"
	".globl mot_handler\n"
	"mot_handler:\n\t"
	"addq.l	#1, mot_count\n\t"
	"move.w	%d0, mot_x\n\t"
	"move.w	%d1, mot_y\n\t"
	"move.l	mot_old, %sp@-\n\t"
	"rts\n"
	".globl cur_handler\n"
	"cur_handler:\n\t"
	"addq.l	#1, cur_count\n\t"
	"move.w	%d0, cur_x\n\t"
	"move.w	%d1, cur_y\n\t"
	"move.l	cur_old, %sp@-\n\t"
	"rts\n");
short tim_handler(void);
short but_handler(void);
short mot_handler(void);
short cur_handler(void);

static void vex_binding(const char* fn, void (*f)(short, INT16_VDI_CB, INT16_VDI_CB*), short op)
{
	t_run(fn);
	INT16_VDI_CB old = 0;
	v_fake(0, 0);
	v_fake_contrl[9] = 0x0012;
	v_fake_contrl[10] = 0x3456;
	v_fake_cmask = (1 << 9) | (1 << 10);
	f(FH, (INT16_VDI_CB)mot_handler, &old);
	v_off();
	V_IN(fn, op, -1, 0, 0, 0, 0);
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)mot_handler, fn, "contrl[7..8] = %08lx, expected %08lx", ptr_at(&v_rec[0].contrl[7]), (long)mot_handler);
	CHECK_EQ(fn, (long)old, 0x00123456L);
}

static void binding_tests(void)
{
	short r, a = 0, b = 0, c = 0, d = 0;

	t_run("vsin_mode");
	v_fake(1, 0);
	r = vsin_mode(FH, 4, 2);
	v_off();
	V_IN("vsin_mode", 33, -1, 0, 2, W(4, 2), 0);
	CHECK_EQ("vsin_mode", r, 0x100);

	t_run("vrq_locator");
	v_fake(1, 2);
	vrq_locator(FH, 10, 20, &a, &b, &c);
	v_off();
	V_IN("vrq_locator", 28, -1, 1, 0, 0, W(10, 20));
	v_words("vrq_locator", "x, y, term", W(a, b, c), W(0x200, 0x201, 0x100), 3);

	// vsm_locator returns bit 0 when coordinates were returned (contrl[2]), bit 1 for a key (contrl[4]).
	t_run("vsm_locator");
	a = b = c = 0;
	v_fake(1, 2);
	r = vsm_locator(FH, 11, 21, &a, &b, &c);
	v_off();
	V_IN("vsm_locator", 28, -1, 1, 0, 0, W(11, 21));
	CHECK_EQ("vsm_locator", r, 3);
	v_words("vsm_locator", "x, y, term", W(a, b, c), W(0x200, 0x201, 0x100), 3);
	v_fake(0, 2);
	r = vsm_locator(FH, 11, 21, &a, &b, &c);
	v_off();
	CHECK_EQ("vsm_locator", r, 1);
	v_fake(1, 0);
	r = vsm_locator(FH, 11, 21, &a, &b, &c);
	v_off();
	CHECK_EQ("vsm_locator", r, 2);

	t_run("vrq_valuator");
	a = b = 0;
	v_fake(2, 0);
	vrq_valuator(FH, 50, &a, &b);
	v_off();
	V_IN("vrq_valuator", 29, -1, 0, 1, W(50), 0);
	v_words("vrq_valuator", "value, term", W(a, b), W(0x100, 0x101), 2);

	// vsm_valuator: status = contrl[4] (0 nothing, 1 value changed, 2 key).
	t_run("vsm_valuator");
	a = b = c = 0;
	v_fake(2, 0);
	vsm_valuator(FH, 51, &a, &b, &c);
	v_off();
	V_IN("vsm_valuator", 29, -1, 0, 1, W(51), 0);
	v_words("vsm_valuator", "value, term, status", W(a, b, c), W(0x100, 0x101, 2), 3);
	v_fake(1, 0);
	vsm_valuator(FH, 51, &a, &b, &c);
	v_off();
	CHECK_EQ("vsm_valuator", c, 1);

	t_run("vrq_choice");
	a = 0;
	v_fake(1, 0);
	vrq_choice(FH, 3, &a);
	v_off();
	V_IN("vrq_choice", 30, -1, 0, 1, W(3), 0);
	CHECK_EQ("vrq_choice", a, 0x100);

	t_run("vsm_choice");
	a = 0;
	v_fake(1, 0);
	r = vsm_choice(FH, &a);
	v_off();
	V_IN("vsm_choice", 30, -1, 0, 0, 0, 0);
	v_words("vsm_choice", "status, choice", W(r, a), W(1, 0x100), 2);

	// String input: intin = max length, echo mode; ptsin = echo position; the characters come back in intout.
	char str[20];
	t_run("vrq_string");
	memset(str, 'X', sizeof(str));
	v_fake(4, 0);
	v_fake_intout[0] = 'a';
	v_fake_intout[1] = 0xe4;
	v_fake_intout[2] = 'b';
	v_fake_intout[3] = 'c';
	vrq_string(FH, 10, 1, W(30, 40), str);
	v_off();
	V_IN("vrq_string", 31, -1, 1, 2, W(10, 1), W(30, 40));
	t_check(memcmp(str, "a\xe4" "bc", 5) == 0, "vrq_string", "string \"%.10s\"", str);

	t_run("vsm_string");
	memset(str, 'X', sizeof(str));
	v_fake(3, 0);
	v_fake_intout[0] = 'x';
	v_fake_intout[1] = 'y';
	v_fake_intout[2] = 'z';
	r = vsm_string(FH, 10, 0, W(31, 41), str);
	v_off();
	V_IN("vsm_string", 31, -1, 1, 2, W(10, 0), W(31, 41));
	t_check(r == 3 && memcmp(str, "xyz", 4) == 0, "vsm_string", "%d characters \"%.10s\"", r, str);

	// vsc_form: intin = the 37 words of the MFORM.
	t_run("vsc_form");
	MFORM mf;
	short* mw = (short*)&mf;
	for (int i = 0; i < 37; i++)
	{
		mw[i] = (short)(i * 3 + 1);
	}
	v_fake(0, 0);
	vsc_form(FH, &mf);
	v_off();
	V_IN("vsc_form", 111, -1, 0, 37, mw, 0);

	// vex_timv: new routine in contrl[7..8], old routine back in contrl[9..10], ms per tick in intout[0].
	t_run("vex_timv");
	INT16_VDI_CB old = 0;
	a = 0;
	v_fake(1, 0);
	v_fake_contrl[9] = 0x0012;
	v_fake_contrl[10] = 0x3457;
	v_fake_cmask = (1 << 9) | (1 << 10);
	vex_timv(FH, (INT16_VDI_CB)tim_handler, &old, &a);
	v_off();
	V_IN("vex_timv", 118, -1, 0, 0, 0, 0);
	t_check(ptr_at(&v_rec[0].contrl[7]) == (long)tim_handler, "vex_timv", "contrl[7..8] = %08lx, expected %08lx", ptr_at(&v_rec[0].contrl[7]), (long)tim_handler);
	CHECK_EQ("vex_timv", (long)old, 0x00123457L);
	CHECK_EQ("vex_timv", a, 0x100);

	vex_binding("vex_butv", vex_butv, 125);
	vex_binding("vex_motv", vex_motv, 126);
	vex_binding("vex_curv", vex_curv, 127);

	t_run("v_show_c");
	v_fake(0, 0);
	v_show_c(FH, 1);
	v_off();
	V_IN("v_show_c", 122, -1, 0, 1, W(1), 0);

	t_run("v_hide_c");
	v_fake(0, 0);
	v_hide_c(FH);
	v_off();
	V_IN("v_hide_c", 123, -1, 0, 0, 0, 0);

	t_run("vq_mouse");
	a = b = c = 0;
	v_fake(1, 2);
	vq_mouse(FH, &a, &b, &c);
	v_off();
	V_IN("vq_mouse", 124, -1, 0, 0, 0, 0);
	v_words("vq_mouse", "status, x, y", W(a, b, c), W(0x100, 0x200, 0x201), 3);

	t_run("vq_key_s");
	a = 0;
	v_fake(1, 0);
	vq_key_s(FH, &a);
	v_off();
	V_IN("vq_key_s", 128, -1, 0, 0, 0, 0);
	CHECK_EQ("vq_key_s", a, 0x100);
	(void)d;
}

static long kbshift(short mode)
{
	register long result asm ("d0");
	__asm__ volatile (
		"move.w	%1, %%sp@-\n\t"
		"move.w	#11, %%sp@-\n\t"
		"trap	#13\n\t"
		"addq.l	#4, %%sp\n\t"
		: "=r" (result)
		: "r" (mode)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}

static void real_tests(void)
{
	short h = v_handle, r, m = -1, st, mx, my;
	INT16_VDI_CB dummy;

	// Input modes: set the locator to sample mode and back.
	t_run("vsin_mode");
	r = vsin_mode(h, 1, 2);
	CHECK_EQ("vsin_mode", r, 2);
	// The ROM VDIs return the mode - 1.
	short rom = v_nvdi ? 0 : 1;
	t_run("vqin_mode");
	v_spy();
	vqin_mode(h, 1, &m);
	v_off();
	v_maps("vqin_mode", 0, "mode", &m, W(0), 1, 1);
	CHECK_EQ("vqin_mode", m, 2 - rom);

	// Sample mode locator: the current mouse position.
	a_mouse_to(120, 90);
	t_run("vsm_locator");
	short lx = -1, ly = -1, term = -1;
	r = vsm_locator(h, 120, 90, &lx, &ly, &term);
	t_check((r & 1) && lx == 120 && ly == 90, "vsm_locator", "vsm_locator() = %d at %d,%d (mouse 120,90)", r, lx, ly);
	vsin_mode(h, 1, 1);
	vqin_mode(h, 1, &m);
	CHECK_EQ("vqin_mode", m, 1 - rom);

	t_run("vq_mouse");
	a_mouse_to(100, 80);
	st = -1;
	vq_mouse(h, &st, &mx, &my);
	t_check(st == 0 && mx == 100 && my == 80, "vq_mouse", "status %d at %d,%d (expected 0 at 100,80)", st, mx, my);

	// Shift state: set right shift in the BIOS.
	t_run("vq_key_s");
	short old_shift = (short)kbshift(-1);
	kbshift(1);
	st = -1;
	vq_key_s(h, &st);
	kbshift(old_shift);
	CHECK_EQ("vq_key_s", st, 1);

	// Timer vector: the routine is called every tick.
	t_run("vex_timv");
	short conv = -1;
	tim_count = 0;
	vex_timv(h, (INT16_VDI_CB)tim_handler, &tim_old, &conv);
	a_wait(300);
	long ticks = tim_count;
	vex_timv(h, tim_old, &dummy, &conv);
	t_check(conv >= 5 && conv <= 50 && ticks >= 300 / conv / 2, "vex_timv", "%ld calls in 300 ms, %d ms per tick", ticks, conv);
	t_check(dummy == (INT16_VDI_CB)tim_handler, "vex_timv", "restored, the replaced routine is ours");
	a_wait(100);
	CHECK_EQ("vex_timv", tim_count, ticks);

	// Motion vector: called with the new position in d0/d1.
	t_run("vex_motv");
	mot_count = 0;
	vex_motv(h, (INT16_VDI_CB)mot_handler, &mot_old);
	a_mouse_to(140, 70);
	vex_motv(h, mot_old, &dummy);
	t_check(mot_count > 0 && mot_x == 140 && mot_y == 70, "vex_motv", "%ld calls, last at %d,%d (expected 140,70)", mot_count, mot_x, mot_y);
	t_check(dummy == (INT16_VDI_CB)mot_handler, "vex_motv", "restored, the replaced routine is ours");

	// Button vector: called with the button state in d0.
	t_run("vex_butv");
	but_count = 0;
	but_state = -1;
	vex_butv(h, (INT16_VDI_CB)but_handler, &but_old);
	a_mouse_button(1);
	a_wait(50);
	short down = but_state;
	a_mouse_button(0);
	a_wait(50);
	vex_butv(h, but_old, &dummy);
	if (but_count == 0 && v_nvdi)
	{
		t_skip("vex_butv", "NVDI ignores simulated mouse buttons");
	}
	else
	{
		t_check(but_count >= 2 && down == 1 && but_state == 0, "vex_butv", "%ld calls, state %d when pressed, %d released", but_count, down, but_state);
	}
	t_check(dummy == (INT16_VDI_CB)but_handler, "vex_butv", "restored, the replaced routine is ours");

	// Cursor vector: called to draw the mouse cursor, only while the cursor is shown.
	t_run("vex_curv");
	wind_update(BEG_UPDATE);
	cur_count = 0;
	vex_curv(h, (INT16_VDI_CB)cur_handler, &cur_old);
	a_mouse_to(150, 100);
	t_check(cur_count > 0 && cur_x == 150 && cur_y == 100, "vex_curv", "%ld calls, last at %d,%d (expected 150,100)", cur_count, cur_x, cur_y);
	vex_curv(h, cur_old, &dummy);
	t_check(dummy == (INT16_VDI_CB)cur_handler, "vex_curv", "restored, the replaced routine is ours");

	// Hide/show: the hide counter is the Line-A variable M_HID_CT (-598). v_show_c with reset 0 shows the cursor at once,
	// with reset 1 it undoes one v_hide_c.
	short hc0 = v_linea(-598);
	t_run("v_hide_c");
	v_hide_c(h);
	short hc1 = v_linea(-598);
	v_hide_c(h);
	short hc2 = v_linea(-598);
	t_check(hc1 == hc0 + 1 && hc2 == hc0 + 2, "v_hide_c", "hide counter %d, %d, %d", hc0, hc1, hc2);
	t_run("v_show_c");
	v_show_c(h, 1);
	short hc3 = v_linea(-598);
	v_show_c(h, 0);
	short hc4 = v_linea(-598);
	t_check(hc3 == hc0 + 1 && hc4 == 0, "v_show_c", "hide counter %d after v_show_c(1), %d after v_show_c(0)", hc3, hc4);
	if (hc0 > 0)
	{
		for (short i = 0; i < hc0; i++) v_hide_c(h);
	}


	// Mouse form: the VDIs copy it to the Line-A variables M_POS_HX, M_POS_HY, M_PLANES (-856, -854, -852).
	t_run("vsc_form");
	v_hide_c(h);
	MFORM mf;
	memset(&mf, 0, sizeof(mf));
	mf.mf_xhot = 3;
	mf.mf_yhot = 5;
	mf.mf_nplanes = 1;
	mf.mf_fg = 1;
	mf.mf_bg = 0;
	for (int i = 0; i < 16; i++)
	{
		mf.mf_mask[i] = (short)0xffff;
		mf.mf_data[i] = (short)0x8001;
	}
	vsc_form(h, &mf);
	short hx = v_linea(-856), hy = v_linea(-854), mp = v_linea(-852);
	// EmuTOS keeps M_PLANES at 0.
	t_check(hx == 3 && hy == 5 && (mp == 1 || t_env.emutos), "vsc_form", "Line-A hot spot %d,%d planes %d (expected 3,5 1)", hx, hy, mp);
	graf_mouse(ARROW, 0);
	v_show_c(h, 0);
	wind_update(END_UPDATE);
	form_dial(FMD_FINISH, 0, 0, 0, 0, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	a_wait(100);
}

int main(void)
{
	if (v_begin("V_INP"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
