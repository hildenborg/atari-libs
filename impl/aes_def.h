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

INT16_T aes_calli(UINT32_T c, AESPB* aespb);
INT16_T aes_callo(UINT32_T c, AESPB* aespb);

#ifdef __cplusplus
}
#endif

#endif // AES_DEF_DEFINED
