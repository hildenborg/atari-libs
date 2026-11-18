/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	XBIOS tests (trap #14) for functions found on all machines.
	Floppy functions are in t_disk.c, TT/Falcon functions in t_ext.c, t_sound.c and t_dsp.c.
*/

#include <string.h>
#include "harness.h"

volatile long timer_count;
// MFP Timer A interrupt handler: count and clear the in-service bit.
__asm__(
	".globl timer_a_handler\n"
	"timer_a_handler:\n\t"
	"addq.l	#1, timer_count\n\t"
	"bclr	#5, 0xfffffa0f\n\t"
	"rte\n");
void timer_a_handler(void);

static unsigned long peekb_addr;
static long peekb_fn(void)
{
	return *(volatile unsigned char*)peekb_addr;
}
static unsigned char peekb(unsigned long addr)
{
	peekb_addr = addr;
	return (unsigned char)t_super(peekb_fn);
}

static void wait_vbl(int n)
{
	while (n-- > 0)
	{
		Vsync();
	}
}

static void test_screen(void)
{
	t_run("Physbase");
	void* phys = Physbase();
	CHECK_TRUE("Physbase", phys != 0 && ((long)phys & 1) == 0, "Physbase() = %p", phys);
	t_run("Logbase");
	void* log = Logbase();
	CHECK_EQ("Logbase", (long)log, t_peekl(0x44e));
	t_run("Getrez");
	short rez = Getrez();
	if (t_env.mch < MCH_TT)
	{
		CHECK_EQ("Getrez", rez, peekb(0x44c));
	}
	else
	{
		CHECK_TRUE("Getrez", rez >= 0 && rez <= 7, "Getrez() = %d", rez);
	}

	t_run("Setscreen");
	char* mem = Malloc(32000 + 256);
	char* newlog = (char*)(((long)mem + 255) & ~255L);
	Setscreen((void*)-1, (void*)-1, -1);
	CHECK_EQ("Setscreen", (long)Logbase(), (long)log);
	Setscreen(newlog, (void*)-1, -1);
	CHECK_EQ("Setscreen", (long)Logbase(), (long)newlog);
	CHECK_EQ("Setscreen", (long)Physbase(), (long)phys);
	Setscreen(log, (void*)-1, -1);
	CHECK_EQ("Setscreen", (long)Logbase(), (long)log);
	Mfree(mem);

	t_run("Setcolor");
	short c0 = Setcolor(0, -1);
	CHECK_EQ("Setcolor", Setcolor(0, 0x123), c0);
	CHECK_EQ("Setcolor", Setcolor(0, -1) & 0x777, 0x123);
	Setcolor(0, c0);

	t_run("Setpalette");
	short save[16], pal[16];
	for (int i = 0; i < 16; i++)
	{
		save[i] = Setcolor(i, -1);
		pal[i] = (short)(((i & 7) << 8) | ((i >> 1) << 4) | (7 - (i & 7)));
	}
	Setpalette(pal);
	wait_vbl(2);
	int ok = 1;
	for (int i = 0; i < 16; i++)
	{
		if ((Setcolor(i, -1) & 0x777) != pal[i])
		{
			ok = 0;
		}
	}
	CHECK_TRUE("Setpalette", ok, "colors read back with Setcolor match the palette");
	Setpalette(save);
	wait_vbl(2);

	t_run("Vsync");
	long f0 = t_peekl(0x466);
	wait_vbl(3);
	long f1 = t_peekl(0x466);
	CHECK_TRUE("Vsync", f1 - f0 >= 3 && f1 - f0 <= 5, "frclock advanced %ld frames over 3 Vsync()", f1 - f0);

	t_run("Cursconf");
	short rate = Cursconf(5, 0);
	CHECK_TRUE("Cursconf", rate > 0, "Cursconf(get rate) = %d", rate);
	Cursconf(4, 33);
	CHECK_EQ("Cursconf", Cursconf(5, 0), 33);
	Cursconf(4, rate);
}

static void test_keyboard(void)
{
	t_run("Iorec");
	IOREC* io = Iorec(1);
	CHECK_TRUE("Iorec", io != 0 && io->ibuf != 0 && io->ibufsiz > 0 && (io->ibufsiz & 3) == 0, "Iorec(keyboard) = %p size=%d", io, io ? io->ibufsiz : 0);
	io = Iorec(0);
	CHECK_TRUE("Iorec", io != 0 && io->ibuf != 0 && io->ibufsiz > 0, "Iorec(serial) = %p", io);
	io = Iorec(2);
	CHECK_TRUE("Iorec", io != 0 && io->ibuf != 0 && io->ibufsiz > 0, "Iorec(MIDI) = %p", io);

	t_run("Keytbl");
	KEYTAB* kt = Keytbl((char*)-1, (char*)-1, (char*)-1);
	if (CHECK_TRUE("Keytbl", kt != 0, "Keytbl(-1, -1, -1) = %p", kt))
	{
		CHECK_TRUE("Keytbl", kt->unshift[0x1e] == 'a' && kt->shift[0x1e] == 'A' && kt->capslock[0x1e] == 'A',
			"scancode 0x1e maps to '%c' '%c' '%c'", kt->unshift[0x1e], kt->shift[0x1e], kt->capslock[0x1e]);
		static char mytab[128];
		memcpy(mytab, kt->unshift, 128);
		Keytbl(mytab, (char*)-1, (char*)-1);
		CHECK_TRUE("Keytbl", kt->unshift == mytab && kt->shift != mytab, "Keytbl(new unshift table) sets only that table");
		t_run("Bioskeys");
		Bioskeys();
		CHECK_TRUE("Bioskeys", kt->unshift != mytab && kt->unshift[0x1e] == 'a', "Bioskeys() restores the default tables");
	}

	t_run("Kbrate");
	short kr = Kbrate(-1, -1);
	CHECK_TRUE("Kbrate", kr > 0, "Kbrate(-1, -1) = 0x%04x", kr);
	CHECK_EQ("Kbrate", Kbrate(20, 3), kr);
	CHECK_EQ("Kbrate", Kbrate(-1, -1), (20 << 8) | 3);
	Kbrate((kr >> 8) & 0xff, kr & 0xff);

	t_run("Kbdvbase");
	KBDVBASE* kb = Kbdvbase();
	CHECK_TRUE("Kbdvbase", kb != 0 && kb->kb_kbdsys != 0 && kb->kb_midisys != 0 && kb->kb_mousevec != 0,
		"Kbdvbase() = %p", kb);

	t_run("Initmous");
	if (kb)
	{
		MOUSE m = { 0, 0, 1, 1, 0, 0, 0, 0 };
		VOID_TOS_CB oldvec = kb->kb_mousevec;
		Initmous(1, &m, (VOID_TOS_CB)timer_a_handler);	// Any address, mouse doesn't move.
		CHECK_EQ("Initmous", (long)kb->kb_mousevec, (long)timer_a_handler);
		Initmous(1, &m, oldvec);
		CHECK_EQ("Initmous", (long)kb->kb_mousevec, (long)oldvec);
	}

	t_run("Ikbdws");
	Ikbdws(0, "\x08");	// Relative mouse mode, the TOS default.
	t_check(1, "Ikbdws", "called (sent IKBD relative mouse command, effect not observable)");
}

static void test_mfp(void)
{
	t_run("Xbtimer");
	timer_count = 0;
	Xbtimer(0, 7, 255, timer_a_handler);	// Timer A, /200, ~48 Hz
	wait_vbl(10);
	long c1 = timer_count;
	CHECK_TRUE("Xbtimer", c1 > 3, "Timer A interrupts in 10 frames: %ld", c1);
	CHECK_EQ("Xbtimer", t_peekl(0x134), (long)timer_a_handler);

	t_run("Jdisint");
	Jdisint(13);
	CHECK_EQ("Jdisint", peekb(0xfffffa07) & 0x20, 0);
	long c2 = timer_count;
	wait_vbl(5);
	CHECK_EQ("Jdisint", timer_count, c2);
	t_run("Jenabint");
	Jenabint(13);
	CHECK_EQ("Jenabint", peekb(0xfffffa07) & 0x20, 0x20);
	wait_vbl(5);
	CHECK_TRUE("Jenabint", timer_count > c2, "Timer A counts again: %ld -> %ld", c2, timer_count);
	Jdisint(13);
	Xbtimer(0, 0, 0, timer_a_handler);	// Stop the timer

	t_run("Mfpint");
	long old = t_peekl(0x134);
	Mfpint(13, (VOID_TOS_CB)test_mfp);
	CHECK_EQ("Mfpint", t_peekl(0x134), (long)test_mfp);
	Mfpint(13, (VOID_TOS_CB)old);
	CHECK_EQ("Mfpint", t_peekl(0x134), old);
}

static void test_psg(void)
{
	t_run("Giaccess");
	char r11 = Giaccess(0, 11);
	Giaccess(0x55, 11 | 0x80);
	CHECK_EQ("Giaccess", (unsigned char)Giaccess(0, 11), 0x55);
	Giaccess(r11, 11 | 0x80);

	t_run("Ongibit");
	unsigned char a = (unsigned char)Giaccess(0, 14);
	Ongibit(GI_DTR);
	CHECK_EQ("Ongibit", (unsigned char)Giaccess(0, 14) & GI_DTR, GI_DTR);
	t_run("Offgibit");
	Offgibit(~GI_DTR);
	CHECK_EQ("Offgibit", (unsigned char)Giaccess(0, 14) & GI_DTR, 0);
	if (a & GI_DTR)
	{
		Ongibit(GI_DTR);
	}

	t_run("Dosound");
	static char snd[] = { 8, 0, 9, 0, 10, 0, (char)0xff, 5, (char)0xff, 0 };
	Dosound(snd);
	char* p = Dosound((char*)-1);	// -1 returns the current sound pointer
	CHECK_TRUE("Dosound", p >= snd && p < snd + sizeof(snd), "sound pointer = %p while playing %p", p, snd);
	wait_vbl(10);
	CHECK_EQ("Dosound", (long)Dosound((char*)-1), 0);
}

static long super_flag;
static int super_fn(void)
{
	unsigned short sr;
	__asm__ volatile ("move.w %%sr, %0" : "=d" (sr));
	super_flag = sr;
	return 0x12345678;
}

static void test_misc(void)
{
	t_run("Supexec");
	CHECK_EQ("Supexec", Supexec(super_fn), 0x12345678);
	CHECK_TRUE("Supexec", super_flag & 0x2000, "function ran with sr = 0x%04lx", super_flag);

	t_run("Random");
	int r1 = Random(), r2 = Random();
	CHECK_TRUE("Random", r1 >= 0 && r1 <= 0xffffff && r2 >= 0 && r2 <= 0xffffff && r1 != r2, "Random() = 0x%x, 0x%x", r1, r2);

	t_run("Gettime");
	unsigned int g = Gettime();
	unsigned short d = Tgetdate();
	CHECK_TRUE("Gettime", (g >> 16) == d, "Gettime() = 0x%08x, Tgetdate() = 0x%04x", g, d);
	t_run("Settime");
	// 1995: the TOS 2.06 IKBD clock code only handles years up to 1999.
	unsigned int nt = ((unsigned int)(((1995 - 1980) << 9) | (3 << 5) | 4) << 16) | (5 << 11) | (6 << 5) | 4;
	Settime(nt);
	unsigned int g2 = Gettime();
	CHECK_TRUE("Settime", (g2 >> 5) == (nt >> 5), "Gettime() after Settime(0x%08x) = 0x%08x", nt, g2);
	Settime(g);

	t_run("Rsconf");
	int baud = Rsconf(-2, -1, -1, -1, -1, -1);
	CHECK_TRUE("Rsconf", baud >= 0 && baud <= 15, "Rsconf(-2) = %d, expected current baud rate", baud);
	Rsconf(BAUD_2400, -1, -1, -1, -1, -1);
	CHECK_EQ("Rsconf", Rsconf(-2, -1, -1, -1, -1, -1), BAUD_2400);
	Rsconf((short)baud, -1, -1, -1, -1, -1);
	int regs = Rsconf(-1, -1, -1, -1, -1, -1);
	t_check(1, "Rsconf", "Rsconf(-1, ...) = 0x%08x (ucr/rsr/tsr/scr)", regs);

	t_run("Setprt");
	short prt = Setprt(-1);
	CHECK_TRUE("Setprt", prt >= 0, "Setprt(-1) = %d", prt);
	CHECK_EQ("Setprt", Setprt(prt ^ 1), prt);
	CHECK_EQ("Setprt", Setprt(-1), prt ^ 1);
	Setprt(prt);

	t_run("Blitmode");
	short bm = Blitmode(-1);
	int has_blitter = t_env.mch == MCH_STE || t_env.mch == MCH_FALCON;
	CHECK_TRUE("Blitmode", ((bm & 2) != 0) == has_blitter, "Blitmode(-1) = 0x%x on machine %d", bm, t_env.mch);

	t_run("Midiws");
	Midiws(1, "OP");	// Sends 2 bytes, checked by run.sh
	t_check(1, "Midiws", "MIDI output is checked by run.sh");

	t_run("Dbmsg");
	Dbmsg(5, 0, 0);
	t_check(1, "Dbmsg", "called (no debugger installed, nothing to observe)");

	t_skip("Ssbrk", "only valid during boot");
	t_skip("Scrdmp", "dumps the whole screen to the printer");
	t_skip("Prtblk", "prints a screen block to the printer");
	t_skip("Puntaes", "discards the AES and reboots when it is RAM loaded");
}

int main(void)
{
	t_begin("T_XBIOS");
	test_screen();
	test_keyboard();
	test_mfp();
	test_psg();
	test_misc();
	return t_end();
}
