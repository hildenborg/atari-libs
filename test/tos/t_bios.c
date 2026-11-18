/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	BIOS tests (trap #13).
	Printer, serial and MIDI output end up in files on the host, checked by run.sh.
	Serial and MIDI input come from files on the host.
*/

#include <string.h>
#include "harness.h"

static long wait_stat(short dev)
{
	long st = 0;
	for (int i = 0; i < 100 && st == 0; i++)
	{
		st = Bconstat(dev);
		Vsync();
	}
	return st;
}

static void dummy_vector(void)
{
}

static void test_devices(void)
{
	t_run("Bconstat");
	t_flush_keys();
	CHECK_EQ("Bconstat", Bconstat(2), 0);
	t_key(0x1e, 'a');
	CHECK_EQ("Bconstat", Bconstat(2), -1);
	t_run("Bconin");
	CHECK_EQ("Bconin", Bconin(2), 0x1e0061);

	// Serial: run.sh feeds "ST", T_GEMDOS has read the 'S'.
	if (CHECK_EQ_OK("Bconstat", wait_stat(1), -1))
	{
		CHECK_EQ("Bconin", Bconin(1) & 0xff, 'T');
	}
	// MIDI: run.sh feeds "M".
	if (CHECK_EQ_OK("Bconstat", wait_stat(3), -1))
	{
		CHECK_EQ("Bconin", Bconin(3) & 0xff, 'M');
	}

	t_run("Bcostat");
	CHECK_EQ("Bcostat", Bcostat(0), -1);	// Printer
	CHECK_EQ("Bcostat", Bcostat(1), -1);	// Serial
	CHECK_EQ("Bcostat", Bcostat(2), -1);	// Console
	CHECK_EQ("Bcostat", Bcostat(3), -1);	// MIDI (IKBD in TOS due to an old swap, both ready)
	CHECK_EQ("Bcostat", Bcostat(4), -1);

	t_run("Bconout");
	Bconout(0, 'Q');	// printer: "PQ" with Cprnout from T_GEMDOS
	Bconout(1, 'U');	// serial: "U"
	Bconout(3, 'N');	// MIDI: "N" + "OP" from Midiws in T_XBIOS
	Bconout(2, '(');	// console: "(B)"
	Bconout(2, 'B');
	Bconout(2, ')');
	t_check(1, "Bconout", "printer, serial, MIDI and console output is checked by run.sh");
}

static void test_system(void)
{
	MPB mpb;
	memset(&mpb, 0, sizeof(mpb));
	t_run("Getmpb");
	Getmpb(&mpb);
	// The memory descriptors can be in protected system memory, read them in supervisor mode.
	long len = mpb.mp_mfl ? t_peekl((unsigned long)mpb.mp_mfl + 8) : 0;
	CHECK_TRUE("Getmpb", mpb.mp_mfl != 0 && len > 100000, "Getmpb: mfl=%p length=%ld", mpb.mp_mfl, len);

	t_run("Setexc");
	// Vector 35 is trap #3, unused by TOS.
	VOID_TOS_CB v = Setexc(35, (VOID_TOS_CB)-1);
	CHECK_EQ("Setexc", (long)v, t_peekl(35 * 4));
	VOID_TOS_CB old = Setexc(35, dummy_vector);
	CHECK_EQ("Setexc", (long)old, (long)v);
	CHECK_EQ("Setexc", t_peekl(35 * 4), (long)dummy_vector);
	Setexc(35, v);
	CHECK_EQ("Setexc", t_peekl(35 * 4), (long)v);

	t_run("Tickcal");
	CHECK_EQ("Tickcal", Tickcal(), 20);

	t_run("Drvmap");
	int map = Drvmap();
	CHECK_TRUE("Drvmap", (map & 5) == 5 && map == t_peekl(0x4c2), "Drvmap() = 0x%x, _drvbits = 0x%lx", map, t_peekl(0x4c2));

	t_run("Kbshift");
	int ks = Kbshift(-1);
	CHECK_TRUE("Kbshift", ks >= 0 && ks < 256, "Kbshift(-1) = 0x%x", ks);
	CHECK_EQ("Kbshift", Kbshift(ks | 0x10), ks);	// Caps lock
	CHECK_EQ("Kbshift", Kbshift(-1), ks | 0x10);
	Kbshift(ks);
	CHECK_EQ("Kbshift", Kbshift(-1), ks);
}

static void test_disk(void)
{
	static char sec[512], sec2[512];
	t_run("Getbpb");
	short fd = t_env.flop;
	BPB* bpb = Getbpb(fd);
	if (CHECK_TRUE("Getbpb", bpb != 0, "Getbpb(floppy) = %p", bpb))
	{
		CHECK_TRUE("Getbpb", bpb->recsiz == 512 && bpb->clsiz == 2 && bpb->clsizb == 1024 && bpb->numcl > 700 && bpb->numcl < 720,
			"BPB recsiz=%d clsiz=%d clsizb=%d numcl=%d, expected 720K floppy", bpb->recsiz, bpb->clsiz, bpb->clsizb, bpb->numcl);
	}
	t_run("Mediach");
	int mc = Mediach(fd);
	CHECK_TRUE("Mediach", mc >= 0 && mc <= 2, "Mediach(floppy) = %d", mc);

	t_run("Rwabs");
	memset(sec, 0, sizeof(sec));
	CHECK_EQ("Rwabs", Rwabs(0, sec, 1, 0, fd, 0), 0);	// Boot sector
	CHECK_TRUE("Rwabs", sec[11] == 0 && sec[12] == 2 && (unsigned char)sec[19] == 0xa0 && sec[20] == 5,
		"boot sector bytes/sector=%d sectors=%d, expected 512 and 1440", (unsigned char)sec[11] | (sec[12] << 8), (unsigned char)sec[19] | (sec[20] << 8));
	// Write and read back the last sector (unused on an empty disk).
	for (int i = 0; i < 512; i++)
	{
		sec[i] = (char)(i ^ 0x5a);
	}
	CHECK_EQ("Rwabs", Rwabs(1, sec, 1, 1439, fd, 0), 0);
	memset(sec2, 0, sizeof(sec2));
	CHECK_EQ("Rwabs", Rwabs(0, sec2, 1, 1439, fd, 0), 0);
	CHECK_TRUE("Rwabs", memcmp(sec, sec2, 512) == 0, "sector 1439 reads back what was written");
	// Long record number (recno = -1).
	memset(sec2, 0, sizeof(sec2));
	// TOS floppy drivers don't support long record numbers, EmuTOS does.
	if (t_env.emutos)
	{
		CHECK_EQ("Rwabs", Rwabs(0, sec2, 1, -1, fd, 1439), 0);
		CHECK_TRUE("Rwabs", memcmp(sec, sec2, 512) == 0, "sector 1439 read with lrecno");
	}
}

int main(void)
{
	t_begin("T_BIOS");
	test_devices();
	test_system();
	test_disk();
	return t_end();
}
