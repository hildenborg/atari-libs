/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "aes_def.h"

INT16_T mt_wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4, INT16_T* aes_global)
{
	short control[5] = {104, 2, 5, 0, 0};
	short intin[3];
	short intout[5];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		aes_unused_dummy_addr,
		aes_unused_dummy_addr
	};
	intin[0] = handle;
	intin[1] = mode;
	if(mode == WF_DCOLOR || mode == WF_COLOR)
	{
		intin[2] = *parm1;
		control[1] = 3;
	}
	INT16_T result = aes_call(&lcl_aespb);
	*parm1 = intout[1];
	*parm2 = intout[2];
	*parm3 = intout[3];
	*parm4 = intout[4];
	return result;
}

INT16_T wind_get(INT16_T handle, INT16_T mode, INT16_T* parm1, INT16_T* parm2, INT16_T* parm3, INT16_T* parm4)
{
	return mt_wind_get(handle, mode, parm1, parm2, parm3, parm4, aes_global);
}
