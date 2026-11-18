/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Minimal test harness for running library tests in Hatari.
	Output goes to the host through Hatari NatFeats (NF_STDERR), so it works
	even when the TOS functions under test are broken.
	The harness itself only uses raw traps, never the library under test.
*/

#ifndef HARNESS_DEFINED
#define HARNESS_DEFINED

#include <tos.h>

// Machine types from the _MCH cookie (high word).
#define MCH_ST		0
#define MCH_STE		1
#define MCH_TT		2
#define MCH_FALCON	3

// _SND cookie bits.
#define SND_PSG		0x01
#define SND_DMA8	0x02
#define SND_CODEC	0x04
#define SND_DSP		0x08

// GEMDOS/BIOS error codes used in the tests.
#define E_OK		0
#define EINVFN		-32
#define EFILNF		-33
#define EPTHNF		-34
#define EACCDN		-36
#define EIHNDL		-37
#define EIMBA		-40
#define ENMFIL		-49

typedef struct
{
	int mch;				// MCH_*
	long snd;				// _SND cookie (0 if missing)
	long vdo;				// _VDO cookie (0 if missing)
	unsigned short tos;		// TOS version from OSHEADER, e.g. 0x0206
	int emutos;				// non zero for EmuTOS
	unsigned short gemdos;	// Sversion() word as returned by GEMDOS (minor in high byte)
	int has_dsp;			// DSP present and usable
	short flop;				// Floppy drive with the test disk: 1 (B:), or 0 (A:) on single drive machines
} T_ENV;

extern T_ENV t_env;

// Program setup and summary. t_end returns the number of failures (use as exit code).
void t_begin(const char* prog);
int t_end(void);

// Output a line to the host.
void t_print(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Mark that a function is about to be called (helps locating crashes in the log).
void t_run(const char* func);

// Record a test result. Returns ok.
int t_check(int ok, const char* func, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
void t_skip(const char* func, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#define CHECK_EQ(func, got, want) \
	do { long _g = (long)(got); long _w = (long)(want); \
		t_check(_g == _w, func, "%s = %ld (0x%lx), expected %ld", #got, _g, _g, _w); } while (0)

// Like CHECK_EQ, but usable as an expression that returns the result.
#define CHECK_EQ_OK(func, got, want) \
	({ long _g = (long)(got); long _w = (long)(want); \
		t_check(_g == _w, func, "%s = %ld (0x%lx), expected %ld", #got, _g, _g, _w); })

// The condition is evaluated before the message arguments, so the message shows values set by calls in it.
#define CHECK_TRUE(func, cond, ...) \
	({ int _ok = (cond) ? 1 : 0; t_check(_ok, func, __VA_ARGS__); })

// Raw helpers, independent of the library under test.
long t_super(long (*fn)(void));			// XBIOS Supexec via raw trap
long t_peekl(unsigned long addr);		// Read long from supervisor memory
short t_peekw(unsigned long addr);
int t_cookie(unsigned long id, long* value);

// Path on the test floppy: t_fpath("\\X.TXT") gives "B:\\X.TXT" (or "A:..."). Uses rotating buffers.
const char* t_fpath(const char* rest);

// Put a key into the keyboard buffer, as if it was typed.
void t_key(unsigned char scancode, unsigned char ascii);
void t_keys(const char* str);			// ascii chars, '\r' gets the Return scancode
void t_flush_keys(void);

#endif
