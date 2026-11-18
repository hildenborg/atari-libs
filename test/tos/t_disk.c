/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	XBIOS floppy tests on the test floppy (a 720K image in B:, or A: on single drive machines).
	Only the last track (79, side 1) is written, it is unused on an empty disk.
*/

#include <string.h>
#include "harness.h"

static char trk[10 * 1024];	// Flopfmt needs room for a whole raw track (8K or more)
static char sec[512];
static char boot[512];

static void test_floppy(void)
{
	t_run("Floprd");
	memset(sec, 0, sizeof(sec));
	CHECK_EQ("Floprd", Floprd(sec, 0, t_env.flop, 1, 0, 0, 1), 0);
	CHECK_TRUE("Floprd", sec[11] == 0 && sec[12] == 2 && sec[24] == 9 && sec[26] == 2,
		"boot sector: bytes/sector=%d sectors/track=%d sides=%d", (unsigned char)sec[11] | (sec[12] << 8), sec[24], sec[26]);
	memcpy(boot, sec, sizeof(boot));

	// Hatari can't low level format .st images (only .stx), the FDC write track command fails.
	// This only checks that the call reaches the floppy driver and returns its error.
	t_run("Flopfmt");
	static short skew[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	memset(trk, 0, sizeof(trk));
	short fr = Flopfmt(trk, skew, t_env.flop, 9, 79, 1, 1, 0x87654321, 0xe5e5);
	CHECK_TRUE("Flopfmt", fr == -2 || fr == -10 || fr == -13 || fr == -16,
		"Flopfmt() = %d, expected a floppy error (Hatari can't format .st images)", fr);

	t_run("Flopwr");
	for (int i = 0; i < 512; i++)
	{
		sec[i] = (char)(i * 3 + 1);
	}
	CHECK_EQ("Flopwr", Flopwr(sec, 0, t_env.flop, 5, 79, 1, 1), 0);
	memset(trk, 0, 512);
	CHECK_EQ("Flopwr", Floprd(trk, 0, t_env.flop, 5, 79, 1, 1), 0);
	CHECK_TRUE("Flopwr", memcmp(trk, sec, 512) == 0, "sector 5 of track 79 side 1 reads back what was written");

	t_run("Flopver");
	memset(trk, 0x55, 512);
	CHECK_EQ("Flopver", Flopver(trk, 0, t_env.flop, 1, 79, 1, 9), 0);
	CHECK_EQ("Flopver", *(short*)trk, 0);	// No bad sectors in the list

	t_run("Floprate");
	if (t_env.tos >= 0x0104)
	{
		short rate = Floprate(t_env.flop, -1);
		CHECK_TRUE("Floprate", rate >= 0 && rate <= 3, "Floprate(floppy, -1) = %d", rate);
		CHECK_EQ("Floprate", Floprate(t_env.flop, 2), rate);
		CHECK_EQ("Floprate", Floprate(t_env.flop, -1), 2);
		Floprate(t_env.flop, rate);
	}
	else
	{
		t_skip("Floprate", "TOS 0x%04x has no Floprate", t_env.tos);
	}
}

static void test_protobt(void)
{
	t_run("Protobt");
	memcpy(sec, boot, sizeof(sec));
	// Disk type 3: 80 tracks double sided, keep serial number, not executable.
	Protobt(sec, -1, 3, 0);
	CHECK_TRUE("Protobt", sec[11] == 0 && sec[12] == 2 && (unsigned char)sec[19] == 0xa0 && sec[20] == 5 && sec[24] == 9 && sec[26] == 2,
		"prototype boot sector: sectors=%d spt=%d sides=%d", (unsigned char)sec[19] | (sec[20] << 8), sec[24], sec[26]);
	CHECK_TRUE("Protobt", memcmp(sec + 8, boot + 8, 3) == 0, "serial number -1 keeps the old one");
	unsigned short sum = 0;
	for (int i = 0; i < 256; i++)
	{
		sum += ((unsigned short*)sec)[i];
	}
	CHECK_TRUE("Protobt", sum != 0x1234, "not executable, checksum 0x%04x", sum);

	Protobt(sec, 0x00123456, 2, 1);	// 80 tracks single sided, new serial, executable
	sum = 0;
	for (int i = 0; i < 256; i++)
	{
		sum += ((unsigned short*)sec)[i];
	}
	CHECK_TRUE("Protobt", sum == 0x1234, "executable, checksum 0x%04x", sum);
	CHECK_TRUE("Protobt", sec[8] == 0x56 && sec[9] == 0x34 && sec[10] == 0x12 && sec[26] == 1,
		"serial number %02x %02x %02x sides=%d", (unsigned char)sec[8], (unsigned char)sec[9], (unsigned char)sec[10], sec[26]);
}

int main(void)
{
	t_begin("T_DISK");
	test_floppy();
	test_protobt();
	return t_end();
}
