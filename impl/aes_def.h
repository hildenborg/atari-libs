/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#ifndef AES_DEF_DEFINED
#define AES_DEF_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "def_types.h"
#include "aes.h"

extern INT16_T aes_global[16];
extern void* aes_unused_dummy_addr[16];
extern INT16_T aes_unused_dummy_int[16];

INT16_T aes_call(AESPB* aespb);

#ifdef __cplusplus
}
#endif

#endif // AES_DEF_DEFINED
