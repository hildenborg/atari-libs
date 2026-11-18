/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Event functions. Input is simulated (keyboard buffer, IKBD mouse packets).
	Calls that could wait forever get a delayed key (a_key_later) as a way out.
*/

#include <string.h>
#include "a_util.h"

static long ticks(void)
{
	return t_peekl(0x4ba);	// 200 Hz counter
}

static void test_simple(void)
{
	t_run("evnt_timer");
	long t0 = ticks();
	short r = evnt_timer(200, 0);
	long dt = ticks() - t0;
	CHECK_TRUE("evnt_timer", dt >= 36 && dt < 100, "evnt_timer(200 ms) = %d, took %ld ms", r, dt * 5);

	t_run("evnt_keybd");
	a_drain_keys();
	t_key(0x1e, 'a');
	CHECK_EQ("evnt_keybd", evnt_keybd(), 0x1e61);

	t_run("evnt_mesag");
	short msg[8] = { 0x7123, a_apid, 0, 11, 22, 33, 44, 55 }, rd[8];
	appl_write(a_apid, 16, msg);
	memset(rd, 0, sizeof(rd));
	evnt_mesag(rd);
	CHECK_TRUE("evnt_mesag", memcmp(msg, rd, 16) == 0, "message %04x %d %d read back", rd[0] & 0xffff, rd[3], rd[7]);

	t_run("evnt_dclick");
	short dc = evnt_dclick(0, 0);
	CHECK_TRUE("evnt_dclick", dc >= 0 && dc <= 4, "evnt_dclick(get) = %d", dc);
	CHECK_EQ("evnt_dclick", evnt_dclick(4, 1), 4);
	CHECK_EQ("evnt_dclick", evnt_dclick(0, 0), 4);
	t_run("evnt_dclicks");
	CHECK_EQ("evnt_dclicks", evnt_dclicks(1, 1), 1);
	CHECK_EQ("evnt_dclicks", evnt_dclicks(0, 0), 1);
	evnt_dclick(dc, 1);
}

static void test_mouse(void)
{
	short mx, my, b, k;
	t_run("evnt_button");
	a_mouse_to(120, 90);
	a_mouse_button(1);
	a_key_later(3000, 0x39, ' ');	// Safety net
	short r = evnt_button(1, 1, 1, &mx, &my, &b, &k);
	a_key_later_done();
	// The click count is 0 on TOS 2.06 when the button is already down.
	CHECK_TRUE("evnt_button", r >= 0 && mx == 120 && my == 90 && (b & 1), "evnt_button(down) = %d at %d,%d button %d", r, mx, my, b);
	a_mouse_button(0);
	a_key_later(3000, 0x39, ' ');
	r = evnt_button(1, 1, 0, &mx, &my, &b, &k);
	a_key_later_done();
	CHECK_TRUE("evnt_button", (b & 1) == 0, "evnt_button(up) = %d, button %d", r, b);

	t_run("evnt_mouse");
	a_mouse_to(50, 60);
	r = evnt_mouse(0, 40, 50, 30, 30, &mx, &my, &b, &k);	// Wait for inside: already inside
	// The return value is reserved (EmuTOS returns 1, TOS 4.04 returns 0).
	CHECK_TRUE("evnt_mouse", mx == 50 && my == 60, "evnt_mouse(enter 40,50 30x30) = %d at %d,%d", r, mx, my);
	r = evnt_mouse(1, 100, 100, 20, 20, &mx, &my, &b, &k);	// Wait for outside: already outside
	CHECK_TRUE("evnt_mouse", mx == 50 && my == 60, "evnt_mouse(leave 100,100 20x20) = %d at %d,%d", r, mx, my);
}

static void test_multi(void)
{
	short msg[8], mx, my, b, k, kr, br;
	t_run("evnt_multi");
	a_drain_keys();
	t_key(0x30, 'b');
	short r = evnt_multi(MU_KEYBD | MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 2000, 0, &mx, &my, &b, &k, &kr, &br);
	CHECK_TRUE("evnt_multi", r == MU_KEYBD && kr == 0x3062, "evnt_multi(key|timer) = 0x%x key 0x%04x", r, kr);

	long t0 = ticks();
	r = evnt_multi(MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 100, 0, &mx, &my, &b, &k, &kr, &br);
	CHECK_TRUE("evnt_multi", r == MU_TIMER && ticks() - t0 >= 16, "evnt_multi(timer 100 ms) = 0x%x after %ld ms", r, (ticks() - t0) * 5);

	short m[8] = { 0x7321, a_apid, 0, 1, 2, 3, 4, 5 };
	appl_write(a_apid, 16, m);
	memset(msg, 0, sizeof(msg));
	r = evnt_multi(MU_MESAG | MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 2000, 0, &mx, &my, &b, &k, &kr, &br);
	CHECK_TRUE("evnt_multi", r == MU_MESAG && msg[0] == 0x7321 && msg[7] == 5, "evnt_multi(message|timer) = 0x%x msg 0x%04x", r, msg[0] & 0xffff);

	a_mouse_to(30, 40);
	r = evnt_multi(MU_M1 | MU_TIMER, 0, 0, 0, 0, 20, 30, 40, 40, 0, 0, 0, 0, 0, msg, 2000, 0, &mx, &my, &b, &k, &kr, &br);
	CHECK_TRUE("evnt_multi", (r & MU_M1) && mx == 30 && my == 40, "evnt_multi(m1 enter|timer) = 0x%x at %d,%d", r, mx, my);

	a_mouse_button(1);
	r = evnt_multi(MU_BUTTON | MU_TIMER, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 2000, 0, &mx, &my, &b, &k, &kr, &br);
	a_mouse_button(0);
	a_wait(50);
	CHECK_TRUE("evnt_multi", (r & MU_BUTTON) && br >= 1 && mx == 30, "evnt_multi(button|timer) = 0x%x clicks %d", r, br);

	// The mt_ variant takes rectangles, a 32 bit timer and an EVNTDATA.
	// A key arrives after about 1.5 s: the 100 ms timer must come first.
	t_run("mt_evnt_multi");
	extern short _aes_global[16];
	EVNTDATA ev;
	GRECT g1 = { 20, 30, 40, 40 };
	a_drain_keys();
	a_key_later(1500, 0x1e, 'a');
	t0 = ticks();
	r = mt_evnt_multi(MU_TIMER | MU_KEYBD, 0, 0, 0, 0, &g1, 0, &g1, msg, 100, &ev, &kr, &br, _aes_global);
	long ms = (ticks() - t0) * 5;
	a_key_later_done();
	CHECK_TRUE("mt_evnt_multi", r == MU_TIMER && ms < 1000, "mt_evnt_multi(timer 100 ms|key) = 0x%x after %ld ms%s", r, ms,
		r == MU_KEYBD ? " (the timer value doesn't reach the AES as 100 ms)" : "");
	a_drain_keys();
	a_mouse_to(30, 40);
	r = mt_evnt_multi(MU_M1 | MU_TIMER, 0, 0, 0, 0, &g1, 0, &g1, msg, 100, &ev, &kr, &br, _aes_global);
	CHECK_TRUE("mt_evnt_multi", (r & MU_M1) && ev.x == 30 && ev.y == 40, "mt_evnt_multi(m1 enter) = 0x%x at %d,%d", r, ev.x, ev.y);
	a_drain_keys();

	// MT_EVNT_multi returns everything in an EVNT.
	t_run("MT_EVNT_multi");
	EVNT e;
	memset(&e, 0x55, sizeof(e));
	t_key(0x1e, 'a');
	MT_EVNT_multi(MU_KEYBD | MU_TIMER, 0, 0, 0, 0, 0, msg, 2000, &e, _aes_global);
	CHECK_TRUE("MT_EVNT_multi", e.mwhich == MU_KEYBD && e.key == 0x1e61, "MT_EVNT_multi(key|timer): mwhich 0x%x key 0x%04x mx %d",
		e.mwhich, e.key & 0xffff, e.mx);
	a_drain_keys();
	MOBLK m1 = { 0, 20, 30, 40, 40 };
	a_mouse_to(30, 40);
	memset(&e, 0x55, sizeof(e));
	MT_EVNT_multi(MU_M1 | MU_TIMER, 0, 0, 0, &m1, 0, msg, 2000, &e, _aes_global);
	CHECK_TRUE("MT_EVNT_multi", (e.mwhich & MU_M1) && e.mx == 30 && e.my == 40 && e.mbutton == 0,
		"MT_EVNT_multi(m1 enter): mwhich 0x%x at %d,%d button %d clicks %d", e.mwhich, e.mx, e.my, e.mbutton, e.mclicks);
	a_key_later(1500, 0x1e, 'a');
	t0 = ticks();
	MT_EVNT_multi(MU_TIMER | MU_KEYBD, 0, 0, 0, 0, 0, msg, 100, &e, _aes_global);
	ms = (ticks() - t0) * 5;
	a_key_later_done();
	CHECK_TRUE("MT_EVNT_multi", ms < 1000, "MT_EVNT_multi(timer 100 ms|key) returned after %ld ms", ms);
	a_drain_keys();
}

int main(void)
{
	if (!a_begin("A_EVNT"))
	{
		return t_end();
	}
	test_simple();
	test_mouse();
	test_multi();
	return a_end();
}
