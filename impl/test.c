/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/
#include "test.h"

INT16_T glbl_handle;


WS test_WS_struct = {0};
MFDB test_MFDB_struct = {0};
RGB1000 test_RGB1000_struct = {0};
COLOR_RGB test_COLOR_RGB_struct = {0};
COLOR_CMYK test_COLOR_CMYK_struct = {0};
COLOR_ENTRY test_COLOR_ENTRY_struct = {.rgb={0}};
COLOR_TAB test_COLOR_TAB_struct = {0};
COLOR_TAB256 test_COLOR_TAB256_struct = {0};
COLOR_TAB256 ctab_ref_COLOR_TAB256_struct = {0};
COLOR_TAB256 itab_ref_COLOR_TAB256_struct = {0};
GCBITMAP test_GCBITMAP_struct = {.ctab = (CTAB_REF)&ctab_ref_COLOR_TAB256_struct, .itab = (ITAB_REF)&itab_ref_COLOR_TAB256_struct};
RECT16 test_RECT16_struct = {0};
RECT32 test_RECT32_struct = {0};
XFNT_INFO test_XFNT_INFO_struct = {0};

INT16_T test_INT16_VDI_CB_callback(void /*INT16_T mstatus*/)
{
	INT16_T mstatus;
	__asm__ volatile (
		"move.w	%%d0, %0\n\t"
		:
		: "g" (&mstatus)
		:
	);
	// mstatus is mouse buttons pressed.
	return mstatus;
}

#include "vdi_testCalls.c"	// Ugly but functional

INT16_T test_all_calls(FILE* fp)
{
	INT16_T result = 0;
	TEST_CALLBACK *calls = testCalls;
	while (1)
	{
		TEST_CALLBACK call = *calls++;
		if (call == 0)
		{
			break;
		}
		result = call(fp);
		if (result != 0)
		{
			break;
		}
	}
	return result;
}
