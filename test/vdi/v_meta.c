/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	A real metafile with the NVDI META.SYS driver (device 31): the metafile escapes and v_write_meta
	are checked in the written .GEM file (little endian words: a header, then records of
	opcode, point count, intin count, subopcode, ptsin, intin).
*/

#include <string.h>
#include <tos.h>
#include "v_util.h"

#define FILE_NAME "C:\\VDITEST\\T.GEM"

static unsigned short gem[4096];
static long gem_words;

// Find a word sequence in the file, returns its position or -1.
static long find(const short* seq, int n)
{
	for (long i = 0; i + n <= gem_words; i++)
	{
		int j = 0;
		while (j < n && (short)gem[i + j] == seq[j])
		{
			j++;
		}
		if (j == n)
		{
			return i;
		}
	}
	return -1;
}

static void real_tests(void)
{
	if (!v_nvdi)
	{
		t_skip("vm_filename", "no NVDI (META.SYS)");
		return;
	}
	short work_in[11] = { 31, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 }, work_out[57], h = 0;
	t_run("v_opnwk");
	v_opnwk(work_in, &h, (WS*)work_out);
	if (!t_check(h > 0, "v_opnwk", "metafile workstation handle %d, %dx%d", h, work_out[0], work_out[1]))
	{
		return;
	}
	Fdelete(FILE_NAME);
	t_run("vm_filename");
	vm_filename(h, FILE_NAME);
	t_run("v_meta_extents");
	v_meta_extents(h, 10, 20, 300, 200);
	t_run("vm_pagesize");
	vm_pagesize(h, 2100, 2970);
	t_run("vm_coords");
	vm_coords(h, 0, 1000, 1000, 0);
	t_run("v_write_meta");
	v_write_meta(h, 3, W(101, 11, 12), 2, W(1, 2, 3, 4));
	t_run("v_pline");
	v_pline(h, 2, W(100, 110, 120, 130));
	t_run("v_clswk");
	v_clswk(h);

	short fh = Fopen(FILE_NAME, 0);
	if (!t_check(fh >= 0, "vm_filename", "metafile %s: Fopen() = %d", FILE_NAME, fh))
	{
		return;
	}
	long len = Fread(fh, sizeof(gem), gem);
	Fclose(fh);
	gem_words = len / 2;
	for (long i = 0; i < gem_words; i++)
	{
		gem[i] = (unsigned short)((gem[i] >> 8) | (gem[i] << 8));
	}
	t_print("INFO metafile %ld bytes, header %04x %d %d %d | %d %d %d %d | %d %d | %d %d %d %d", len, gem[0], gem[1], gem[2], gem[3],
		gem[4], gem[5], gem[6], gem[7], gem[8], gem[9], gem[10], gem[11], gem[12], gem[13]);
	t_check(len > 40 && gem[0] == 0xffff, "vm_filename", "%ld bytes, first word %04x", len, gem[0]);
	// Header: extents in words 4..7, page size in 8..9, coordinate window in 10..13.
	v_words("v_meta_extents", "header[4..7]", (short*)&gem[4], W(10, 20, 300, 200), 4);
	v_words("vm_pagesize", "header[8..9]", (short*)&gem[8], W(2100, 2970), 2);
	v_words("vm_coords", "header[10..13]", (short*)&gem[10], W(0, 1000, 1000, 0), 4);
	long p = find(W(5, 2, 3, 99, 1, 2, 3, 4, 101, 11, 12), 11);
	t_check(p > 0, "v_write_meta", "record 5/99 with 2 points and 3 intin words %s", p > 0 ? "found" : "not found");
	p = find(W(6, 2, 0), 3);
	t_check(p > 0 && find(W(100, 110, 120, 130), 4) == p + 4, "v_pline", "record 6 with 2 points %s", p > 0 ? "found" : "not found");
	Fdelete(FILE_NAME);
}

int main(void)
{
	if (v_begin("V_META"))
	{
		real_tests();
	}
	return v_end();
}
