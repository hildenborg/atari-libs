/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Object, form and file selector functions, on a dialog tree built in memory (a_tree.h).
	The graphics functions are in a_graf.c.
	Modal calls (form_do, form_alert, fsel_input...) are answered by simulated keys and clicks.
*/

#include <string.h>
#include "a_util.h"

#include "a_tree.h"

static void test_objects(void)
{
	short x, y, w, h, ox, oy;
	t_run("rsrc_obfix");
	CHECK_TRUE("rsrc_obfix", tree[D_ROOT].ob_width == 30 * a_wchar && tree[D_OK].ob_y == 5 * a_hchar,
		"root width %d (30 chars of %d), OK button y %d", tree[D_ROOT].ob_width, a_wchar, tree[D_OK].ob_y);

	t_run("form_center");
	CHECK_TRUE("form_center", form_center(tree, &x, &y, &w, &h) != 0, "form_center()");
	short cx = tree[D_ROOT].ob_x + tree[D_ROOT].ob_width / 2;
	CHECK_TRUE("form_center", cx >= a_desk.g_x + a_desk.g_w / 2 - a_wchar && cx <= a_desk.g_x + a_desk.g_w / 2 + a_wchar
		&& x <= tree[D_ROOT].ob_x && w >= tree[D_ROOT].ob_width, "dialog at %d,%d, area %d,%d %dx%d", tree[D_ROOT].ob_x, tree[D_ROOT].ob_y, x, y, w, h);

	t_run("objc_offset");
	CHECK_TRUE("objc_offset", objc_offset(tree, D_OK, &ox, &oy) != 0 && ox == tree[D_ROOT].ob_x + tree[D_OK].ob_x
		&& oy == tree[D_ROOT].ob_y + tree[D_OK].ob_y, "objc_offset(OK) = %d,%d", ox, oy);

	t_run("objc_find");
	CHECK_EQ("objc_find", objc_find(tree, ROOT, MAX_DEPTH, ox + 2, oy + 2), D_OK);
	CHECK_EQ("objc_find", objc_find(tree, ROOT, MAX_DEPTH, tree[D_ROOT].ob_x + 2, tree[D_ROOT].ob_y + tree[D_ROOT].ob_height - 3), D_ROOT);
	CHECK_EQ("objc_find", objc_find(tree, ROOT, MAX_DEPTH, tree[D_ROOT].ob_x - 10, tree[D_ROOT].ob_y - 10), -1);

	t_run("form_dial");
	CHECK_TRUE("form_dial", form_dial(FMD_START, x, y, w, h, x, y, w, h) != 0, "form_dial(FMD_START)");
	CHECK_TRUE("form_dial", form_dial(FMD_GROW, 0, 0, 10, 10, x, y, w, h) != 0, "form_dial(FMD_GROW)");
	t_run("objc_draw");
	CHECK_TRUE("objc_draw", objc_draw(tree, ROOT, MAX_DEPTH, x, y, w, h) != 0, "objc_draw(tree)");

	t_run("objc_change");
	CHECK_TRUE("objc_change", objc_change(tree, D_CANCEL, 0, x, y, w, h, SELECTED, 1) != 0 && tree[D_CANCEL].ob_state == SELECTED,
		"objc_change(Cancel, SELECTED) state 0x%x", tree[D_CANCEL].ob_state);
	objc_change(tree, D_CANCEL, 0, x, y, w, h, NORMAL, 1);
	CHECK_EQ("objc_change", tree[D_CANCEL].ob_state, NORMAL);

	t_run("objc_edit");
	short idx = -1;
	objc_edit(tree, D_EDIT, 0, &idx, ED_INIT);
	CHECK_EQ("objc_edit", idx, 0);
	objc_edit(tree, D_EDIT, 0x2368, &idx, ED_CHAR);	// 'h'
	objc_edit(tree, D_EDIT, 0x1769, &idx, ED_CHAR);	// 'i'
	objc_edit(tree, D_EDIT, 0, &idx, ED_END);
	CHECK_TRUE("objc_edit", strcmp(text, "hi") == 0 && idx == 2, "edit field \"%s\", cursor %d", text, idx);

	t_run("objc_order");
	CHECK_TRUE("objc_order", objc_order(tree, D_CANCEL, 0) != 0 && tree[D_ROOT].ob_head == D_CANCEL && tree[D_CANCEL].ob_next == D_TITLE
		&& tree[D_OK].ob_next == D_CHAR, "objc_order(Cancel, 0): first child %d", tree[D_ROOT].ob_head);
	objc_order(tree, D_CANCEL, 3);
	CHECK_TRUE("objc_order", tree[D_ROOT].ob_head == D_TITLE && tree[D_OK].ob_next == D_CANCEL && tree[D_CANCEL].ob_next == D_CHAR,
		"objc_order(Cancel, 3) restores the order");

	t_run("objc_delete");
	CHECK_TRUE("objc_delete", objc_delete(tree, D_CHAR) != 0 && tree[D_ROOT].ob_tail == D_CANCEL && tree[D_CANCEL].ob_next == D_ROOT,
		"objc_delete(char box): last child %d", tree[D_ROOT].ob_tail);
	t_run("objc_add");
	CHECK_TRUE("objc_add", objc_add(tree, D_ROOT, D_CHAR) != 0 && tree[D_ROOT].ob_tail == D_CHAR && tree[D_CANCEL].ob_next == D_CHAR
		&& tree[D_CHAR].ob_next == D_ROOT, "objc_add(char box): last child %d", tree[D_ROOT].ob_tail);

	t_run("objc_sysvar");
	if (a_aesver >= 0x340)
	{
		short o1 = -1, o2 = -1;
		a_key_later(2000, 0x1c, '\r');	// In case the AES shows an "unknown function" alert
		short r = objc_sysvar(0, LK3DIND, 0, 0, &o1, &o2);
		int alert = a_key_later_done();
		CHECK_TRUE("objc_sysvar", !alert && r != 0 && (o1 == 0 || o1 == 1), "objc_sysvar(get LK3DIND) = %d, %d %d", r, o1, o2);
		a_drain_keys();
	}
	else
	{
		t_skip("objc_sysvar", "AES 0x%04x has no objc_sysvar", a_aesver);
	}
}

static void test_forms(void)
{
	short x, y, w, h, n, k;
	form_center(tree, &x, &y, &w, &h);

	t_run("form_keybd");
	short r = form_keybd(tree, D_EDIT, D_EDIT, 0x1e61, &n, &k);
	CHECK_TRUE("form_keybd", r != 0 && k == 0x1e61, "form_keybd('a') = %d, key passed on 0x%04x", r, k);
	r = form_keybd(tree, D_EDIT, D_EDIT, 0x1c0d, &n, &k);	// Return selects the default button
	CHECK_TRUE("form_keybd", r == 0 && n == D_OK, "form_keybd(Return) = %d, new object %d", r, n);
	tree[D_OK].ob_state = NORMAL;

	// form_button tracks the button until it is released, a release comes from the timer.
	t_run("form_button");
	a_mouse_to(0, 0);
	a_later_mouse(300, 0, 0, 0);
	a_later_start();
	r = form_button(tree, D_OK, 1, &n);
	a_later_done();
	CHECK_TRUE("form_button", r == 0 && n == D_OK && (tree[D_OK].ob_state & SELECTED), "form_button(OK) = %d, next %d, state 0x%x",
		r, n, tree[D_OK].ob_state);
	tree[D_OK].ob_state = NORMAL;

	// form_do: type a name and press Return.
	t_run("form_do");
	memset(text, 0, sizeof(text));
	objc_draw(tree, ROOT, MAX_DEPTH, x, y, w, h);
	a_drain_keys();
	a_later_type(300, "ab\r");
	a_later_key(2000, 0x01, 0x1b);	// Escape, in case Return is lost
	a_later_start();
	r = form_do(tree, D_EDIT);
	a_later_done();
	CHECK_TRUE("form_do", (r & 0x7fff) == D_OK && strcmp(text, "ab") == 0, "form_do() = %d, edit field \"%s\"", r, text);
	tree[D_OK].ob_state = NORMAL;

	// form_do: click Cancel.
	objc_draw(tree, ROOT, MAX_DEPTH, x, y, w, h);
	short bx, by;
	center_of(D_CANCEL, &bx, &by);
	// The click comes from the timer while form_do runs, like a user would click.
	a_mouse_to(bx, by);
	a_later_mouse(300, 0, 0, 1);
	a_later_mouse(100, 0, 0, 0);
	a_later_key(2000, 0x1c, '\r');	// In case the click is lost
	a_later_start();
	r = form_do(tree, 0);
	a_later_done();
	CHECK_TRUE("form_do", (r & 0x7fff) == D_CANCEL, "form_do() after a click on Cancel = %d", r);
	tree[D_CANCEL].ob_state = NORMAL;
	a_drain_keys();

	t_run("form_dial");
	CHECK_TRUE("form_dial", form_dial(FMD_SHRINK, 0, 0, 10, 10, x, y, w, h) != 0, "form_dial(FMD_SHRINK)");
	CHECK_TRUE("form_dial", form_dial(FMD_FINISH, x, y, w, h, x, y, w, h) != 0, "form_dial(FMD_FINISH)");

	t_run("form_alert");
	a_key_later(300, 0x1c, '\r');
	r = form_alert(2, "[2][Test alert|Return picks the default][One|Two|Three]");
	a_later_done();
	CHECK_EQ("form_alert", r, 2);
	a_key_later(300, 0x1c, '\r');
	r = form_alert(1, "[1][Second alert][Yes|No]");
	a_later_done();
	CHECK_EQ("form_alert", r, 1);

	// The alert must wait for the key (sent after 500 ms).
	t_run("form_error");
	a_key_later(500, 0x1c, '\r');
	r = form_error(2);	// File not found
	int sent = a_key_later_done();
	CHECK_TRUE("form_error", r >= 0 && sent == 1, "form_error(2) = %d, alert closed by Return: %s", r, sent ? "yes" : "no");
	a_drain_keys();
}


static void test_fsel(void)
{
	char path[128], file[16];
	short b = -1;
	if (t_cookie(0x4E564449, 0))
	{
		t_skip("fsel_input", "the file selector doesn't close under NVDI here (tested without NVDI)");
		t_skip("fsel_exinput", "the file selector doesn't close under NVDI here (tested without NVDI)");
		return;
	}
	strcpy(path, "C:\\AESTEST\\*.*");
	strcpy(file, "TEST.RSC");
	t_run("fsel_input");
	a_drain_keys();
	a_later_key(500, 0x1c, '\r');
	a_later_key(2000, 0x1c, '\r');	// Again, in case the first one came too early
	a_later_start();
	short r = fsel_input(path, file, &b);
	a_later_done();
	CHECK_TRUE("fsel_input", r != 0 && b == 1 && strcmp(file, "TEST.RSC") == 0 && strncmp(path, "C:\\AESTEST\\", 11) == 0,
		"fsel_input() = %d, button %d, \"%s\" \"%s\"", r, b, path, file);
	a_drain_keys();

	t_run("fsel_exinput");
	b = -1;
	a_later_key(500, 0x1c, '\r');
	a_later_key(2000, 0x1c, '\r');
	a_later_start();
	r = fsel_exinput(path, file, &b, "Test title");
	a_later_done();
	CHECK_TRUE("fsel_exinput", r != 0 && b == 1 && strcmp(file, "TEST.RSC") == 0, "fsel_exinput() = %d, button %d, \"%s\"", r, b, file);
	a_drain_keys();
}

// MagiC / AES 4.1 extensions, not in these TOS versions.
static void test_extensions(void)
{
	static const char* funcs[] =
	{
		"objc_xedit", "form_xdo", "form_xdial", "objc_wdraw", "objc_wchange", "graf_wwatchbox", "form_wbutton",
		"form_wkeybd", "objc_wedit", "form_popup", "xfrm_popup", "form_xerr", 0
	};
	for (int i = 0; funcs[i]; i++)
	{
		t_skip(funcs[i], "MagiC / AES 4.1 extension, not in AES 0x%04x", a_aesver);
	}
}

int main(void)
{
	if (!a_begin("A_OBJC"))
	{
		return t_end();
	}
	build_tree();
	test_objects();
	test_forms();
	test_fsel();
	test_extensions();
	return a_end();
}
