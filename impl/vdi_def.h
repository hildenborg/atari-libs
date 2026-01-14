/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#ifndef VDI_DEF_DEFINED
#define VDI_DEF_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "def_types.h"
#include "vdi.h"

extern INT16_T unused_dummy_array[16];	// Should never be used.

void vdi_words_to_bytes(const INT16_T* src, INT8_T* dst, INT16_T len);
void vdi_bytes_to_words(const INT8_T* src, INT16_T* dst, INT16_T len);
short vdi_strlen(const void* src);

#define vdi_call(vdipb) \
	__asm__ volatile ( \
		"move.l	%0, %%d1\n\t" \
		"moveq	#0x73, %%d0\n\t" \
		"trap	#2\n\t" \
		: \
		: "g" (vdipb) \
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory" \
	)

#define VDI_COPY_LONG(src, dst) \
	__asm__ volatile ( \
		"move.l	%0@, %1@\n\t" \
		: \
		: "a" (src), "a" (dst) \
		: "cc", "memory" \
	);

#define VDI_COPY_LONGS(src, dst, len) \
	for (short i = len; --i >= 0; ((unsigned int*)dst)[i] = ((unsigned int*)src)[i]) {}

#define VDI_COPY_WORDS(src, dst, len) \
	for (short i = len; --i >= 0; ((unsigned short*)dst)[i] = ((unsigned short*)src)[i]) {}

#define VDI_CAST_FROM_BYTES(src, dst, len) vdi_bytes_to_words((const INT8_T*)(src), (INT16_T*)(dst), (len))

#define VDI_CAST_TO_BYTES(src, dst, len) vdi_words_to_bytes((const INT16_T*)(src), (INT8_T*)(dst), (len))

#ifdef __cplusplus
}
#endif

#endif // VDI_DEF_DEFINED
