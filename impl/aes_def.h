/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#ifndef AES_DEF_DEFINED
#define AES_DEF_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "def_types.h"
#include "aes.h"

extern INT16_T _aes_global[16];
extern void* aes_unused_dummy_addr[16];
extern INT16_T aes_unused_dummy_int[16];

void aes_call(AESPB* aespb);
void _appl_yield(void);

/*
	Trampolines for callbacks called with Pure C conventions (aes_tramp.c).
*/
void _aes_tramp_hndl_obj(void);
void _aes_tramp_set_item(void);
void _aes_tramp_slct_item(void);
void _aes_tramp_xfrm_popup(void);
void _aes_tramp_pdlg_hndl(void);
void _aes_tramp_utxt(void);

void _aes_hndl_pending(HNDL_OBJ fn);
INT16_T _aes_hndl_register(DIALOG* dialog, HNDL_OBJ fn);
void _aes_hndl_unregister(DIALOG* dialog);
void _aes_lbox_pending(SET_ITEM set, SLCT_ITEM slct);
INT16_T _aes_lbox_register(LIST_BOX* box, SET_ITEM set, SLCT_ITEM slct);
void _aes_lbox_unregister(LIST_BOX* box);
void _aes_xfrm_popup_set(XFRM_POPUP_CB fn);
INT16_T _aes_pdlg_subs_register(PRN_DIALOG* dialog, PDLG_SUB* subs);
void _aes_pdlg_subs_unregister(PRN_DIALOG* dialog);
INT16_T _aes_fnts_register(FNT_DIALOG* dialog, FNTS_ITEM* items);
void _aes_fnts_unregister(FNT_DIALOG* dialog);

#define AES_COPY_LONG(src, dst) \
	__asm__ volatile ( \
		"move.l	%0@, %1@\n\t" \
		: \
		: "a" (src), "a" (dst) \
		: "cc", "memory" \
	);

#ifdef __cplusplus
}
#endif

#endif // AES_DEF_DEFINED
