/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "harness.h"

// Keep the TPA small so Malloc and Pexec have memory to work with.
long _stksize = 65536;

T_ENV t_env;

static const char* t_prog = "";
static int n_pass, n_fail, n_skip;
static long nf_stderr_id, nf_exit_id;

/*
	NatFeats. Hatari handles the 0x7300/0x7301 opcodes, arguments are read from the stack.
*/
__asm__(
	".globl t_nf_id\n"
	"t_nf_id:\n\t"
	".word 0x7300\n\t"
	"rts\n"
	".globl t_nf_call\n"
	"t_nf_call:\n\t"
	".word 0x7301\n\t"
	"rts\n");
long t_nf_id(const char* name);
long t_nf_call(long id, ...);

static void nf_out(const char* s)
{
	if (nf_stderr_id == 0)
	{
		nf_stderr_id = t_nf_id("NF_STDERR");
	}
	t_nf_call(nf_stderr_id, s);
}

void t_print(const char* fmt, ...)
{
	char buf[256];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, va);
	va_end(va);
	strcat(buf, "\n");
	nf_out(buf);
}

static void report(const char* status, const char* func, const char* fmt, va_list va)
{
	char buf[256];
	int n = snprintf(buf, sizeof(buf), "%s %s %s: ", status, t_prog, func);
	vsnprintf(buf + n, sizeof(buf) - n - 1, fmt, va);
	strcat(buf, "\n");
	nf_out(buf);
}

void t_run(const char* func)
{
	char buf[96];
	snprintf(buf, sizeof(buf), "RUN  %s %s\n", t_prog, func);
	nf_out(buf);
}

int t_check(int ok, const char* func, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	report(ok ? "PASS" : "FAIL", func, fmt, va);
	va_end(va);
	if (ok)
	{
		n_pass++;
	}
	else
	{
		n_fail++;
	}
	return ok;
}

void t_skip(const char* func, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	report("SKIP", func, fmt, va);
	va_end(va);
	n_skip++;
}

/*
	Raw traps.
*/
long t_super(long (*fn)(void))
{
	register long result asm ("d0");
	__asm__ volatile (
		"move.l	%1, %%a7@-\n\t"
		"move.w	#38, %%a7@-\n\t"
		"trap	#14\n\t"
		"addq.l	#6, %%a7\n\t"
		: "=r" (result)
		: "r" (fn)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}

static IOREC* raw_iorec(short dev)
{
	register IOREC* result asm ("d0");
	__asm__ volatile (
		"move.w	%1, %%a7@-\n\t"
		"move.w	#14, %%a7@-\n\t"
		"trap	#14\n\t"
		"addq.l	#4, %%a7\n\t"
		: "=r" (result)
		: "r" (dev)
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}

static short raw_sversion(void)
{
	register long result asm ("d0");
	__asm__ volatile (
		"move.w	#48, %%a7@-\n\t"
		"trap	#1\n\t"
		"addq.l	#2, %%a7\n\t"
		: "=r" (result)
		:
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return (short)result;
}

static unsigned long peek_addr;
static long peekl_fn(void)
{
	return *(volatile long*)peek_addr;
}
static long peekw_fn(void)
{
	return *(volatile short*)peek_addr;
}

long t_peekl(unsigned long addr)
{
	peek_addr = addr;
	return t_super(peekl_fn);
}

short t_peekw(unsigned long addr)
{
	peek_addr = addr;
	return (short)t_super(peekw_fn);
}

int t_cookie(unsigned long id, long* value)
{
	unsigned long jar = (unsigned long)t_peekl(0x5a0);
	if (jar == 0)
	{
		return 0;
	}
	for (int i = 0; i < 256; i++)
	{
		unsigned long cid = (unsigned long)t_peekl(jar + i * 8);
		if (cid == 0)
		{
			break;
		}
		if (cid == id)
		{
			if (value)
			{
				*value = t_peekl(jar + i * 8 + 4);
			}
			return 1;
		}
	}
	return 0;
}

/*
	Keyboard buffer injection.
	The keyboard IOREC buffer holds longs: scancode in bits 16-23, ascii in bits 0-7.
*/
static IOREC* kbd_iorec;
static long key_value;

static long put_key_fn(void)
{
	IOREC* io = kbd_iorec;
	short sr;
	__asm__ volatile ("move.w %%sr, %0\n\tori.w #0x0700, %%sr" : "=d" (sr));
	short tl = io->ibuftl + 4;
	if (tl >= io->ibufsiz)
	{
		tl = 0;
	}
	*(long*)((char*)io->ibuf + tl) = key_value;
	io->ibuftl = tl;
	__asm__ volatile ("move.w %0, %%sr" : : "d" (sr));
	return 0;
}

static long flush_keys_fn(void)
{
	kbd_iorec->ibufhd = kbd_iorec->ibuftl;
	return 0;
}

void t_key(unsigned char scancode, unsigned char ascii)
{
	kbd_iorec = raw_iorec(1);
	key_value = ((long)scancode << 16) | ascii;
	t_super(put_key_fn);
}

void t_keys(const char* str)
{
	while (*str)
	{
		unsigned char c = (unsigned char)*str++;
		t_key(c == '\r' ? 0x1c : 0x10, c);
	}
}

void t_flush_keys(void)
{
	kbd_iorec = raw_iorec(1);
	t_super(flush_keys_fn);
}

/*
	Setup.
*/
void t_begin(const char* prog)
{
	long v;
	t_prog = prog;
	n_pass = n_fail = n_skip = 0;
	nf_exit_id = 0;

	t_env.mch = MCH_ST;
	if (t_cookie(0x5F4D4348, &v))	// _MCH
	{
		t_env.mch = (int)(v >> 16);
	}
	t_env.snd = t_cookie(0x5F534E44, &v) ? v : 0;	// _SND
	t_env.vdo = t_cookie(0x5F56444F, &v) ? v : 0;	// _VDO
	unsigned long sysbase = (unsigned long)t_peekl(0x4f2);
	unsigned long osbase = (unsigned long)t_peekl(sysbase + 8);
	t_env.tos = (unsigned short)t_peekw(osbase + 2);
	t_env.emutos = t_peekl(osbase + 0x2c) == 0x45544F53;	// 'ETOS'
	t_env.gemdos = (unsigned short)raw_sversion();
	t_env.has_dsp = (t_env.snd & SND_DSP) != 0;
	t_env.flop = t_peekw(0x4a6) >= 2 ? 1 : 0;	// _nflops

	t_print("BEGIN %s mch=%d tos=%04x%s gemdos=%04x snd=%lx vdo=%lx", prog, t_env.mch, t_env.tos,
		t_env.emutos ? " (EmuTOS)" : "", t_env.gemdos, t_env.snd, t_env.vdo);
}

const char* t_fpath(const char* rest)
{
	static char bufs[4][64];
	static int n;
	char* b = bufs[n++ & 3];
	b[0] = (char)('A' + t_env.flop);
	b[1] = ':';
	strncpy(b + 2, rest, 61);
	b[63] = 0;
	return b;
}

int t_end(void)
{
	t_print("SUMMARY %s pass=%d fail=%d skip=%d", t_prog, n_pass, n_fail, n_skip);
	return n_fail;
}
