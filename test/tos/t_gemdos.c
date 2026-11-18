/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	GEMDOS tests, except file and directory functions (see t_file.c).
	Character output is checked by redirecting the standard handles to a file on the
	test floppy (handled by TOS itself, not by Hatari's GEMDOS drive emulation).
	Character input is checked by putting keys into the keyboard buffer.
*/

#include <string.h>
#include "harness.h"

#define CHILD "C:\\TOSTEST\\CHILD.PRG"

static unsigned short get_sr(void)
{
	unsigned short sr;
	__asm__ volatile ("move.w %%sr, %0" : "=d" (sr));
	return sr;
}

static void test_version(void)
{
	t_run("Sversion");
	short v = Sversion();
	CHECK_EQ("Sversion", (unsigned short)v, t_env.gemdos);
	CHECK_TRUE("Sversion", (v & 0xff) == 0 && v != 0, "Sversion() = 0x%04x, expected GEMDOS 0.xx", (unsigned short)v);
}

static void test_super(void)
{
	t_run("Super");
	CHECK_EQ("Super", Super((void*)1), 0);	// Inquire in user mode
	long old = Super(0);
	unsigned short sr = get_sr();
	CHECK_TRUE("Super", old != 0 && (sr & 0x2000), "Super(0) = 0x%lx, sr = 0x%04x, expected supervisor mode", old, sr);
	CHECK_EQ("Super", Super((void*)1), -1);	// Inquire in supervisor mode
	Super((void*)old);
	sr = get_sr();
	CHECK_TRUE("Super", (sr & 0x2000) == 0, "Super(old) back to user mode, sr = 0x%04x", sr);
}

static void test_date_time(void)
{
	t_run("Tgetdate");
	unsigned short d0 = Tgetdate();
	unsigned short tm0 = Tgettime();
	CHECK_TRUE("Tgetdate", ((d0 >> 5) & 15) >= 1 && ((d0 >> 5) & 15) <= 12 && (d0 & 31) >= 1,
		"Tgetdate() = 0x%04x, expected a valid date", d0);
	unsigned short nd = ((2021 - 1980) << 9) | (6 << 5) | 15;
	t_run("Tsetdate");
	CHECK_EQ("Tsetdate", (short)Tsetdate(nd), 0);
	CHECK_EQ("Tgetdate", Tgetdate(), nd);
	unsigned short nt = (13 << 11) | (45 << 5) | 10;
	t_run("Tsettime");
	CHECK_EQ("Tsettime", (short)Tsettime(nt), 0);
	unsigned short t = Tgettime();
	CHECK_TRUE("Tgettime", (t >> 5) == (nt >> 5) && (t & 31) >= 10 && (t & 31) <= 20,
		"Tgettime() = 0x%04x, expected 0x%04x (+ a few seconds)", t, nt);
	Tsetdate(d0);
	Tsettime(tm0);
}

static void test_drives(void)
{
	t_run("Dgetdrv");
	short d = Dgetdrv();
	CHECK_TRUE("Dgetdrv", d >= 0 && d < 16 && (Drvmap() & (1 << d)), "Dgetdrv() = %d, expected an existing drive", d);
	t_run("Dsetdrv");
	int map = Dsetdrv(1);
	CHECK_TRUE("Dsetdrv", (map & 5) == 5, "Dsetdrv(1) = 0x%x, expected drive map with A: and C:", map);
	CHECK_EQ("Dsetdrv", Dgetdrv(), 1);
	Dsetdrv(2);
	CHECK_EQ("Dsetdrv", Dgetdrv(), 2);
	Dsetdrv(d);

	DISKINFO di;
	memset(&di, 0, sizeof(di));
	t_run("Dfree");
	CHECK_EQ("Dfree", Dfree(&di, t_env.flop + 1), 0);	// Test floppy
	CHECK_TRUE("Dfree", di.b_secsize == 512 && di.b_clsize == 2 && di.b_total > 700 && di.b_total < 720 && di.b_free <= di.b_total && di.b_free > 600,
		"Dfree(floppy) free=%u total=%u secsize=%u clsize=%u, expected 720K floppy", di.b_free, di.b_total, di.b_secsize, di.b_clsize);
	memset(&di, 0, sizeof(di));
	CHECK_EQ("Dfree", Dfree(&di, 3), 0);	// C:
	CHECK_TRUE("Dfree", di.b_secsize > 0 && di.b_clsize > 0 && di.b_total >= di.b_free && di.b_free > 0,
		"Dfree(C:) free=%u total=%u secsize=%u clsize=%u", di.b_free, di.b_total, di.b_secsize, di.b_clsize);
}

static void test_memory(void)
{
	t_run("Malloc");
	long avail = (long)Malloc(-1);
	CHECK_TRUE("Malloc", avail > 100000, "Malloc(-1) = %ld, expected > 100000 free", avail);
	char* p = Malloc(10000);
	CHECK_TRUE("Malloc", p != 0 && ((long)p & 1) == 0, "Malloc(10000) = %p", p);
	if (p == 0)
	{
		return;
	}
	memset(p, 0x55, 10000);
	long avail2 = (long)Malloc(-1);
	CHECK_TRUE("Malloc", avail2 <= avail - 10000, "Malloc(-1) after allocation = %ld, before %ld", avail2, avail);
	t_run("Mshrink");
	CHECK_EQ("Mshrink", Mshrink(p, 5000), 0);
	CHECK_TRUE("Mshrink", Mshrink(p, 20000) < 0, "Mshrink to a larger size fails");
	t_run("Mfree");
	CHECK_EQ("Mfree", Mfree(p), 0);
	CHECK_TRUE("Mfree", Mfree(p) < 0, "Mfree of an already freed block fails");
	CHECK_EQ("Malloc", (long)Malloc(-1), avail);

	if (t_env.gemdos >= 0x1900)
	{
		t_run("Mxalloc");
		long phystop = t_peekl(0x42e);
		long st = (long)Mxalloc(-1, 0);
		CHECK_TRUE("Mxalloc", st > 100000, "Mxalloc(-1, ST) = %ld", st);
		char* q = Mxalloc(1000, 0);
		CHECK_TRUE("Mxalloc", q != 0 && (long)q < phystop, "Mxalloc(1000, ST) = %p, phystop = 0x%lx", q, phystop);
		if (q)
		{
			CHECK_EQ("Mxalloc", Mfree(q), 0);
		}
		q = Mxalloc(1000, 3);
		CHECK_TRUE("Mxalloc", q != 0, "Mxalloc(1000, prefer any) = %p", q);
		if (q)
		{
			Mfree(q);
		}
		long tt = (long)Mxalloc(-1, 1);
		CHECK_TRUE("Mxalloc", t_env.mch == MCH_TT ? tt > 0 : tt == 0, "Mxalloc(-1, TT-RAM) = %ld", tt);
	}
	else
	{
		t_skip("Mxalloc", "GEMDOS 0x%04x has no Mxalloc", t_env.gemdos);
	}
	t_skip("Maddalt", "would add memory to the system pool");
}

static void test_pexec(void)
{
	t_run("Pexec");
	CHECK_EQ("Pterm", Pexec(0, CHILD, "\004T 42", 0), 42);
	CHECK_EQ("Pterm", Pexec(0, CHILD, "\005T 300", 0), 300);
	CHECK_EQ("Pterm0", Pexec(0, CHILD, "\001Z", 0), 0);
	CHECK_EQ("Ptermres", Pexec(0, CHILD, "\001R", 0), 77);
	CHECK_EQ("Pexec", Pexec(0, CHILD, "\007A 12345", 0), 7);
	CHECK_EQ("Pexec", Pexec(0, CHILD, "\001E", "FOO=bar\0\0"), 1);
	CHECK_EQ("Pexec", Pexec(0, "C:\\TOSTEST\\NOSUCH.PRG", "\0", 0), EFILNF);

	// Load, then go.
	BASEPAGE* bp = (BASEPAGE*)Pexec(3, CHILD, "\004T 33", 0);
	if (CHECK_TRUE("Pexec", (long)bp > 0 && bp->p_tbase == (char*)bp + 256 && bp->p_tlen > 0,
		"Pexec(3) = %p, expected a basepage", bp))
	{
		CHECK_EQ("Pexec", Pexec(4, 0, bp, 0), 33);
		Mfree(bp->p_env);
		CHECK_EQ("Pexec", Mfree(bp), 0);
	}
	// Create basepage.
	bp = (BASEPAGE*)Pexec(5, 0, "\0", 0);
	if (CHECK_TRUE("Pexec", (long)bp > 0 && bp->p_lowtpa == bp && bp->p_hitpa > (void*)bp,
		"Pexec(5) = %p, expected a basepage", bp))
	{
		Mfree(bp->p_env);
		Mfree(bp);
	}
	// Load, then just go and free.
	if (t_env.gemdos >= 0x1500)
	{
		bp = (BASEPAGE*)Pexec(3, CHILD, "\004T 44", 0);
		if ((long)bp > 0)
		{
			CHECK_EQ("Pexec", Pexec(6, 0, bp, 0), 44);
		}
	}
}

static int read_file(const char* name, char* buf, int size)
{
	int h = Fopen(name, FO_READ);
	if (h < 0)
	{
		return h;
	}
	int n = Fread((short)h, size - 1, buf);
	Fclose((short)h);
	buf[n > 0 ? n : 0] = 0;
	return n;
}

// Redirect a standard handle to a file, run the output function and check what was written.
static void redirect_out(short std, const char* file, void (*fn)(void), const char* func, const char* want)
{
	char buf[32];
	int h = Fcreate(file, 0);
	if (!CHECK_TRUE(func, h >= 0, "Fcreate(%s) = %d", file, h))
	{
		return;
	}
	int saved = Fdup(std);
	CHECK_TRUE("Fdup", saved >= 6, "Fdup(%d) = %d, expected a non standard handle", std, saved);
	CHECK_EQ("Fforce", Fforce(std, (short)h), 0);
	fn();
	Fforce(std, (short)saved);
	Fclose((short)saved);
	Fclose((short)h);
	read_file(file, buf, sizeof(buf));
	CHECK_TRUE(func, strcmp(buf, want) == 0, "redirected output \"%s\", expected \"%s\"", buf, want);
}

static void con_out(void)
{
	Cconout('A');
	Cconws("BC");
}
static void aux_out(void)
{
	Cauxout('E');
}
static void prn_out(void)
{
	Cprnout('F');
}

static void test_console(void)
{
	t_run("Cconout");
	redirect_out(1, t_fpath("\\CONOUT.TXT"), con_out, "Cconout", "ABC");
	t_run("Cauxout");
	redirect_out(2, t_fpath("\\AUXOUT.TXT"), aux_out, "Cauxout", "E");
	t_run("Cprnout");
	redirect_out(3, t_fpath("\\PRNOUT.TXT"), prn_out, "Cprnout", "F");

	t_run("Cconos");
	CHECK_EQ("Cconos", Cconos(), -1);
	t_run("Cprnos");
	CHECK_EQ("Cprnos", Cprnos(), -1);	// Hatari --printer
	t_run("Cprnout");
	short pr = Cprnout('P');	// Goes to the printer file, checked by run.sh
	CHECK_TRUE("Cprnout", pr != 0, "Cprnout('P') = %d, expected non zero (printed)", pr);
	t_run("Cauxos");
	CHECK_EQ("Cauxos", Cauxos(), -1);

	// Serial input: run.sh feeds "ST" to the serial port.
	t_run("Cauxis");
	short st = 0;
	for (int i = 0; i < 100 && st == 0; i++)
	{
		st = Cauxis();
		Vsync();
	}
	CHECK_EQ("Cauxis", st, -1);
	t_run("Cauxin");
	if (st)
	{
		CHECK_EQ("Cauxin", Cauxin(), 'S');
	}
	else
	{
		t_check(0, "Cauxin", "not called, no serial input available");
	}

	// Console output, captured by Hatari --conout 2 and checked by run.sh.
	t_run("Crawio");
	Crawio('{');
	Crawio('D');
	Crawio('}');
	t_run("Cconws");
	Cconws("<cws>");
	t_check(1, "Cconws", "console output \"<cws>\" and \"{D}\" (Crawio) is checked by run.sh");

	// Keyboard input.
	t_flush_keys();
	t_run("Cconis");
	CHECK_EQ("Cconis", Cconis(), 0);
	t_key(0x1e, 'a');
	CHECK_EQ("Cconis", Cconis(), -1);
	t_run("Cconin");
	CHECK_EQ("Cconin", Cconin(), 0x1e0061);
	t_key(0x30, 'b');
	t_run("Crawcin");
	CHECK_EQ("Crawcin", Crawcin(), 0x300062);
	t_key(0x2e, 'c');
	t_run("Cnecin");
	CHECK_EQ("Cnecin", Cnecin(), 0x2e0063);
	t_run("Crawio");
	CHECK_EQ("Crawio", Crawio(0xff), 0);
	t_key(0x20, 'd');
	CHECK_EQ("Crawio", Crawio(0xff), 0x200064);
	char line[16];
	memset(line, 0, sizeof(line));
	line[0] = 10;
	t_keys("hi\r");
	t_run("Cconrs");
	Cconrs(line);
	CHECK_TRUE("Cconrs", line[1] == 2 && line[2] == 'h' && line[3] == 'i', "Cconrs() read %d chars \"%c%c\", expected 2 \"hi\"", line[1], line[2], line[3]);
	Cconws("\r\n");
}

int main(void)
{
	t_begin("T_GEMDOS");
	test_version();
	test_super();
	test_date_time();
	test_drives();
	test_memory();
	test_console();
	test_pexec();
	return t_end();
}
