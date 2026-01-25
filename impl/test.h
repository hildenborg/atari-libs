/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	This file is only used when vdi library is built for testing.
*/

#ifndef TEST_DEFINED
#define TEST_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "def_types.h"
#include "vdi.h"

#ifdef testing

extern INT16_T glbl_handle;
#define MAX_TEST_ARRAY 512
typedef INT16_T (*TEST_CALLBACK)(FILE*);
INT16_T test_all_calls(FILE* fp);

extern WS test_WS_struct;
extern MFDB test_MFDB_struct;
extern RGB1000 test_RGB1000_struct;
extern COLOR_RGB test_COLOR_RGB_struct;
extern COLOR_CMYK test_COLOR_CMYK_struct;
extern COLOR_ENTRY test_COLOR_ENTRY_struct;
extern COLOR_TAB test_COLOR_TAB_struct;
extern COLOR_TAB256 test_COLOR_TAB256_struct;
extern COLOR_TAB256 ctab_ref_COLOR_TAB256_struct;
extern COLOR_TAB256 itab_ref_COLOR_TAB256_struct;
extern GCBITMAP test_GCBITMAP_struct;
extern RECT16 test_RECT16_struct;
extern RECT32 test_RECT32_struct;
extern XFNT_INFO test_XFNT_INFO_struct;
INT16_T test_INT16_VDI_CB_callback(void /*INT16_T mstatus*/);


#endif

#ifdef __cplusplus
}
#endif

#endif // TEST_DEFINED
