/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include <string.h>
#include "a_util.h"

short a_apid;
short a_aesver;
int a_agi;
short a_wchar, a_hchar, a_wbox, a_hbox, a_vdi;
GRECT a_desk;

static short mbuttons;
static unsigned char packet[3];
static VOID_TOS_CB mousevec;

// Call the IKBD mouse vector with a relative mouse packet in a0, like the IKBD interrupt does
// (at interrupt level 6, so other interrupt code can't run in the middle of it).
static long send_packet_fn(void)
{
	__asm__ volatile (
		"movem.l	%%d0-%%d7/%%a0-%%a6, %%a7@-\n\t"
		"move.w	%%sr, %%a7@-\n\t"
		"ori.w	#0x0600, %%sr\n\t"
		"move.l	%0, %%a0\n\t"
		"move.l	%1, %%a1\n\t"
		"jsr	%%a1@\n\t"
		"move.w	%%a7@+, %%sr\n\t"
		"movem.l	%%a7@+, %%d0-%%d7/%%a0-%%a6\n\t"
		:
		: "g" (packet), "g" (mousevec)
		: "cc", "memory"
	);
	return 0;
}

static void send_packet(short dx, short dy)
{
	KBDVBASE* kb = Kbdvbase();
	mousevec = kb->kb_mousevec;
	packet[0] = (unsigned char)(0xf8 | (mbuttons ? 2 : 0));
	packet[1] = (unsigned char)dx;
	packet[2] = (unsigned char)dy;
	t_super(send_packet_fn);
}

void a_wait(short ms)
{
	evnt_timer(ms, 0);
}

void a_mouse_pos(short* x, short* y)
{
	short b, k;
	graf_mkstate(x, y, &b, &k);
}

void a_mouse_to(short x, short y)
{
	for (int i = 0; i < 40; i++)
	{
		short mx, my;
		a_mouse_pos(&mx, &my);
		short dx = x - mx, dy = y - my;
		if (dx == 0 && dy == 0)
		{
			return;
		}
		if (dx > 100) dx = 100;
		if (dx < -100) dx = -100;
		if (dy > 100) dy = 100;
		if (dy < -100) dy = -100;
		send_packet(dx, dy);
		a_wait(10);
	}
}

void a_mouse_button(int down)
{
	mbuttons = down;
	send_packet(0, 0);
	a_wait(60);	// EmuTOS needs a moment before the AES sees the new button state
}

void a_click(short x, short y)
{
	a_mouse_to(x, y);
	a_mouse_button(1);
	a_wait(30);
	a_mouse_button(0);
}

/*
	Delayed input: MFP Timer A (about 48 Hz) runs a list of steps, each one a key for the keyboard
	buffer or a mouse packet for the IKBD mouse vector, after its delay.
*/
typedef struct
{
	short ticks;		// Delay before this step
	short mouse;		// 1: mouse packet, 0: key
	long key;
	unsigned char packet[3];
} LATER;

static LATER later[8];
static volatile short later_count, later_pos, later_wait;
static volatile short later_sent;
static IOREC* later_iorec;
static VOID_TOS_CB later_mousevec;
static unsigned char* later_packet;

static void later_mouse(void)
{
	__asm__ volatile (
		"move.l	%0, %%a0\n\t"
		"move.l	%1, %%a1\n\t"
		"jsr	%%a1@\n\t"
		:
		: "g" (later_packet), "g" (later_mousevec)
		: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a3", "a4", "a5", "cc", "memory"
	);
}

void a_later_tick(void)
{
	if (later_pos >= later_count || --later_wait > 0)
	{
		return;
	}
	LATER* l = &later[later_pos++];
	if (l->mouse)
	{
		later_packet = l->packet;
		later_mouse();
	}
	else
	{
		IOREC* io = later_iorec;
		short tl = io->ibuftl + 4;
		if (tl >= io->ibufsiz)
		{
			tl = 0;
		}
		*(long*)((char*)io->ibuf + tl) = l->key;
		io->ibuftl = tl;
	}
	later_sent++;
	if (later_pos < later_count)
	{
		later_wait = later[later_pos].ticks;
	}
}

__asm__(
	".globl a_later_handler\n"
	"a_later_handler:\n\t"
	"movem.l	%d0-%d7/%a0-%a6, %a7@-\n\t"
	"jsr	a_later_tick\n\t"
	"movem.l	%a7@+, %d0-%d7/%a0-%a6\n\t"
	"bclr	#5, 0xfffffa0f\n\t"
	"rte\n");
void a_later_handler(void);

void a_later_key(short ms, unsigned char scancode, unsigned char ascii)
{
	if (later_count < 8)
	{
		LATER* l = &later[later_count++];
		l->ticks = (short)(ms / 20 + 1);
		l->mouse = 0;
		l->key = ((long)scancode << 16) | ascii;
	}
}

void a_later_mouse(short ms, short dx, short dy, int down)
{
	if (later_count < 8)
	{
		LATER* l = &later[later_count++];
		l->ticks = (short)(ms / 20 + 1);
		l->mouse = 1;
		l->packet[0] = (unsigned char)(0xf8 | (down ? 2 : 0));
		l->packet[1] = (unsigned char)dx;
		l->packet[2] = (unsigned char)dy;
	}
}

void a_later_type(short first_ms, const char* str)
{
	static const unsigned char scan[26] =
	{
		0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
		0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c
	};
	short ms = first_ms;
	while (*str)
	{
		unsigned char c = (unsigned char)*str++;
		unsigned char sc = c == '\r' ? 0x1c : (c >= 'a' && c <= 'z') ? scan[c - 'a'] : 0x39;
		a_later_key(ms, sc, c);
		ms = 100;
	}
}

void a_later_start(void)
{
	later_iorec = Iorec(1);
	later_mousevec = Kbdvbase()->kb_mousevec;
	later_sent = 0;
	later_pos = 0;
	later_wait = later_count ? later[0].ticks : 0;
	Xbtimer(0, 7, 255, a_later_handler);	// Timer A, /200, ~48 Hz
}

int a_later_done(void)
{
	Jdisint(13);
	Xbtimer(0, 0, 0, a_later_handler);
	int sent = later_sent;
	later_count = later_pos = 0;
	return sent;
}

void a_key_later(short ms, unsigned char scancode, unsigned char ascii)
{
	a_later_key(ms, scancode, ascii);
	a_later_start();
}

int a_key_later_done(void)
{
	return a_later_done();
}

int a_getinfo(short type, short* o1, short* o2, short* o3, short* o4)
{
	short d;
	if (!a_agi)
	{
		return 0;
	}
	return appl_getinfo(type, o1 ? o1 : &d, o2 ? o2 : &d, o3 ? o3 : &d, o4 ? o4 : &d);
}

int a_begin(const char* prog)
{
	extern short _aes_global[16];
	t_begin(prog);
	t_run("appl_init");
	a_apid = appl_init();
	a_aesver = _aes_global[0];
	if (!t_check(a_apid >= 0 && a_aesver >= 0x100, "appl_init", "appl_init() = %d, AES version 0x%04x", a_apid, a_aesver))
	{
		return 0;
	}
	// appl_getinfo exists from AES 4.0, or when appl_find("?AGI") finds it (e.g. WDIALOG installed).
	a_agi = a_aesver >= 0x400 || appl_find("?AGI") == 0;
	a_vdi = graf_handle(&a_wchar, &a_hchar, &a_wbox, &a_hbox);
	wind_get(0, WF_WORKXYWH, &a_desk.g_x, &a_desk.g_y, &a_desk.g_w, &a_desk.g_h);
	t_print("AES 0x%04x ap_id %d appl_getinfo %s, char %dx%d, desktop %d,%d %dx%d", a_aesver, a_apid, a_agi ? "yes" : "no",
		a_wchar, a_hchar, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	a_drain_keys();
	return 1;
}

// Empty the BIOS keyboard buffer and the AES keyboard queue (which survives between programs).
void a_drain_keys(void)
{
	short msg[8], d;
	t_flush_keys();
	for (int i = 0; i < 64; i++)
	{
		if (!(evnt_multi(MU_KEYBD | MU_TIMER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, msg, 0, 0, &d, &d, &d, &d, &d, &d) & MU_KEYBD))
		{
			break;
		}
	}
}

int a_end(void)
{
	a_drain_keys();
	t_run("appl_exit");
	short r = appl_exit();
	t_check(r != 0, "appl_exit", "appl_exit() = %d", r);
	return t_end();
}

void a_obj(OBJECT* tree, short i, short next, short head, short tail, unsigned short type, unsigned short flags,
	unsigned short state, void* spec, short x, short y, short w, short h)
{
	OBJECT* o = &tree[i];
	o->ob_next = next;
	o->ob_head = head;
	o->ob_tail = tail;
	o->ob_type = type;
	o->ob_flags = flags;
	o->ob_state = state;
	memcpy(&o->ob_spec, &spec, sizeof(spec));
	o->ob_x = x;
	o->ob_y = y;
	o->ob_width = w;
	o->ob_height = h;
}

void a_fix_tree(OBJECT* tree, short count)
{
	for (short i = 0; i < count; i++)
	{
		rsrc_obfix(tree, i);
	}
}
