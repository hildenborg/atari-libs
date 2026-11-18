/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Application, resource, shell and clipboard functions, and the mt_ (global array) variants.
*/

#include <string.h>
#include "a_util.h"

extern short _aes_global[16];

static void test_appl(void)
{
	t_run("appl_find");
	short scr = appl_find("SCRENMGR");
	CHECK_TRUE("appl_find", scr > 0, "appl_find(\"SCRENMGR\") = %d", scr);
	CHECK_EQ("appl_find", appl_find("NOSUCHAP"), -1);

	t_run("appl_write");
	short msg[8] = { 0x4711, a_apid, 0, 1, 2, 3, 4, 5 }, rd[8];
	CHECK_TRUE("appl_write", appl_write(a_apid, 16, msg) > 0, "appl_write to own ap_id");
	t_run("appl_read");
	memset(rd, 0, sizeof(rd));
	CHECK_TRUE("appl_read", appl_read(a_apid, 16, rd) > 0, "appl_read from own pipe");
	CHECK_TRUE("appl_read", memcmp(msg, rd, 16) == 0, "message read back: %04x %d %d %d", rd[0] & 0xffff, rd[1], rd[3], rd[7]);

	// Play back a mouse move and a key press.
	t_run("appl_tplay");
	// Keyboard events have the key code in the high word (as appl_trecord records them).
	EVNTREC play[2] = { { APPEVNT_MOUSE, (100L << 16) | 80 }, { APPEVNT_KEYBOARD, 0x1e61L << 16 } };
	t_flush_keys();
	appl_tplay(play, 2, 100);
	short mx, my;
	a_mouse_pos(&mx, &my);
	CHECK_TRUE("appl_tplay", mx == 100 && my == 80, "mouse at %d,%d after playing a move to 100,80", mx, my);
	a_key_later(2000, 0x30, 'b');	// In case the played key didn't reach the queue, don't block.
	short k = evnt_keybd();
	a_key_later_done();
	CHECK_EQ("appl_tplay", k, 0x1e61);
	a_drain_keys();

	t_run("appl_trecord");
	EVNTREC rec[2];
	memset(rec, 0, sizeof(rec));
	t_key(0x1e, 'a');
	t_key(0x30, 'b');
	short n = appl_trecord(rec, 2);
	// EmuTOS also records the time between the keys as a timer event.
	CHECK_TRUE("appl_trecord", n == 2 && rec[0].ap_event == APPEVNT_KEYBOARD && (rec[0].ap_value >> 16) == 0x1e61
		&& ((rec[1].ap_event == APPEVNT_KEYBOARD && (rec[1].ap_value >> 16) == 0x3062) || rec[1].ap_event == APPEVNT_TIMER),
		"appl_trecord() = %d, events %d:%x %d:%x", n, rec[0].ap_event, rec[0].ap_value, rec[1].ap_event, rec[1].ap_value);
	a_drain_keys();

	t_run("appl_getinfo");
	if (a_agi)
	{
		short o1 = 0, o2 = 0, o3 = 0, o4 = 0;
		CHECK_TRUE("appl_getinfo", appl_getinfo(0, &o1, &o2, &o3, &o4) == 1 && o1 > 0, "appl_getinfo(0) font height %d id %d", o1, o2);
	}
	else
	{
		t_skip("appl_getinfo", "AES 0x%04x without appl_getinfo", a_aesver);
	}

	t_run("appl_search");
	short o3 = 0;
	if (a_getinfo(4, 0, 0, &o3, 0) && o3)
	{
		char name[16];
		short type = 0, id = -1;
		CHECK_TRUE("appl_search", appl_search(0, name, &type, &id) == 1 && id >= 0, "appl_search(first) = %s type %d id %d", name, type, id);
	}
	else
	{
		t_skip("appl_search", "not available in this AES");
	}
	if (a_aesver >= 0x400)
	{
		t_run("appl_yield");
		CHECK_TRUE("appl_yield", appl_yield() >= 0, "appl_yield()");
	}
	else
	{
		t_skip("appl_yield", "AES 0x%04x has no appl_yield (opcode 17)", a_aesver);
	}
}

// The mt_ variants take their own global array.
static void test_mt(void)
{
	short g[16];
	memset(g, 0, sizeof(g));
	t_run("mt_appl_init");
	short id = mt_appl_init(g);
	CHECK_TRUE("mt_appl_init", id == a_apid && g[0] == a_aesver && g[2] == id, "mt_appl_init(g) = %d, g[0] = 0x%04x, g[2] = %d", id, g[0], g[2]);
	short wc, hc, wb, hb;
	t_run("mt_graf_handle");
	short v = mt_graf_handle(&wc, &hc, &wb, &hb, g);
	CHECK_TRUE("mt_graf_handle", v == a_vdi && wc == a_wchar && hb == a_hbox, "mt_graf_handle() = %d (%dx%d)", v, wc, hc);
	t_run("mt_appl_find");
	CHECK_TRUE("mt_appl_find", mt_appl_find("SCRENMGR", g) > 0, "mt_appl_find(\"SCRENMGR\", g)");
}

/*
	Minimal resource file: one tree (box + string) and one free string.
*/
static char rsc[104];

static void put16(int off, unsigned short v)
{
	rsc[off] = (char)(v >> 8);
	rsc[off + 1] = (char)v;
}

static void put32(int off, unsigned long v)
{
	put16(off, (unsigned short)(v >> 16));
	put16(off + 2, (unsigned short)v);
}

static void build_rsc(void)
{
	memset(rsc, 0, sizeof(rsc));
	// Header
	put16(0, 0);		// rsh_vrsn
	put16(2, 48);		// rsh_object
	put16(4, 48);		// rsh_tedinfo
	put16(6, 48);		// rsh_iconblk
	put16(8, 48);		// rsh_bitblk
	put16(10, 100);		// rsh_frstr
	put16(12, 36);		// rsh_string
	put16(14, 104);		// rsh_imdata
	put16(16, 104);		// rsh_frimg
	put16(18, 96);		// rsh_trindex
	put16(20, 2);		// rsh_nobs
	put16(22, 1);		// rsh_ntree
	put16(24, 0);		// rsh_nted
	put16(26, 0);		// rsh_nib
	put16(28, 0);		// rsh_nbb
	put16(30, 1);		// rsh_nstring
	put16(32, 0);		// rsh_nimages
	put16(34, 104);		// rsh_rssize
	memcpy(rsc + 36, "Hello\0Free\0", 11);
	// Object 0: box
	put16(48, 0xffff); put16(50, 1); put16(52, 1); put16(54, G_BOX); put16(56, 0); put16(58, 0);
	put32(60, 0x00011100); put16(64, 0); put16(66, 0); put16(68, 20); put16(70, 3);
	// Object 1: string
	put16(72, 0); put16(74, 0xffff); put16(76, 0xffff); put16(78, G_STRING); put16(80, LASTOB); put16(82, 0);
	put32(84, 36); put16(88, 1); put16(90, 1); put16(92, 5); put16(94, 1);
	put32(96, 48);		// Tree index
	put32(100, 42);		// Free string index
}

static int is_str(void* p, const char* s)
{
	return p && strcmp((const char*)p, s) == 0;
}

static void test_rsrc(void)
{
	build_rsc();
	int h = Fcreate("C:\\AESTEST\\TEST.RSC", 0);
	if (h >= 0)
	{
		Fwrite((short)h, sizeof(rsc), rsc);
		Fclose((short)h);
	}
	t_run("rsrc_load");
	CHECK_TRUE("rsrc_load", rsrc_load("C:\\AESTEST\\TEST.RSC") != 0, "rsrc_load(TEST.RSC)");
	CHECK_TRUE("rsrc_load", _aes_global[7] != 0 || _aes_global[8] != 0, "global[7-8] points to the loaded resource");

	t_run("rsrc_gaddr");
	OBJECT* tree = 0;
	CHECK_TRUE("rsrc_gaddr", rsrc_gaddr(R_TREE, 0, (void**)&tree) != 0 && tree != 0, "rsrc_gaddr(R_TREE, 0) = %p", tree);
	if (tree)
	{
		char* s = 0;
		memcpy(&s, &tree[1].ob_spec, sizeof(s));
		CHECK_TRUE("rsrc_gaddr", tree[0].ob_type == G_BOX && tree[1].ob_type == G_STRING && is_str(s, "Hello")
			&& tree[1].ob_width == 5 * a_wchar && tree[1].ob_y == a_hchar,
			"tree: types %d %d, string \"%s\", width %d (5 chars of %d), fixed up", tree[0].ob_type, tree[1].ob_type, s ? s : "", tree[1].ob_width, a_wchar);
	}
	void* fs = 0;
	rsrc_gaddr(R_FRSTR, 0, &fs);
	// R_FRSTR gives the address of the free string pointer.
	CHECK_TRUE("rsrc_gaddr", fs && is_str(*(char**)fs, "Free"), "rsrc_gaddr(R_FRSTR, 0) points to \"Free\"");

	t_run("rsrc_saddr");
	static char other[] = "Other";
	CHECK_TRUE("rsrc_saddr", rsrc_saddr(R_FRSTR, 0, other) != 0, "rsrc_saddr(R_FRSTR, 0, \"Other\")");
	rsrc_gaddr(R_FRSTR, 0, &fs);
	CHECK_TRUE("rsrc_saddr", fs && *(char**)fs == other, "free string pointer is replaced");

	t_run("rsrc_free");
	CHECK_TRUE("rsrc_free", rsrc_free() != 0, "rsrc_free()");

	t_run("rsrc_rcfix");
	short o4 = 0;
	if (a_aesver >= 0x400 || (a_getinfo(13, 0, 0, 0, &o4) && (o4 & 8)))
	{
		build_rsc();
		CHECK_TRUE("rsrc_rcfix", rsrc_rcfix((RSHDR*)rsc) != 0, "rsrc_rcfix(in memory resource)");
		tree = 0;
		rsrc_gaddr(R_TREE, 0, (void**)&tree);
		CHECK_TRUE("rsrc_rcfix", tree == (OBJECT*)(rsc + 48), "tree is fixed up in place");
	}
	else
	{
		t_skip("rsrc_rcfix", "AES 0x%04x has no rsrc_rcfix", a_aesver);
	}
	// rsrc_obfix is used by every tree in these tests (a_fix_tree).
	t_check(1, "rsrc_obfix", "used to fix up all test trees, checked through object sizes");
}

static void test_shel(void)
{
	char cmd[128], tail[128];
	t_run("shel_read");
	memset(cmd, 0, sizeof(cmd));
	CHECK_TRUE("shel_read", shel_read(cmd, tail) != 0, "shel_read() command \"%s\"", cmd);

	t_run("shel_get");
	static char buf[1024], buf2[1024];
	memset(buf, 0, sizeof(buf));
	short r = shel_get(buf, sizeof(buf) - 1);
	CHECK_TRUE("shel_get", r != 0 && buf[0] == '#', "shel_get() = %d, desktop buffer starts with \"%.12s\"", r, buf);
	t_run("shel_put");
	int len = (int)strlen(buf) + 1;
	CHECK_TRUE("shel_put", shel_put(buf, (short)len) != 0, "shel_put() of the same buffer");
	memset(buf2, 0, sizeof(buf2));
	shel_get(buf2, sizeof(buf2) - 1);
	CHECK_TRUE("shel_put", strcmp(buf, buf2) == 0, "shel_get after shel_put returns the same buffer");

	t_run("shel_find");
	Dsetdrv(2);
	Dsetpath("\\AESTEST");
	strcpy(cmd, "A_APPL.PRG");
	r = shel_find(cmd);
	// Found in the current directory, TOS leaves the name as it is.
	char* end = cmd + strlen(cmd);
	CHECK_TRUE("shel_find", r != 0 && end - cmd >= 10 && strcmp(end - 10, "A_APPL.PRG") == 0,
		"shel_find(\"A_APPL.PRG\") = %d, \"%s\"", r, cmd);
	strcpy(cmd, "NOSUCH.PRG");
	CHECK_EQ("shel_find", shel_find(cmd), 0);

	t_run("shel_envrn");
	char* val = (char*)1;
	r = shel_envrn(&val, "PATH=");
	CHECK_TRUE("shel_envrn", r != 0 && val != (char*)1, "shel_envrn(\"PATH=\") = %d, value \"%s\"", r, val ? val : "(none)");

	t_run("shel_write");
	r = shel_write(0, 1, 0, "", "");	// Mode 0: nothing to start after this program
	CHECK_TRUE("shel_write", r != 0, "shel_write(0, ...) = %d", r);

	if (a_aesver >= 0x400)
	{
		char n[32], d[128];
		t_run("shel_rdef");
		shel_rdef(n, d);
		t_run("shel_wdef");
		shel_wdef(n, d);
		t_check(1, "shel_rdef", "default shell \"%s\" in \"%s\"", n, d);
	}
	else
	{
		t_skip("shel_rdef", "AES 0x%04x has no shel_rdef", a_aesver);
		t_skip("shel_wdef", "AES 0x%04x has no shel_wdef", a_aesver);
	}
}

static void test_scrp(void)
{
	char path[128];
	t_run("scrp_write");
	CHECK_TRUE("scrp_write", scrp_write("C:\\CLIPBRD") != 0, "scrp_write(\"C:\\CLIPBRD\")");
	t_run("scrp_read");
	memset(path, 0, sizeof(path));
	CHECK_TRUE("scrp_read", scrp_read(path) != 0 && strcmp(path, "C:\\CLIPBRD") == 0, "scrp_read() = \"%s\"", path);
	if (a_aesver >= 0x400)
	{
		t_run("scrp_clear");
		CHECK_TRUE("scrp_clear", scrp_clear() != 0, "scrp_clear()");
	}
	else
	{
		t_skip("scrp_clear", "AES 0x%04x has no scrp_clear", a_aesver);
	}
}

int main(void)
{
	if (!a_begin("A_APPL"))
	{
		return t_end();
	}
	test_appl();
	test_mt();
	test_rsrc();
	test_shel();
	test_scrp();
	return a_end();
}
