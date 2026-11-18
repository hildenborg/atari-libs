/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Helpers for the AES tests. The programs run under GEM (started from the desktop by run.sh),
	output and results go through the TOS test harness (NatFeats).
	User input is simulated: keys go into the BIOS keyboard buffer, mouse packets are sent
	to the IKBD mouse vector, so the AES sees them as real input.
*/

#ifndef A_UTIL_DEFINED
#define A_UTIL_DEFINED

#include <aes.h>
#include "harness.h"

extern short a_apid;
extern short a_aesver;		// global[0]
extern int a_agi;			// appl_getinfo is available
extern short a_wchar, a_hchar, a_wbox, a_hbox, a_vdi;
extern GRECT a_desk;		// Desktop work area

// appl_init and environment, returns non zero when the AES is usable.
int a_begin(const char* prog);
int a_end(void);

// appl_getinfo, or 0 when it isn't available.
int a_getinfo(short type, short* o1, short* o2, short* o3, short* o4);

// Mouse: move to an absolute position, press/release the left button, click.
void a_mouse_to(short x, short y);
void a_mouse_button(int down);
void a_click(short x, short y);
void a_mouse_pos(short* x, short* y);

// Empty the BIOS keyboard buffer and the AES keyboard queue.
void a_drain_keys(void);

// Let the AES process pending input.
void a_wait(short ms);

// Put a key into the keyboard buffer from a timer interrupt after about ms milliseconds,
// while the program is blocked in an AES call (a modal dialog, an event wait).
// a_key_later_done() stops the timer and returns non zero if the key was sent.
void a_key_later(short ms, unsigned char scancode, unsigned char ascii);
int a_key_later_done(void);

// The general form: a list of steps (each delay counted from the previous step), then start.
// a_later_mouse sends a relative mouse packet with the left button down or up.
// a_later_done() stops the timer and returns how many steps were done.
void a_later_key(short ms, unsigned char scancode, unsigned char ascii);
void a_later_mouse(short ms, short dx, short dy, int down);
void a_later_start(void);
// Type a string with the timer: the first key after first_ms, then one key every 100 ms.
void a_later_type(short first_ms, const char* str);
int a_later_done(void);

// Object tree building, coordinates in characters (converted with rsrc_obfix by a_fix_tree).
void a_obj(OBJECT* tree, short i, short next, short head, short tail, unsigned short type, unsigned short flags,
	unsigned short state, void* spec, short x, short y, short w, short h);
void a_fix_tree(OBJECT* tree, short count);

#endif
