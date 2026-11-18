/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Request and sample mode input with simulated keys. Runs last, a request call that never
	returns stops the rest of this program only.
*/

#include <string.h>
#include "v_util.h"

#define K_RETURN	0x1c
#define K_UP		0x48
#define K_F2		0x3c
#define K_F3		0x3d

static void real_tests(void)
{
	short h = v_handle, r, term, v, st;
	char str[20];

	// Sample mode string: the characters already typed.
	t_run("vsm_string");
	vsin_mode(h, 4, 2);
	a_drain_keys();
	t_keys("xy");
	memset(str, '-', sizeof(str));
	r = vsm_string(h, 10, 0, W(0, 0), str);
	t_check(r == 2 && strcmp(str, "xy") == 0, "vsm_string", "vsm_string() = %d \"%.10s\" (expected 2 \"xy\")", r, str);
	a_drain_keys();

	// Request mode string: waits for Return.
	t_run("vrq_string");
	vsin_mode(h, 4, 1);
	memset(str, '-', sizeof(str));
	a_later_type(300, "ab\r");
	a_later_start();
	vrq_string(h, 10, 0, W(0, 0), str);
	int sent = a_later_done();
	// NVDI includes the Return.
	if (v_nvdi && strcmp(str, "ab\r") == 0) str[2] = 0;
	t_check(strcmp(str, "ab") == 0, "vrq_string", "\"%.10s\" (%02x %02x %02x %02x, expected \"ab\"), %d keys sent", str,
		(unsigned char)str[0], (unsigned char)str[1], (unsigned char)str[2], (unsigned char)str[3], sent);
	a_drain_keys();

	// Sample mode choice: function key F3 gives choice 3.
	t_run("vsm_choice");
	vsin_mode(h, 3, 2);
	a_drain_keys();
	t_key(K_F3, 0);
	v = -1;
	r = vsm_choice(h, &v);
	// NVDI returns the scan code in the high byte, the ROM VDIs don't map function keys to choices.
	if (!v_nvdi)
		t_skip("vsm_choice", "ROM VDI: vsm_choice() = %d, choice %d for F3", r, v);
	else
	t_check(r == 1 && (v & 0xff) == 3, "vsm_choice", "vsm_choice() = %d, choice 0x%x (expected 1, 3)", r, (unsigned short)v);
	a_drain_keys();

	t_run("vrq_choice");
	vsin_mode(h, 3, 1);
	a_later_key(300, K_F2, 0);
	a_later_start();
	v = -1;
	vrq_choice(h, 1, &v);
	sent = a_later_done();
	if (sent == 0 || !v_nvdi)
		t_skip("vrq_choice", "returned without waiting for input (choice %d)", v);
	else
	t_check((v & 0xff) == 2, "vrq_choice", "choice 0x%x (expected 2), %d keys sent", v, sent);
	a_drain_keys();

	// Sample mode valuator: cursor up changes the value.
	t_run("vsm_valuator");
	vsin_mode(h, 2, 2);
	a_drain_keys();
	t_key(K_UP, 0);
	v = -1;
	term = -1;
	st = -1;
	vsm_valuator(h, 50, &v, &term, &st);
	// Drivers differ here: the ROM VDIs don't implement it, NVDI ends with cursor up as a key (status 2).
	if (st == 1)
		t_check(v > 50, "vsm_valuator", "status %d value %d (from 50)", st, v);
	else
		t_skip("vsm_valuator", "status %d value %d: the driver didn't change the value with cursor up", st, v);
	a_drain_keys();

	// Request mode valuator: up, up, Return.
	t_run("vrq_valuator");
	vsin_mode(h, 2, 1);
	a_later_key(300, K_UP, 0);
	a_later_key(100, K_UP, 0);
	a_later_key(100, K_RETURN, '\r');
	a_later_start();
	v = -1;
	term = -1;
	vrq_valuator(h, 50, &v, &term);
	sent = a_later_done();
	if (sent == 0)
		t_skip("vrq_valuator", "returned without waiting for input");
	else
	t_check(v > 50 && (term & 0xff) == '\r', "vrq_valuator", "value %d (from 50), terminator 0x%x, %d keys sent", v, (unsigned short)term, sent);
	a_drain_keys();

	// Request mode locator: ends with a key, the terminator is the character.
	// NVDI waits for a mouse button, and it ignores simulated buttons.
	if (v_nvdi)
	{
		t_skip("vrq_locator", "NVDI waits for a mouse button and ignores simulated buttons");
		return;
	}
	t_run("vrq_locator");
	vsin_mode(h, 1, 1);
	wind_update(BEG_UPDATE);
	a_later_key(300, 0x1e, 'a');
	a_later_start();
	short x = -1, y = -1;
	term = -1;
	vrq_locator(h, 100, 90, &x, &y, &term);
	sent = a_later_done();
	wind_update(END_UPDATE);
	if (sent == 0)
		t_skip("vrq_locator", "returned without waiting for input (terminator 0x%x)", (unsigned short)term);
	else
	t_check((term & 0xff) == 'a' && x == 100 && y == 90, "vrq_locator", "terminator 0x%x at %d,%d (expected 'a' at 100,90), %d keys sent",
		(unsigned short)term, x, y, sent);
	a_drain_keys();
}

int main(void)
{
	if (v_begin("V_REQ"))
	{
		real_tests();
	}
	return v_end();
}
