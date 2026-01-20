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
