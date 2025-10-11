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

void vdi_call(void);
INT16_T vdi_zero_ended_string_to_words(INT8_T* src, INT16_T* dst);
void vdi_words_to_bytes(INT16_T* src, INT8_T* dst, INT16_T len);
void vdi_bytes_to_words(INT8_T* src, INT16_T* dst, INT16_T len);
//extern void vdi_large_generic_copy(void* src, void* dst, INT16_T len);

#define VDI_COPY_LONG(src, dst) \
	__asm__ volatile ( \
		"move.l	%0@+, %1@+\n\t" \
		: \
		: "a" (src), "a" (dst) \
		: \
	);

#define VDI_COPY_WORD(src, dst) \
	__asm__ volatile ( \
		"move.w	%0@+, %1@+\n\t" \
		: \
		: "a" (src), "a" (dst) \
		: \
	);

#define VDI_SET_WORD(value, dst) \
	__asm__ volatile ( \
		"move.w	#%0, %1@+\n\t" \
		: \
		: "g" (value), "a" (dst) \
		: \
	);

#define VDI_COPY_LONGS(src, dst, len) \
	__asm__ volatile ( \
		"1:\n\t" \
		"move.l	%0@+, %1@+\n\t" \
		"dbra	%2, 1b\n\t" \
		: \
		: "a" (src), "a" (dst), "d" (len - 1) \
		: \
	);

#define VDI_COPY_WORDS(src, dst, len) \
	__asm__ volatile ( \
		"1:\n\t" \
		"move.w	%0@+, %1@+\n\t" \
		"dbra	%2, 1b\n\t" \
		: \
		: "a" (src), "a" (dst), "d" (len - 1) \
		: \
	);

#define VDI_CAST_FROM_BYTE(src, dst) \
__asm__ volatile ( \
	"clr.b	%1@+\n\t" \
	"move.b	%0@+, %1@+\n\t" \
	: \
	: "a" (src), "a" (dst) \
	: \
);

#define VDI_CAST_TO_BYTE(src, dst) \
__asm__ volatile ( \
	"addq.l	#1, %0@+\n\t" \
	"move.b	%0@+, %1@+\n\t" \
	: \
	: "a" (src), "a" (dst) \
	: \
);

#define VDI_CAST_FROM_BYTES(src, dst, len) vdi_bytes_to_words((INT8_T*)(src), (INT16_T*)(dst), (len))

#define VDI_CAST_TO_BYTES(src, dst, len) vdi_words_to_bytes((INT16_T*)(src), (INT8_T*)(dst), (len))

#ifdef __cplusplus
}
#endif

#endif // VDI_DEF_DEFINED
