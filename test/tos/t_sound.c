/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Falcon sound XBIOS tests (needs the CODEC, _SND bit 2).
*/

#include <string.h>
#include "harness.h"

#define SNDNOTLOCK	-128
#define SNDLOCKED	-129

static const char* funcs[] =
{
	"Locksnd", "Unlocksnd", "Soundcmd", "Setbuffer", "Setmode", "Settracks", "Setmontracks", "Setinterrupt",
	"Buffoper", "Dsptristate", "Gpio", "Devconnect", "Sndstatus", "Buffptr", "WavePlay", 0
};

static void wait_vbl(int n)
{
	while (n-- > 0)
	{
		Vsync();
	}
}

int main(void)
{
	t_begin("T_SOUND");
	if (!(t_env.snd & SND_CODEC))
	{
		for (int i = 0; funcs[i]; i++)
		{
			t_skip(funcs[i], "no Falcon CODEC");
		}
		return t_end();
	}

	t_run("Locksnd");
	CHECK_EQ("Locksnd", Locksnd(), 1);
	CHECK_EQ("Locksnd", Locksnd(), SNDLOCKED);
	t_run("Unlocksnd");
	CHECK_EQ("Unlocksnd", Unlocksnd(), 0);
	CHECK_EQ("Unlocksnd", Unlocksnd(), SNDNOTLOCK);
	Locksnd();

	t_run("Soundcmd");
	int att = Soundcmd(0, -1);	// LTATTEN inquire
	CHECK_TRUE("Soundcmd", att >= 0, "Soundcmd(LTATTEN, inquire) = 0x%x", att);
	Soundcmd(0, 0x50);
	CHECK_EQ("Soundcmd", Soundcmd(0, -1) & 0xf0, 0x50);
	Soundcmd(0, att);
	int pre = Soundcmd(6, -1);	// SETPRESCALE inquire
	CHECK_TRUE("Soundcmd", pre >= 0 && pre <= 3, "Soundcmd(SETPRESCALE, inquire) = %d", pre);

	// Play a short buffer with DMA and check that the play pointer moves.
	short* buf = Mxalloc(8000, 0);
	if (buf == 0)
	{
		t_check(0, "Setbuffer", "no ST-RAM for the sound buffer");
		Unlocksnd();
		return t_end();
	}
	memset(buf, 0, 8000);
	t_run("Setbuffer");
	CHECK_EQ("Setbuffer", Setbuffer(0, buf, (char*)buf + 8000), 0);
	t_run("Setmode");
	CHECK_EQ("Setmode", Setmode(1), 0);	// 16 bit stereo
	t_run("Settracks");
	CHECK_EQ("Settracks", Settracks(0, 0), 0);
	t_run("Setmontracks");
	CHECK_EQ("Setmontracks", Setmontracks(0), 0);
	t_run("Setinterrupt");
	CHECK_EQ("Setinterrupt", Setinterrupt(0, 0), 0);
	t_run("Devconnect");
	int dc = Devconnect(0, 8, 0, 1, 1);	// DMA play -> DAC, 25 MHz clock, 49 kHz, no handshake
	if (t_env.emutos)
	{
		CHECK_TRUE("Devconnect", dc >= 0, "Devconnect() = %d (EmuTOS returns 1, TOS 4.04 returns 0)", dc);
	}
	else
	{
		CHECK_EQ("Devconnect", dc, 0);
	}
	t_run("Buffoper");
	CHECK_EQ("Buffoper", Buffoper(-1) & 3, 0);
	CHECK_EQ("Buffoper", Buffoper(3), 0);	// Play, repeat
	CHECK_EQ("Buffoper", Buffoper(-1) & 3, 3);
	wait_vbl(2);
	t_run("Buffptr");
	void* ptr[4] = { 0, 0, 0, 0 };
	CHECK_EQ("Buffptr", Buffptr((int*)ptr), 0);
	CHECK_TRUE("Buffptr", ptr[0] >= (void*)buf && ptr[0] <= (void*)((char*)buf + 8000),
		"play pointer %p inside the buffer %p-%p", ptr[0], buf, (char*)buf + 8000);
	CHECK_EQ("Buffoper", Buffoper(0), 0);
	CHECK_EQ("Buffoper", Buffoper(-1) & 3, 0);

	t_run("Sndstatus");
	int st = Sndstatus(0);
	CHECK_EQ("Sndstatus", st & 0x0f, 0);	// No error
	CHECK_EQ("Sndstatus", Sndstatus(1), 0);	// Reset

	t_run("Dsptristate");
	CHECK_EQ("Dsptristate", Dsptristate(1, 1), 0);
	CHECK_EQ("Dsptristate", Dsptristate(0, 0), 0);

	t_run("Gpio");
	int g = Gpio(1, 0);	// Read
	CHECK_TRUE("Gpio", g >= 0 && g <= 7, "Gpio(read) = %d", g);

	t_run("WavePlay");
	short wp = WavePlay(0, 25175, buf, 64);
	if (wp == 165 || wp == EINVFN)
	{
		t_skip("WavePlay", "not available in this TOS (returned %d)", wp);
	}
	else
	{
		CHECK_EQ("WavePlay", wp, 0);
	}

	Mfree(buf);
	Unlocksnd();
	return t_end();
}
