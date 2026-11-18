/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Escapes (opcode 5): text mode, printer, metafile, camera, tablet and NVDI printer escapes.
	Most only exist in device drivers that aren't here, they get binding tests only.
	The text mode escapes are also tested on the screen.
*/

#include <string.h>
#include "v_util.h"

#define CANARY 0x5aa5

// An escape without arguments.
static void esc0(const char* fn, void (*f)(short), short sub)
{
	t_run(fn);
	v_fake(0, 0);
	f(FH);
	v_off();
	V_IN(fn, 5, sub, 0, 0, 0, 0);
}

// An escape with two intin words.
static void esc2(const char* fn, void (*f)(short, short, short), short sub, short a, short b)
{
	t_run(fn);
	v_fake(0, 0);
	f(FH, a, b);
	v_off();
	V_IN(fn, 5, sub, 0, 2, W(a, b), 0);
}

// An escape returning two intout words.
static void escq2(const char* fn, void (*f)(short, short*, short*), short sub)
{
	t_run(fn);
	short a = 0, b = 0;
	v_fake(2, 0);
	f(FH, &a, &b);
	v_off();
	V_IN(fn, 5, sub, 0, 0, 0, 0);
	v_words(fn, "outputs", W(a, b), W(0x100, 0x101), 2);
}

// An escape with one intin word, returning intout[0].
static void esc1r(const char* fn, short (*f)(short, short), short sub, short a)
{
	t_run(fn);
	v_fake(1, 0);
	short r = f(FH, a);
	v_off();
	V_IN(fn, 5, sub, 0, 1, W(a), 0);
	CHECK_EQ(fn, r, 0x100);
}

static void binding_tests(void)
{
	short r, a, b;

	// Text mode.
	escq2("vq_chcells", vq_chcells, 1);
	esc0("v_exit_cur", v_exit_cur, 2);
	esc0("v_enter_cur", v_enter_cur, 3);
	esc0("v_curup", v_curup, 4);
	esc0("v_curdown", v_curdown, 5);
	esc0("v_curright", v_curright, 6);
	esc0("v_curleft", v_curleft, 7);
	esc0("v_curhome", v_curhome, 8);
	esc0("v_eeos", v_eeos, 9);
	esc0("v_eeol", v_eeol, 10);
	esc2("v_curaddress", v_curaddress, 11, 5, 12);
	esc2("vs_curaddress", vs_curaddress, 11, 6, 13);
	t_run("v_curtext");
	v_fake(0, 0);
	v_curtext(FH, "Ab\x84");
	v_off();
	V_IN("v_curtext", 5, 12, 0, 3, W('A', 'b', 0x84), 0);
	esc0("v_rvon", v_rvon, 13);
	esc0("v_rvoff", v_rvoff, 14);
	escq2("vq_curaddress", vq_curaddress, 15);
	t_run("vq_tabstatus");
	v_fake(1, 0);
	r = vq_tabstatus(FH);
	v_off();
	V_IN("vq_tabstatus", 5, 16, 0, 0, 0, 0);
	CHECK_EQ("vq_tabstatus", r, 0x100);
	esc0("v_hardcopy", v_hardcopy, 17);
	t_run("v_dspcur");
	v_fake(0, 0);
	v_dspcur(FH, 33, 44);
	v_off();
	V_IN("v_dspcur", 5, 18, 1, 0, 0, W(33, 44));
	esc0("v_rmcur", v_rmcur, 19);

	// Printer.
	esc0("v_form_adv", v_form_adv, 20);
	t_run("v_output_window");
	short xy[4] = { 10, 20, 300, 400 };
	v_fake(0, 0);
	v_output_window(FH, xy);
	v_off();
	V_IN("v_output_window", 5, 21, 2, 0, 0, xy);
	esc0("v_clear_disp_list", v_clear_disp_list, 22);
	// v_bit_image: intin = aspect, x scale, y scale, h align, v align, file name; ptsin = the rectangle.
	t_run("v_bit_image");
	v_fake(0, 0);
	v_bit_image(FH, "A.IMG", 1, 2, 3, 4, 5, xy);
	v_off();
	V_IN("v_bit_image", 5, 23, 2, 10, W(1, 2, 3, 4, 5, 'A', '.', 'I', 'M', 'G'), xy);
	t_run("vq_scan");
	short sc[6] = { 0, 0, 0, 0, 0, CANARY };
	v_fake(5, 0);
	vq_scan(FH, &sc[0], &sc[1], &sc[2], &sc[3], &sc[4]);
	v_off();
	V_IN("vq_scan", 5, 24, 0, 0, 0, 0);
	v_words("vq_scan", "outputs", sc, W(0x100, 0x101, 0x102, 0x103, 0x104, CANARY), 6);
	t_run("v_alpha_text");
	v_fake(0, 0);
	v_alpha_text(FH, "Hi\x84");
	v_off();
	V_IN("v_alpha_text", 5, 25, 0, 3, W('H', 'i', 0x84), 0);
	esc1r("vs_palette", vs_palette, 60, 2);
	esc2("v_sound", v_sound, 61, 440, 18);
	esc1r("vs_mute", vs_mute, 62, -1);

	// Tablet.
	t_run("vt_resolution");
	a = b = 0;
	v_fake(2, 0);
	vt_resolution(FH, 100, 200, &a, &b);
	v_off();
	V_IN("vt_resolution", 5, 81, 0, 2, W(100, 200), 0);
	v_words("vt_resolution", "outputs", W(a, b), W(0x100, 0x101), 2);
	t_run("vt_axis");
	a = b = 0;
	v_fake(2, 0);
	vt_axis(FH, 101, 201, &a, &b);
	v_off();
	V_IN("vt_axis", 5, 82, 0, 2, W(101, 201), 0);
	v_words("vt_axis", "outputs", W(a, b), W(0x100, 0x101), 2);
	esc2("vt_origin", vt_origin, 83, 7, 8);
	escq2("vq_tdimensions", vq_tdimensions, 84);
	esc2("vt_alignment", vt_alignment, 85, 9, 10);

	// Polaroid Palette.
	t_run("vqp_films");
	char films[130];
	memset(films, 'X', sizeof(films));
	v_fake(125, 0);
	for (int i = 0; i < 125; i++) v_fake_intout[i] = 'a' + i % 25;
	vqp_films(FH, films);
	v_off();
	V_IN("vqp_films", 5, 91, 0, 0, 0, 0);
	t_check(films[0] == 'a' && films[24] == 'y' && films[25] == 'a' && films[124] == 'y', "vqp_films", "film names \"%.30s...\"", films);
	esc2("vsp_film", vsp_film, 91, 3, 4);
	// vqp_state: intout = port, film number, lightness, interlace, planes, 16 colour indices.
	t_run("vqp_state");
	short port = 0, light = 0, inter = 0, planes = 0, idx[17];
	char film = 0;
	idx[16] = CANARY;
	v_fake(21, 0);
	vqp_state(FH, &port, &film, &light, &inter, &planes, idx);
	v_off();
	V_IN("vqp_state", 5, 92, 0, 0, 0, 0);
	v_words("vqp_state", "port, film, lightness, interlace, planes", W(port, film, light, inter, planes), W(0x100, 0x01, 0x102, 0x103, 0x104), 5);
	v_words("vqp_state", "indexes", idx, W(0x105, 0x106, 0x107, 0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10d, 0x10e, 0x10f, 0x110, 0x111, 0x112, 0x113, 0x114, CANARY), 17);
	t_run("vsp_state");
	short ind[16];
	for (int i = 0; i < 16; i++) ind[i] = 20 + i;
	v_fake(0, 0);
	vsp_state(FH, 1, 2, 3, 4, 5, ind);
	v_off();
	V_IN("vsp_state", 5, 93, 0, 21, W(1, 2, 3, 4, 5, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35), 0);
	esc0("vsp_save", vsp_save, 94);
	esc0("vsp_message", vsp_message, 95);
	t_run("vqp_error");
	v_fake(1, 0);
	r = vqp_error(FH);
	v_off();
	V_IN("vqp_error", 5, 96, 0, 0, 0, 0);
	CHECK_EQ("vqp_error", r, 0x100);
	// Camera.
	t_run("vsc_expose");
	v_fake(0, 0);
	vsc_expose(FH, 1);
	v_off();
	V_IN("vsc_expose", 5, 93, 0, 1, W(1), 0);

	// Metafile.
	t_run("v_meta_extents");
	v_fake(0, 0);
	v_meta_extents(FH, 1, 2, 300, 400);
	v_off();
	V_IN("v_meta_extents", 5, 98, 2, 0, 0, W(1, 2, 300, 400));
	// v_write_meta: num_pts is the number of points (contrl[1]).
	t_run("v_write_meta");
	v_fake(0, 0);
	v_write_meta(FH, 3, W(101, 11, 12), 2, W(1, 2, 3, 4));
	v_off();
	V_IN("v_write_meta", 5, 99, 2, 3, W(101, 11, 12), W(1, 2, 3, 4));
	t_run("vm_coords");
	v_fake(0, 0);
	vm_coords(FH, 0, 1000, 1000, 0);
	v_off();
	V_IN("vm_coords", 5, 99, 0, 5, W(1, 0, 1000, 1000, 0), 0);
	t_run("vm_pagesize");
	v_fake(0, 0);
	vm_pagesize(FH, 2100, 2970);
	v_off();
	V_IN("vm_pagesize", 5, 99, 0, 3, W(0, 2100, 2970), 0);
	t_run("vm_filename");
	v_fake(0, 0);
	vm_filename(FH, "T.GEM");
	v_off();
	V_IN("vm_filename", 5, 100, 0, 5, W('T', '.', 'G', 'E', 'M'), 0);

	// GDOS/printer.
	t_run("v_offset");
	v_fake(0, 0);
	v_offset(FH, 12);
	v_off();
	V_IN("v_offset", 5, 101, 0, 1, W(12), 0);
	t_run("v_fontinit");
	v_fake(0, 0);
	v_fontinit(FH, 0x0012, 0x3456);
	v_off();
	V_IN("v_fontinit", 5, 102, 0, 2, W(0x0012, 0x3456), 0);
	t_run("v_escape2000");
	v_fake(0, 0);
	v_escape2000(FH, 3);
	v_off();
	V_IN("v_escape2000", 5, 2000, 0, 1, W(3), 0);

	// NVDI printer escapes.
	esc1r("v_orient", v_orient, 27, 1);
	esc1r("v_copies", v_copies, 28, 2);
	t_run("vq_margins");
	short mg[7];
	v_fake(7, 0);
	r = vq_margins(FH, &mg[0], &mg[1], &mg[2], &mg[3], &mg[4], &mg[5]);
	v_off();
	V_IN("vq_margins", 5, 2100, 0, 0, 0, 0);
	CHECK_EQ("vq_margins", r, 0x100);
	v_words("vq_margins", "margins, dpi", mg, W(0x101, 0x102, 0x103, 0x104, 0x105, 0x106), 6);
	// vs_document_info: intin = type, then the characters.
	t_run("vs_document_info");
	v_fake(1, 0);
	r = vs_document_info(FH, 2, "Doc\x84", 0);
	v_off();
	V_IN("vs_document_info", 5, 2103, 0, 5, W(2, 'D', 'o', 'c', 0x84), 0);
	CHECK_EQ("vs_document_info", r, 0x100);
	v_fake(1, 0);
	r = vs_document_info(FH, 3, (const char*)W('W', 0x20ac, 0), 1);
	v_off();
	V_IN("vs_document_info", 5, 2103, 0, 3, W(3, 'W', 0x20ac), 0);
	// vq_prn_scaling: intin = -1, -1; the scale factor in intout[0..1], -1 when not supported.
	t_run("vq_prn_scaling");
	v_fake(2, 0);
	v_fake_intout[0] = 0x0001;
	v_fake_intout[1] = 0x8000;
	long sf = vq_prn_scaling(FH);
	v_off();
	V_IN("vq_prn_scaling", 5, 39, 0, 2, W(-1, -1), 0);
	CHECK_EQ("vq_prn_scaling", sf, 0x00018000L);
	v_fake(0, 0);
	sf = vq_prn_scaling(FH);
	v_off();
	CHECK_EQ("vq_prn_scaling", sf, -1L);
}

static void real_tests(void)
{
	short h = v_handle, rows = -1, cols = -1, r, c;

	// Text cell size from Line-A: V_CEL_HT at -46, V_CEL_MX at -44, V_CEL_MY at -42.
	short cel_h = v_linea(-46), cel_mx = v_linea(-44), cel_my = v_linea(-42);
	t_run("vq_chcells");
	vq_chcells(h, &rows, &cols);
	t_check(rows == cel_my + 1 && cols == cel_mx + 1, "vq_chcells", "%d rows %d columns (Line-A %d %d)", rows, cols, cel_my + 1, cel_mx + 1);

	t_run("vq_tabstatus");
	r = vq_tabstatus(h);
	t_check(r == 0 || r == 1, "vq_tabstatus", "vq_tabstatus() = %d", r);

	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, 0);
	t_run("v_enter_cur");
	v_enter_cur(h);
	t_check(1, "v_enter_cur", "returned");

#define AT(fn, wr, wc) do { r = c = -1; vq_curaddress(h, &r, &c); \
		t_check(r == (wr) && c == (wc), fn, "cursor at %d,%d, expected %d,%d", r, c, wr, wc); } while (0)

	t_run("v_curaddress");
	v_curaddress(h, 5, 10);
	t_run("vq_curaddress");
	AT("v_curaddress", 5, 10);
	t_run("v_curdown");
	v_curdown(h);
	AT("v_curdown", 6, 10);
	t_run("v_curright");
	v_curright(h);
	AT("v_curright", 6, 11);
	t_run("v_curup");
	v_curup(h);
	AT("v_curup", 5, 11);
	t_run("v_curleft");
	v_curleft(h);
	AT("v_curleft", 5, 10);
	t_run("v_curtext");
	v_curtext(h, "ab");
	AT("v_curtext", 5, 12);
	t_run("vs_curaddress");
	vs_curaddress(h, 3, 4);
	AT("vs_curaddress", 3, 4);
	t_run("v_curhome");
	v_curhome(h);
	AT("v_curhome", 1, 1);

	// Text on row 3 (pixels from (row - 1) * cell height), the cursor stays at home.
	short ty = 2 * cel_h, tw = 8 * 8;
	v_curaddress(h, 3, 1);
	v_curtext(h, "########");
	int n1 = v_count_ink(0, ty, tw, cel_h);
	t_run("v_eeol");
	v_curaddress(h, 3, 5);
	v_eeol(h);
	v_curhome(h);	// the text cursor itself is ink
	int n2 = v_count_ink(0, ty, tw / 2, cel_h), n3 = v_count_ink(tw / 2, ty, tw / 2, cel_h);
	t_check(n1 > 40 && n2 > 20 && n3 == 0, "v_eeol", "'#' x 8: %d pixels, after erasing from column 5: %d left, %d right", n1, n2, n3);
	t_run("v_rvon");
	v_curaddress(h, 4, 1);
	v_rvon(h);
	v_curtext(h, "  ");
	n1 = v_count_ink(0, ty + cel_h, 16, cel_h);
	CHECK_EQ("v_rvon", n1, 16 * cel_h);
	t_run("v_rvoff");
	v_rvoff(h);
	v_curaddress(h, 4, 1);
	v_curtext(h, "  ");
	n1 = v_count_ink(0, ty + cel_h, 16, cel_h);
	CHECK_EQ("v_rvoff", n1, 0);
	t_run("v_eeos");
	v_curaddress(h, 4, 1);
	v_rvon(h);
	v_curtext(h, "  ");
	v_rvoff(h);
	v_curaddress(h, 2, 1);
	v_eeos(h);
	n1 = v_count_ink(0, ty, tw, 2 * cel_h);
	CHECK_EQ("v_eeos", n1, 0);
	v_curhome(h);

	t_run("v_exit_cur");
	v_exit_cur(h);
	t_check(1, "v_exit_cur", "returned");
	graf_mouse(M_ON, 0);
	wind_update(END_UPDATE);
	form_dial(FMD_FINISH, 0, 0, 0, 0, 0, 0, v_scr_w, v_scr_h);
	a_wait(200);

	// v_dspcur moves the mouse cursor, v_rmcur removes it.
	t_run("v_dspcur");
	v_dspcur(h, 77, 66);
	t_check(1, "v_dspcur", "returned");
	t_run("v_rmcur");
	v_rmcur(h);
	t_check(1, "v_rmcur", "returned");
	v_show_c(h, 0);
	graf_mouse(ARROW, 0);
}

int main(void)
{
	if (v_begin("V_ESC"))
	{
		binding_tests();
		real_tests();
	}
	return v_end();
}
