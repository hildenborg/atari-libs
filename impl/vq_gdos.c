/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "vdi_def.h"

INT16_T vq_gdos(void)
{
	register INT16_T result asm ("d0");
	__asm__ volatile (
		"moveq	#-2, %%d0\n\t"
		"trap	#2\n\t"
		"cmp.w	#-2, %%d0\n\t"
		"sne	%%d0\n\t"
		"ext.w	%%d0\n\t"
		: "=r" (result)
		:
		: "d1", "d2", "a0", "a1", "a2", "cc"
	);
	return result;
}

