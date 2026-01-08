/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "aes_def.h"

#ifndef flagTreadSafe
/*
	Special handling for wind_get
*/
INT16_T wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4)
{
	UINT32_T call = (104 << 24) | (2 << 16) | (5 << 8) | 0;
	aesparblk.intin[0] = handle;
	aesparblk.intin[1] = mode;
	if(mode == WF_DCOLOR || mode == WF_COLOR)
	{
		aesparblk.intin[2] = *parm1;
		call = (104 << 24) | (3 << 16) | (5 << 8) | 0;
	}
	INT16_T result = aes_calli(call, &aespb);
	*parm1 = aesparblk.intout[1];
	*parm2 = aesparblk.intout[2];
	*parm3 = aesparblk.intout[3];
	*parm4 = aesparblk.intout[4];
	return result;
}
#else
INT16_T mt_wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4, INT16_T* aes_global)
{
	short control[5];
	short intin[3];
	short intout[5];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		aesparblk.addrin,	// Unused.
		aesparblk.addrout	// Unused.
	};
	UINT32_T call = (104 << 24) | (2 << 16) | (5 << 8) | 0;
	intin[0] = handle;
	intin[1] = mode;
	if(mode == WF_DCOLOR || mode == WF_COLOR)
	{
		intin[2] = *parm1;
		call = (104 << 24) | (3 << 16) | (5 << 8) | 0;
	}
	INT16_T result = aes_calli(call, &aespb);
	*parm1 = intout[1];
	*parm2 = intout[2];
	*parm3 = intout[3];
	*parm4 = intout[4];
	return result;
}

INT16_T wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4)
{
	return mt_wind_get(handle, mode, parm1, parm2, parm3, parm4, aesparblk.global);
}
#endif