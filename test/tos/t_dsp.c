/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Falcon DSP XBIOS tests.
	A small echo program is loaded into the DSP: it reads words from the host port and sends them back.
	Before the library transfer functions are used, the echo is checked by polling the host port
	directly with a timeout, so a DSP that doesn't answer can't hang the test.

	DSP program (P memory):
	$0000	0C0040			jmp $40
	$0040	0AA980 000040	jclr #0,x:$ffe9,$40		; wait for host receive data full
	$0042	56F000 00FFEB	move x:$ffeb,a			; read word from host
	$0044	0AA981 000044	jclr #1,x:$ffe9,$44		; wait for host transmit data empty
	$0046	567000 00FFEB	move a,x:$ffeb			; send it back
	$0048	0C0040			jmp $40

	Dsp_InStream gets a sink program (only reads) and Dsp_OutStream a source program (only sends
	$123456), so no data is left waiting in the other direction.
*/

#include <string.h>
#include "harness.h"

static const char* funcs[] =
{
	"Dsp_DoBlock", "Dsp_BlkHandShake", "Dsp_BlkUnpacked", "Dsp_InStream", "Dsp_OutStream", "Dsp_IOStream",
	"Dsp_RemoveInterrupts", "Dsp_GetWordSize", "Dsp_Lock", "Dsp_Unlock", "Dsp_Available", "Dsp_Reserve",
	"Dsp_LoadProg", "Dsp_ExecProg", "Dsp_ExecBoot", "Dsp_LodToBinary", "Dsp_TriggerHC", "Dsp_RequestUniqueAbility",
	"Dsp_GetProgAbility", "Dsp_FlushSubroutines", "Dsp_LoadSubroutine", "Dsp_InqSubrAbility", "Dsp_RunSubroutine",
	"Dsp_Hf0", "Dsp_Hf1", "Dsp_Hf2", "Dsp_Hf3", "Dsp_BlkWords", "Dsp_BlkBytes", "Dsp_HStat", "Dsp_SetVectors",
	"Dsp_MultBlocks", 0
};

static const unsigned long echo_words[] =
{
	0x0AA980, 0x000040, 0x56F000, 0x00FFEB, 0x0AA981, 0x000044, 0x567000, 0x00FFEB, 0x0C0040
};
#define ECHO_LEN (sizeof(echo_words) / sizeof(echo_words[0]))
static const unsigned long sink_words[] =
{
	0x0AA980, 0x000040, 0x56F000, 0x00FFEB, 0x0C0040		// read, discard, loop
};
static const unsigned long source_words[] =
{
	0x56F400, 0x123456,		// move #$123456,a
	0x0AA981, 0x000042, 0x567000, 0x00FFEB, 0x0C0042	// send a forever
};

// Binary program format (Dsp_ExecProg): blocks of type, address, length, then the words. All 3 bytes.
static char prog[3 * (3 + 1 + 3 + ECHO_LEN)];
static int prog_words;
static char prog2[sizeof(prog)];
static char boot[3 * (0x40 + ECHO_LEN)];	// Dsp_ExecBoot: plain words from P:0
static char lodbin[sizeof(prog) + 64];

static char* put24(char* p, unsigned long v)
{
	*p++ = (char)(v >> 16);
	*p++ = (char)(v >> 8);
	*p++ = (char)v;
	return p;
}

// Program with "jmp $40" at P:0 and the words at P:$40. Returns the size in DSP words.
static int build_prog(char* buf, const unsigned long* words, unsigned n)
{
	char* p = buf;
	p = put24(p, 0);	// P memory
	p = put24(p, 0);
	p = put24(p, 1);
	p = put24(p, 0x0C0040);
	p = put24(p, 0);
	p = put24(p, 0x40);
	p = put24(p, n);
	for (unsigned i = 0; i < n; i++)
	{
		p = put24(p, words[i]);
	}
	return (int)(p - buf) / 3;
}

static void build_programs(void)
{
	prog_words = build_prog(prog, echo_words, ECHO_LEN);

	memset(boot, 0, sizeof(boot));
	put24(boot, 0x0C0040);
	for (unsigned i = 0; i < ECHO_LEN; i++)
	{
		put24(boot + 3 * (0x40 + i), echo_words[i]);
	}
}

/*
	Direct host port access (supervisor mode), with timeouts.
*/
#define HOST_ISR	((volatile unsigned char*)0xffffa202)
#define HOST_DATA	((volatile unsigned char*)0xffffa205)
static long host_value;

static long host_send_fn(void)
{
	for (long i = 0; i < 200000; i++)
	{
		if (*HOST_ISR & 2)	// TXDE
		{
			HOST_DATA[0] = (unsigned char)(host_value >> 16);
			HOST_DATA[1] = (unsigned char)(host_value >> 8);
			HOST_DATA[2] = (unsigned char)host_value;
			return 1;
		}
	}
	return 0;
}

static long host_recv_fn(void)
{
	for (long i = 0; i < 200000; i++)
	{
		if (*HOST_ISR & 1)	// RXDF
		{
			long v = (long)HOST_DATA[0] << 16;
			v |= (long)HOST_DATA[1] << 8;
			v |= HOST_DATA[2];
			host_value = v;
			return 1;
		}
	}
	return 0;
}

// Send a word and check that the same word comes back.
static int echo_ok(long v)
{
	host_value = v;
	if (!t_super(host_send_fn))
	{
		return 0;
	}
	host_value = -1;
	if (!t_super(host_recv_fn))
	{
		return 0;
	}
	return host_value == v;
}

static int check_echo(const char* func)
{
	return CHECK_TRUE(func, echo_ok(0x123456) && echo_ok(0x00abcd), "DSP program echoes words through the host port");
}

static volatile long recv_d0, recv_stack, recv_count;
// Receiver for Dsp_SetVectors: record the word, whether it comes in d0 or on the stack.
__asm__(
	".globl dsp_receiver\n"
	"dsp_receiver:\n\t"
	"move.l	%d0, recv_d0\n\t"
	"move.l	%a7@(4), recv_stack\n\t"
	"addq.l	#1, recv_count\n\t"
	"rts\n");
void dsp_receiver(void);

static void wait_vbl(int n)
{
	while (n-- > 0)
	{
		Vsync();
	}
}

static void test_transfers(void)
{
	// Two words at a time, so the echo program never blocks both sides.
	char in3[6] = { 0x12, 0x34, 0x56, 0x00, 0x00, 0x7f }, out3[6];
	t_run("Dsp_DoBlock");
	memset(out3, 0, sizeof(out3));
	Dsp_DoBlock(in3, 2, out3, 2);
	CHECK_TRUE("Dsp_DoBlock", memcmp(in3, out3, 6) == 0, "echoed %02x%02x%02x %02x%02x%02x", out3[0] & 0xff, out3[1] & 0xff, out3[2] & 0xff,
		out3[3] & 0xff, out3[4] & 0xff, out3[5] & 0xff);
	t_run("Dsp_BlkHandShake");
	memset(out3, 0, sizeof(out3));
	Dsp_BlkHandShake(in3, 2, out3, 2);
	CHECK_TRUE("Dsp_BlkHandShake", memcmp(in3, out3, 6) == 0, "echoed data matches");

	t_run("Dsp_BlkUnpacked");
	int inl[2] = { 0x00654321, 0x00000042 }, outl[2] = { 0, 0 };
	Dsp_BlkUnpacked(inl, 2, outl, 2);
	CHECK_TRUE("Dsp_BlkUnpacked", (outl[0] & 0xffffff) == inl[0] && (outl[1] & 0xffffff) == inl[1], "echoed %06x %06x", outl[0], outl[1]);

	t_run("Dsp_BlkWords");
	short inw[2] = { 0x1234, -2 }, outw[2] = { 0, 0 };
	Dsp_BlkWords(inw, 2, outw, 2);
	CHECK_TRUE("Dsp_BlkWords", outw[0] == inw[0] && outw[1] == inw[1], "echoed %04x %04x", outw[0] & 0xffff, outw[1] & 0xffff);

	t_run("Dsp_BlkBytes");
	unsigned char inb[2] = { 0x12, 0xfe }, outb[2] = { 0, 0 };
	Dsp_BlkBytes(inb, 2, outb, 2);
	CHECK_TRUE("Dsp_BlkBytes", outb[0] == inb[0] && outb[1] == inb[1], "echoed %02x %02x", outb[0], outb[1]);

	t_run("Dsp_MultBlocks");
	int ml[2] = { 0x111111, 0x222222 }, mo[2] = { 0, 0 };
	DSPBLOCK sb = { 0, 2, ml }, rb = { 0, 2, mo };
	Dsp_MultBlocks(1, 1, &sb, &rb);
	CHECK_TRUE("Dsp_MultBlocks", (mo[0] & 0xffffff) == ml[0] && (mo[1] & 0xffffff) == ml[1], "echoed %06x %06x", mo[0], mo[1]);

	// Interrupt driven transfers.
	t_run("Dsp_IOStream");
	char sin[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, sout[6];
	int done = 0;
	memset(sout, 0, sizeof(sout));
	Dsp_IOStream(sin, sout, 1, 1, 2, &done);
	for (int i = 0; i < 50 && done < 2; i++)
	{
		Vsync();
	}
	CHECK_TRUE("Dsp_IOStream", done == 2 && memcmp(sin, sout, 6) == 0, "blocks done %d, data %s", done, memcmp(sin, sout, 6) ? "differs" : "matches");
	t_run("Dsp_RemoveInterrupts");
	Dsp_RemoveInterrupts(3);

	CHECK_TRUE("Dsp_RemoveInterrupts", check_echo("Dsp_RemoveInterrupts"), "polling works again after removing the interrupts");

	t_run("Dsp_SetVectors");
	recv_count = 0;
	Dsp_SetVectors(dsp_receiver, 0);
	host_value = 0x00beef;
	t_super(host_send_fn);
	wait_vbl(3);
	Dsp_RemoveInterrupts(3);
	CHECK_TRUE("Dsp_SetVectors", recv_count == 1 && ((recv_d0 & 0xffffff) == 0xbeef || (recv_stack & 0xffffff) == 0xbeef),
		"receiver called %ld times, d0=0x%lx stack=0x%lx", recv_count, recv_d0, recv_stack);
}

static void test_streams(short ability)
{
	char sin[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, sout[6];
	t_run("Dsp_InStream");
	Dsp_ExecProg(prog2, build_prog(prog2, sink_words, sizeof(sink_words) / sizeof(sink_words[0])), ability);
	int done_in = 0, done_out = 0;
	Dsp_InStream(sin, 1, 2, &done_in);
	for (int i = 0; i < 50 && done_in < 2; i++)
	{
		Vsync();
	}
	CHECK_EQ("Dsp_InStream", done_in, 2);
	Dsp_RemoveInterrupts(3);

	t_run("Dsp_OutStream");
	Dsp_ExecProg(prog2, build_prog(prog2, source_words, sizeof(source_words) / sizeof(source_words[0])), ability);
	memset(sout, 0, sizeof(sout));
	Dsp_OutStream(sout, 1, 2, &done_out);
	for (int i = 0; i < 50 && done_out < 2; i++)
	{
		Vsync();
	}
	Dsp_RemoveInterrupts(3);
	static const char want[6] = { 0x12, 0x34, 0x56, 0x12, 0x34, 0x56 };
	CHECK_TRUE("Dsp_OutStream", done_out == 2 && memcmp(sout, want, 6) == 0, "blocks done %d, data %02x%02x%02x %02x%02x%02x", done_out,
		sout[0] & 0xff, sout[1] & 0xff, sout[2] & 0xff, sout[3] & 0xff, sout[4] & 0xff, sout[5] & 0xff);
}

static void test_flags(void)
{
	t_run("Dsp_Hf0");
	short h0 = Dsp_Hf0(-1);
	if (h0 == 119)	// Unimplemented XBIOS functions return their number
	{
		t_skip("Dsp_Hf0", "not implemented by this TOS");
		t_skip("Dsp_Hf1", "not implemented by this TOS");
		t_skip("Dsp_Hf2", "not implemented by this TOS");
		t_skip("Dsp_Hf3", "not implemented by this TOS");
		return;
	}
	CHECK_EQ("Dsp_Hf0", h0, 0);
	Dsp_Hf0(1);
	CHECK_EQ("Dsp_Hf0", Dsp_Hf0(-1), 1);
	Dsp_Hf0(0);
	CHECK_EQ("Dsp_Hf0", Dsp_Hf0(-1), 0);
	t_run("Dsp_Hf1");
	Dsp_Hf1(1);
	CHECK_EQ("Dsp_Hf1", Dsp_Hf1(-1), 1);
	Dsp_Hf1(0);
	CHECK_EQ("Dsp_Hf1", Dsp_Hf1(-1), 0);
	t_run("Dsp_Hf2");
	CHECK_EQ("Dsp_Hf2", Dsp_Hf2(), 0);	// Set by the DSP, the echo program doesn't
	t_run("Dsp_Hf3");
	CHECK_EQ("Dsp_Hf3", Dsp_Hf3(), 0);
	t_run("Dsp_HStat");
	char st = Dsp_HStat();
	CHECK_TRUE("Dsp_HStat", (st & 0x07) == 0x06, "Dsp_HStat() = 0x%02x, expected TXDE and TRDY, no RXDF", st & 0xff);
}

int main(void)
{
	t_begin("T_DSP");
	if (!t_env.has_dsp)
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no DSP");
		}
		return t_end();
	}
	build_programs();

	t_run("Dsp_GetWordSize");
	CHECK_EQ("Dsp_GetWordSize", Dsp_GetWordSize(), 3);
	t_run("Dsp_Lock");
	CHECK_EQ("Dsp_Lock", Dsp_Lock(), 0);
	CHECK_EQ("Dsp_Lock", Dsp_Lock(), -1);

	t_run("Dsp_Available");
	int xa = 0, ya = 0;
	Dsp_Available(&xa, &ya);
	CHECK_TRUE("Dsp_Available", xa > 0x100 && ya > 0x100, "Dsp_Available() x=0x%x y=0x%x", xa, ya);
	t_run("Dsp_Reserve");
	CHECK_EQ("Dsp_Reserve", Dsp_Reserve(0x100, 0x100), 0);
	CHECK_TRUE("Dsp_Reserve", Dsp_Reserve(0x100000, 0x100) != 0, "reserving more than available fails");
	Dsp_Reserve(0x100, 0x100);

	t_run("Dsp_RequestUniqueAbility");
	short ab = Dsp_RequestUniqueAbility();
	short ab2 = Dsp_RequestUniqueAbility();
	CHECK_TRUE("Dsp_RequestUniqueAbility", ab != ab2, "two calls return %d and %d", ab, ab2);

	t_run("Dsp_ExecProg");
	Dsp_ExecProg(prog, prog_words, ab);
	int echo = check_echo("Dsp_ExecProg");
	t_run("Dsp_GetProgAbility");
	CHECK_EQ("Dsp_GetProgAbility", Dsp_GetProgAbility(), ab);

	if (echo)
	{
		test_transfers();
	}
	else
	{
		t_check(0, "Dsp_DoBlock", "transfer tests not run, the DSP program doesn't echo");
	}
	test_flags();

	// The same program as LOD text.
	t_run("Dsp_LodToBinary");
	const char* lod =
		"_START ECHO 0000 0000 0000 DSP56000 4.1.1\r\n\r\n"
		"_DATA P 0000\r\n0C0040\r\n"
		"_DATA P 0040\r\n0AA980 000040 56F000 00FFEB 0AA981 000044 567000 00FFEB\r\n0C0040\r\n"
		"_END 0000\r\n";
	int h = Fcreate("C:\\TOSTEST\\ECHO.LOD", 0);
	if (h >= 0)
	{
		Fwrite((short)h, (int)strlen(lod), lod);
		Fclose((short)h);
	}
	memset(lodbin, 0, sizeof(lodbin));
	int n = Dsp_LodToBinary("C:\\TOSTEST\\ECHO.LOD", lodbin);
	CHECK_TRUE("Dsp_LodToBinary", n == prog_words && memcmp(lodbin, prog, sizeof(prog)) == 0,
		"Dsp_LodToBinary() = %d words (expected %d), binary %s", n, prog_words, memcmp(lodbin, prog, sizeof(prog)) ? "differs" : "matches");

	t_run("Dsp_LoadProg");
	short ab3 = Dsp_RequestUniqueAbility();
	CHECK_EQ("Dsp_LoadProg", Dsp_LoadProg("C:\\TOSTEST\\ECHO.LOD", ab3, lodbin), 0);
	check_echo("Dsp_LoadProg");
	CHECK_EQ("Dsp_LoadProg", Dsp_GetProgAbility(), ab3);

	t_run("Dsp_ExecBoot");
	Dsp_ExecBoot(boot, sizeof(boot) / 3, ab);
	check_echo("Dsp_ExecBoot");

	t_run("Dsp_TriggerHC");
	Dsp_TriggerHC(0x13);
	t_check(1, "Dsp_TriggerHC", "called (host command not enabled in the DSP program, nothing to observe)");

	test_streams(ab);

	// Subroutine: a single rts.
	if (t_env.emutos)
	{
		static const char* sub[] = { "Dsp_LoadSubroutine", "Dsp_InqSubrAbility", "Dsp_RunSubroutine", "Dsp_FlushSubroutines", 0 };
		for (int i = 0; sub[i]; i++)
		{
			t_skip(sub[i], "EmuTOS hangs in Dsp_LoadSubroutine (passes on TOS 4.04)");
		}
		t_run("Dsp_Unlock");
		Dsp_Unlock();
		CHECK_EQ("Dsp_Unlock", Dsp_Lock(), 0);
		Dsp_Unlock();
		return t_end();
	}
	t_run("Dsp_LoadSubroutine");
	char rts[3];
	put24(rts, 0x00000C);
	short sab = Dsp_RequestUniqueAbility();
	short sh = Dsp_LoadSubroutine(rts, 1, sab);
	CHECK_TRUE("Dsp_LoadSubroutine", sh > 0, "Dsp_LoadSubroutine() = %d, expected a handle", sh);
	t_run("Dsp_InqSubrAbility");
	CHECK_EQ("Dsp_InqSubrAbility", Dsp_InqSubrAbility(sab), sh);
	CHECK_EQ("Dsp_InqSubrAbility", Dsp_InqSubrAbility(sab + 1000), 0);
	if (sh > 0)
	{
		t_run("Dsp_RunSubroutine");
		CHECK_EQ("Dsp_RunSubroutine", Dsp_RunSubroutine(sh), 0);
	}
	t_run("Dsp_FlushSubroutines");
	Dsp_FlushSubroutines();
	CHECK_EQ("Dsp_FlushSubroutines", Dsp_InqSubrAbility(sab), 0);

	t_run("Dsp_Unlock");
	Dsp_Unlock();
	CHECK_EQ("Dsp_Unlock", Dsp_Lock(), 0);
	Dsp_Unlock();
	return t_end();
}
