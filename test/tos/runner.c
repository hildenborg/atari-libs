/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Runs from the AUTO folder. Starts each test program with a raw Pexec (so a broken
	library Pexec can't stop the run), then exits Hatari with the total number of failures.
	A test program that crashes only loses its own results.
*/

#include "harness.h"

// The program list and directory can be set at compile time (the AES suite uses its own).
#ifndef TEST_PROGS
#define TEST_PROGS "T_GEMDOS", "T_FILE", "T_BIOS", "T_XBIOS", "T_DISK", "T_EXT", "T_SOUND", "T_MINT", "T_DSP"
#endif
#ifndef TEST_DIR
#define TEST_DIR "C:\\TOSTEST\\"
#endif

static const char* progs[] =
{
	TEST_PROGS, 0
};

static long raw_pexec0(const char* name, const char* cmd)
{
	register long result asm ("d0");
	__asm__ volatile (
		"clr.l	%%a7@-\n\t"
		"move.l	%2, %%a7@-\n\t"
		"move.l	%1, %%a7@-\n\t"
		"clr.w	%%a7@-\n\t"
		"move.w	#75, %%a7@-\n\t"
		"trap	#1\n\t"
		"lea	%%a7@(16), %%a7\n\t"
		: "=r" (result)
		: "r" (name), "r" (cmd)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}

__asm__(
	".globl r_nf_id\n"
	"r_nf_id:\n\t"
	".word 0x7300\n\t"
	"rts\n"
	".globl r_nf_call\n"
	"r_nf_call:\n\t"
	".word 0x7301\n\t"
	"rts\n");
long r_nf_id(const char* name);
long r_nf_call(long id, ...);

int main(void)
{
	char path[64];
	long failures = 0;
	t_begin("RUNNER");
	for (int i = 0; progs[i]; i++)
	{
		const char* p = progs[i];
		int n = 0;
		const char* pre = TEST_DIR;
		while (*pre)
		{
			path[n++] = *pre++;
		}
		while (*p)
		{
			path[n++] = *p++;
		}
		path[n++] = '.';
		path[n++] = 'P';
		path[n++] = 'R';
		path[n++] = 'G';
		path[n] = 0;
		long rc = raw_pexec0(path, "\0");
		if (rc == EFILNF)
		{
			t_print("RESULT %s missing", progs[i]);
			continue;
		}
		t_print("RESULT %s rc=%ld%s", progs[i], rc, (rc < 0 || rc > 1000) ? " (crashed or failed to start)" : "");
		if (rc < 0 || rc > 1000)
		{
			failures++;
		}
		else
		{
			failures += rc;
		}
	}
	t_print("TOTAL failures=%ld", failures);
	r_nf_call(r_nf_id("NF_EXIT"), failures);
	return 0;
}
