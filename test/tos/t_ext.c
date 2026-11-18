/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	XBIOS tests for functions that only exist on some machines or TOS versions:
	Bconmap, NVMaccess, Metainit, DMAread/DMAwrite, TT video (E...) and Falcon video (V...).
	Sound is in t_sound.c, the DSP in t_dsp.c.
*/

#include <string.h>
#include "harness.h"

static void test_bconmap(void)
{
	t_run("Bconmap");
	if (Bconmap(0) != 0)	// Returns 0 when Bconmap is supported.
	{
		t_skip("Bconmap", "not supported on this machine/TOS");
		return;
	}
	int cur = Bconmap(-1);
	CHECK_TRUE("Bconmap", cur >= 6, "Bconmap(-1) = %d, expected the current AUX device (6 or more)", cur);
	BCONMAP* bm = (BCONMAP*)Bconmap(-2);
	CHECK_TRUE("Bconmap", bm != 0 && bm->maptab != 0 && bm->maptabsize >= 1,
		"Bconmap(-2) = %p, maptabsize = %d", bm, bm ? bm->maptabsize : 0);
	// Map the first device. TOS 4.04 reports 7 at boot but only has device 6 in its table,
	// so the old device is only restored, not checked.
	CHECK_EQ("Bconmap", Bconmap(6), cur);
	CHECK_EQ("Bconmap", Bconmap(-1), 6);
	Bconmap((short)cur);
}

static void test_nvram(void)
{
	t_run("NVMaccess");
	if (t_env.mch != MCH_TT && t_env.mch != MCH_FALCON)
	{
		t_skip("NVMaccess", "no NVRAM on this machine");
		return;
	}
	unsigned char buf[8], buf2[8];
	memset(buf, 0, sizeof(buf));
	CHECK_EQ("NVMaccess", NVMaccess(0, 0, 8, buf), 0);	// Read
	CHECK_EQ("NVMaccess", NVMaccess(1, 0, 8, buf), 0);	// Write the same bytes back
	memset(buf2, 0xaa, sizeof(buf2));
	CHECK_EQ("NVMaccess", NVMaccess(0, 0, 8, buf2), 0);
	CHECK_TRUE("NVMaccess", memcmp(buf, buf2, 8) == 0, "NVRAM bytes 0-7 read back the same after writing");
	CHECK_TRUE("NVMaccess", NVMaccess(0, 50, 8, buf2) < 0, "reading outside the 50 bytes of NVRAM fails");
}

static void test_metainit(void)
{
	t_run("Metainit");
	METAINFO mi;
	memset(&mi, 0, sizeof(mi));
	Metainit(&mi);
	CHECK_TRUE("Metainit", mi.version == 0 && mi.drivemap == 0, "no MetaDOS installed: version=%p drivemap=0x%x", mi.version, mi.drivemap);
}

static void test_dma(void)
{
	if (t_env.tos < 0x0200)
	{
		t_skip("DMAread", "TOS 0x%04x has no DMAread", t_env.tos);
		t_skip("DMAwrite", "TOS 0x%04x has no DMAwrite", t_env.tos);
		return;
	}
	// run.sh attaches a hard disk image: ACSI 0 on ST/STE, SCSI 0 (8) on TT and Falcon.
	short dev = t_env.mch >= MCH_TT ? 8 : 0;
	unsigned char* buf = Mxalloc(1024, 0);	// DMA needs ST-RAM
	if (buf == 0)
	{
		t_check(0, "DMAread", "no ST-RAM for the buffer");
		return;
	}
	t_run("DMAread");
	memset(buf, 0, 1024);
	CHECK_EQ("DMAread", DMAread(10, 2, buf, dev), 0);
	int ok = 1;
	for (int s = 0; s < 2; s++)
	{
		for (int i = 0; i < 512; i++)
		{
			if (buf[s * 512 + i] != (unsigned char)((s + 10) * 3 + i))
			{
				ok = 0;
			}
		}
	}
	CHECK_TRUE("DMAread", ok, "sectors 10 and 11 of device %d hold the pattern written by run.sh", dev);

	t_run("DMAwrite");
	for (int i = 0; i < 512; i++)
	{
		buf[i] = (unsigned char)(i * 5 + 1);
	}
	short w = DMAwrite(20, 1, buf, dev);
	memset(buf, 0, 512);
	DMAread(20, 1, buf, dev);
	ok = w == 0;
	for (int i = 0; i < 512; i++)
	{
		if (buf[i] != (unsigned char)(i * 5 + 1))
		{
			ok = 0;
		}
	}
	CHECK_TRUE("DMAwrite", ok, "DMAwrite(20) = %d, sector reads back what was written (also checked on the host)", w);
	Mfree(buf);
}

static void test_tt_video(void)
{
	static const char* funcs[] = { "Esetshift", "Egetshift", "EsetBank", "EsetColor", "EsetPalette", "EgetPalette", "EsetGray", "EsetSmear", 0 };
	if (t_env.mch != MCH_TT)
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "TT only");
		}
		return;
	}
	t_run("Egetshift");
	short sh = Egetshift();
	CHECK_EQ("Egetshift", sh & 0x0700, (Getrez() & 7) << 8);
	t_run("Esetshift");
	CHECK_EQ("Esetshift", Esetshift(sh), sh);
	CHECK_EQ("Esetshift", Egetshift(), sh);

	t_run("EsetBank");
	short bank = EsetBank(-1);
	CHECK_TRUE("EsetBank", bank >= 0 && bank <= 15, "EsetBank(-1) = %d", bank);
	CHECK_EQ("EsetBank", EsetBank(1), bank);
	CHECK_EQ("EsetBank", EsetBank(-1), 1);
	EsetBank(bank);

	t_run("EsetColor");
	short c = EsetColor(5, -1);
	CHECK_EQ("EsetColor", EsetColor(5, 0x0abc), c);
	CHECK_EQ("EsetColor", EsetColor(5, -1), 0x0abc);
	EsetColor(5, c);

	t_run("EsetPalette");
	short save[4], pal[4] = { 0x0123, 0x0456, 0x0789, 0x0abc }, rd[4];
	t_run("EgetPalette");
	EgetPalette(0, 4, save);
	CHECK_EQ("EgetPalette", save[0], EsetColor(0, -1));
	EsetPalette(0, 4, pal);
	memset(rd, 0, sizeof(rd));
	EgetPalette(0, 4, rd);
	CHECK_TRUE("EsetPalette", memcmp(rd, pal, sizeof(pal)) == 0, "EgetPalette reads back %03x %03x %03x %03x", rd[0], rd[1], rd[2], rd[3]);
	EsetPalette(0, 4, save);

	t_run("EsetGray");
	short g = EsetGray(-1);
	CHECK_TRUE("EsetGray", g == 0 || g == 1, "EsetGray(-1) = %d", g);
	CHECK_EQ("EsetGray", EsetGray(1), g);
	CHECK_EQ("EsetGray", EsetGray(-1), 1);
	EsetGray(g);

	t_run("EsetSmear");
	short sm = EsetSmear(-1);
	CHECK_TRUE("EsetSmear", sm == 0 || sm == 1, "EsetSmear(-1) = %d", sm);
	CHECK_EQ("EsetSmear", EsetSmear(1), sm);
	CHECK_EQ("EsetSmear", EsetSmear(-1), 1);
	EsetSmear(sm);
}

static void test_falcon_video(void)
{
	static const char* funcs[] = { "Vsetmode", "VgetMonitor", "VsetSync", "VgetSize", "VsetMask", "VsetRGB", "VgetRGB", 0 };
	if (t_env.mch != MCH_FALCON)
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "Falcon only");
		}
		return;
	}
	t_run("VgetMonitor");
	short mon = VgetMonitor();
	CHECK_TRUE("VgetMonitor", mon >= 0 && mon <= 3, "VgetMonitor() = %d", mon);

	t_run("Vsetmode");
	short mode = Vsetmode(-1);
	CHECK_TRUE("Vsetmode", mode > 0, "Vsetmode(-1) = 0x%04x", mode);
	CHECK_EQ("Vsetmode", Vsetmode(mode), mode);

	t_run("VgetSize");
	int size = VgetSize(mode);
	int bpp = 1 << (mode & NUMCOLS);
	int w = (mode & COL80) ? 640 : 320;
	int h = (mode & VGA_FALCON) ? ((mode & VERTFLAG) ? 240 : 480) : ((mode & VERTFLAG) ? 400 : 200);
	int want = (mode & STMODES) ? 32000 : w * h * bpp / 8;
	if (mode & OVERSCAN)
	{
		CHECK_TRUE("VgetSize", size > want, "VgetSize(0x%04x) = %d, more than %d with overscan", mode, size, want);
	}
	else
	{
		CHECK_EQ("VgetSize", size, want);
	}
	if (mon == 2)	// The VGA flag only counts on a VGA monitor.
	{
		CHECK_EQ("VgetSize", VgetSize(COL80 | BPS1 | VGA_FALCON), 640 * 480 / 8);
	}
	else
	{
		CHECK_EQ("VgetSize", VgetSize(COL80 | BPS1), 640 * 200 / 8);
	}

	t_run("VsetRGB");
	int save[4], rgb[4] = { 0x00fc0000, 0x0000fc00, 0x000000fc, 0x00808080 }, rd[4];
	t_run("VgetRGB");
	VgetRGB(0, 4, save);
	VsetRGB(0, 4, rgb);
	memset(rd, 0, sizeof(rd));
	VgetRGB(0, 4, rd);
	CHECK_TRUE("VsetRGB", memcmp(rd, rgb, sizeof(rgb)) == 0, "VgetRGB reads back %06x %06x %06x %06x", rd[0], rd[1], rd[2], rd[3]);
	VsetRGB(0, 4, save);

	t_run("VsetSync");
	VsetSync(0);
	t_check(1, "VsetSync", "called with internal sync (nothing to observe)");
	t_run("VsetMask");
	VsetMask(0, -1, 0);
	t_check(1, "VsetMask", "called (only affects true color overlay mode)");
}

int main(void)
{
	t_begin("T_EXT");
	test_bconmap();
	test_nvram();
	test_metainit();
	test_dma();
	test_tt_video();
	test_falcon_video();
	return t_end();
}
