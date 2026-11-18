/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	GEMDOS file and directory tests.
	Run on C: (Hatari GEMDOS drive emulation) and on the floppy image (handled by TOS).
*/

#include <string.h>
#include "harness.h"

static char pattern[1000];
static char buf[2048];
static char func[32];
static short boot_drv;

// Function name with the drive letter, so failures show where they happened.
static const char* F(const char* name, short drv)
{
	strcpy(func, name);
	strcat(func, drv == 2 ? "(C:)" : "(floppy)");
	return func;
}

static void test_dirs_and_files(short drv)
{
	char path[64];
	char tmp[64];
	const char* base = drv == 2 ? "C:\\TFDIR" : t_fpath("\\TFDIR");

	t_run(F("Dcreate", drv));
	CHECK_EQ(F("Dcreate", drv), Dcreate(base), 0);
	CHECK_EQ(F("Dcreate", drv), Dcreate(base), EACCDN);

	t_run(F("Dsetpath", drv));
	Dsetdrv(drv);
	CHECK_EQ(F("Dsetpath", drv), Dsetpath("\\TFDIR"), 0);
	memset(path, 0, sizeof(path));
	t_run(F("Dgetpath", drv));
	CHECK_EQ(F("Dgetpath", drv), Dgetpath(path, 0), 0);
	CHECK_TRUE(F("Dgetpath", drv), strcmp(path, "\\TFDIR") == 0, "Dgetpath(current) = \"%s\", expected \"\\TFDIR\"", path);
	memset(path, 0, sizeof(path));
	CHECK_EQ(F("Dgetpath", drv), Dgetpath(path, drv + 1), 0);
	CHECK_TRUE(F("Dgetpath", drv), strcmp(path, "\\TFDIR") == 0, "Dgetpath(drive) = \"%s\", expected \"\\TFDIR\"", path);
	CHECK_EQ(F("Dsetpath", drv), Dsetpath("\\NOSUCH"), EPTHNF);

	// Write a file.
	t_run(F("Fcreate", drv));
	int h = Fcreate("A.TXT", 0);
	CHECK_TRUE(F("Fcreate", drv), h >= 6, "Fcreate(A.TXT) = %d, expected a handle", h);
	if (h < 0)
	{
		return;
	}
	t_run(F("Fwrite", drv));
	CHECK_EQ(F("Fwrite", drv), Fwrite((short)h, sizeof(pattern), pattern), sizeof(pattern));
	t_run(F("Fseek", drv));
	CHECK_EQ(F("Fseek", drv), Fseek(0, (short)h, 1), sizeof(pattern));
	CHECK_EQ(F("Fseek", drv), Fseek(0, (short)h, 0), 0);
	CHECK_EQ(F("Fseek", drv), Fseek(-10, (short)h, 2), sizeof(pattern) - 10);
	t_run(F("Fread", drv));
	CHECK_EQ(F("Fread", drv), Fread((short)h, 100, buf), 10);
	CHECK_TRUE(F("Fread", drv), memcmp(buf, pattern + sizeof(pattern) - 10, 10) == 0, "Fread data after Fseek(-10, end) matches");
	CHECK_EQ(F("Fread", drv), Fread((short)h, 100, buf), 0);
	t_run(F("Fclose", drv));
	CHECK_EQ(F("Fclose", drv), Fclose((short)h), 0);
	CHECK_EQ(F("Fclose", drv), Fclose((short)h), EIHNDL);

	// Read it back.
	t_run(F("Fopen", drv));
	h = Fopen("A.TXT", FO_READ);
	CHECK_TRUE(F("Fopen", drv), h >= 6, "Fopen(A.TXT, read) = %d, expected a handle", h);
	if (h >= 0)
	{
		memset(buf, 0, sizeof(buf));
		CHECK_EQ(F("Fread", drv), Fread((short)h, sizeof(buf), buf), sizeof(pattern));
		CHECK_TRUE(F("Fread", drv), memcmp(buf, pattern, sizeof(pattern)) == 0, "Fread data matches what was written");
		Fclose((short)h);
	}
	CHECK_EQ(F("Fopen", drv), Fopen("NOSUCH.TXT", FO_READ), EFILNF);

	// Date and time of an open file.
	t_run(F("Fdatime", drv));
	DOSTIME dt = { (3 << 11) | (4 << 5) | 3, ((2020 - 1980) << 9) | (1 << 5) | 2 };
	DOSTIME dt2 = { 0, 0 };
	h = Fopen("A.TXT", FO_RW);
	if (h >= 0)
	{
		CHECK_EQ(F("Fdatime", drv), Fdatime(&dt, (short)h, 1), 0);
		Fclose((short)h);
	}
	h = Fopen("A.TXT", FO_READ);
	if (h >= 0)
	{
		CHECK_EQ(F("Fdatime", drv), Fdatime(&dt2, (short)h, 0), 0);
		Fclose((short)h);
	}
	CHECK_TRUE(F("Fdatime", drv), dt2.time == dt.time && dt2.date == dt.date, "Fdatime read back time=0x%04x date=0x%04x, expected time=0x%04x date=0x%04x",
		dt2.time, dt2.date, dt.time, dt.date);

	// Second file, then search.
	h = Fcreate("B.TXT", 0);
	if (h >= 0)
	{
		Fwrite((short)h, 5, "hello");
		Fclose((short)h);
	}
	DTA dta;
	memset(&dta, 0, sizeof(dta));
	DTA* old_dta = Fgetdta();
	t_run(F("Fsetdta", drv));
	Fsetdta(&dta);
	CHECK_TRUE(F("Fsetdta", drv), Fgetdta() == &dta, "Fgetdta() = %p after Fsetdta(%p)", Fgetdta(), &dta);
	t_run(F("Fgetdta", drv));
	CHECK_TRUE(F("Fgetdta", drv), old_dta != 0 && old_dta != &dta, "Fgetdta() = %p before Fsetdta", old_dta);
	t_run(F("Fsfirst", drv));
	int found_a = 0, found_b = 0, n = 0;
	short r = Fsfirst("*.TXT", 0);
	CHECK_EQ(F("Fsfirst", drv), r, 0);
	while (r == 0 && n < 10)
	{
		n++;
		if (strcmp(dta.d_fname, "A.TXT") == 0)
		{
			found_a = dta.d_length == sizeof(pattern) && dta.d_time == dt.time && dta.d_date == dt.date;
		}
		if (strcmp(dta.d_fname, "B.TXT") == 0)
		{
			found_b = dta.d_length == 5;
		}
		t_run(F("Fsnext", drv));
		r = Fsnext();
	}
	CHECK_EQ(F("Fsnext", drv), r, ENMFIL);
	CHECK_TRUE(F("Fsfirst", drv), n == 2 && found_a && found_b, "found %d files, A.TXT %s, B.TXT %s", n,
		found_a ? "ok" : "missing or wrong", found_b ? "ok" : "missing or wrong");
	CHECK_EQ(F("Fsfirst", drv), Fsfirst("NOSUCH.*", 0), EFILNF);
	strcpy(tmp, base);
	*strrchr(tmp, '\\') = 0;
	strcat(tmp, "\\TFD*.*");
	r = Fsfirst(tmp, FA_SUBDIR);
	CHECK_TRUE(F("Fsfirst", drv), r == 0 && strcmp(dta.d_fname, "TFDIR") == 0 && (dta.d_attrib & FA_SUBDIR),
		"Fsfirst(%s, FA_SUBDIR) = %d name=\"%s\" attrib=0x%02x", tmp, r, dta.d_fname, dta.d_attrib);
	Fsetdta(old_dta);

	// Attributes.
	t_run(F("Fattrib", drv));
	short a = Fattrib("A.TXT", 0, 0);
	CHECK_TRUE(F("Fattrib", drv), a >= 0 && (a & (FA_SUBDIR | FA_VOLUME | FA_READONLY)) == 0, "Fattrib(A.TXT, get) = %d", a);
	CHECK_TRUE(F("Fattrib", drv), Fattrib("A.TXT", 1, FA_READONLY) >= 0, "Fattrib(A.TXT, set read only)");
	a = Fattrib("A.TXT", 0, 0);
	CHECK_TRUE(F("Fattrib", drv), a >= 0 && (a & FA_READONLY), "Fattrib(A.TXT, get) = %d, expected read only", a);
	if (drv != 2)
	{
		// Hatari's GEMDOS drive emulation (C:) uses host file semantics and doesn't enforce this.
		CHECK_EQ(F("Fattrib", drv), Fopen("A.TXT", FO_WRITE), EACCDN);
		CHECK_EQ(F("Fattrib", drv), Fdelete("A.TXT"), EACCDN);
	}
	Fattrib("A.TXT", 1, 0);
	CHECK_EQ(F("Fattrib", drv), Fattrib("A.TXT", 0, 0) & FA_READONLY, 0);

	// Rename.
	t_run(F("Frename", drv));
	CHECK_EQ(F("Frename", drv), Frename("B.TXT", "C.TXT"), 0);
	CHECK_EQ(F("Frename", drv), Fopen("B.TXT", FO_READ), EFILNF);
	h = Fopen("C.TXT", FO_READ);
	CHECK_TRUE(F("Frename", drv), h >= 0, "Fopen(C.TXT) after rename = %d", h);
	if (h >= 0)
	{
		Fclose((short)h);
	}

	// Redirect standard input to a file.
	t_run(F("Fforce", drv));
	h = Fopen("A.TXT", FO_READ);
	int saved = Fdup(0);
	CHECK_TRUE(F("Fdup", drv), saved >= 6, "Fdup(0) = %d", saved);
	CHECK_EQ(F("Fforce", drv), Fforce(0, (short)h), 0);
	memset(buf, 0, 16);
	CHECK_EQ(F("Fforce", drv), Fread(0, 5, buf), 5);
	CHECK_TRUE(F("Fforce", drv), memcmp(buf, pattern, 5) == 0, "Fread(0) reads the redirected file");
	Fforce(0, (short)saved);
	Fclose((short)saved);
	Fclose((short)h);

	// Record locking only exists with the _FLK cookie.
	t_run(F("Flock", drv));
	h = Fopen("A.TXT", FO_RW);
	if (t_cookie(0x5F464C4B, 0))
	{
		CHECK_EQ(F("Flock", drv), Flock((short)h, 0, 0, 10), 0);
		CHECK_EQ(F("Flock", drv), Flock((short)h, 1, 0, 10), 0);
	}
	else
	{
		CHECK_EQ(F("Flock", drv), Flock((short)h, 0, 0, 10), EINVFN);
	}
	Fclose((short)h);

	// Delete.
	t_run(F("Fdelete", drv));
	CHECK_EQ(F("Fdelete", drv), Fdelete("A.TXT"), 0);
	CHECK_EQ(F("Fdelete", drv), Fdelete("A.TXT"), EFILNF);
	Dsetpath("\\");
	Dsetdrv(boot_drv);
	t_run(F("Ddelete", drv));
	CHECK_EQ(F("Ddelete", drv), Ddelete(base), EACCDN);	// Not empty
	strcpy(tmp, base);
	strcat(tmp, "\\C.TXT");
	Fdelete(tmp);
	CHECK_EQ(F("Ddelete", drv), Ddelete(base), 0);
	CHECK_TRUE(F("Ddelete", drv), Ddelete(base) < 0, "Ddelete of a removed directory fails");
}

int main(void)
{
	t_begin("T_FILE");
	boot_drv = Dgetdrv();
	for (int i = 0; i < (int)sizeof(pattern); i++)
	{
		pattern[i] = (char)(i * 7 + 3);
	}
	test_dirs_and_files(2);	// C:
	test_dirs_and_files(t_env.flop);	// Floppy, handled by TOS
	return t_end();
}
