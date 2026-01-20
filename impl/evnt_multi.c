#include "aes_def.h"

#ifdef TARGET_M68K_ATARI_MINTELF
INT16_T mt_evnt_multi(INT16_T events, INT16_T bclicks, INT16_T bmask, INT16_T bstate, INT16_T m1flag, INT16_T m1x, INT16_T m1y, INT16_T m1w, INT16_T m1h, INT16_T m2flag, INT16_T m2x, INT16_T m2y, INT16_T m2w, INT16_T m2h, INT16_T* msg, INT32_T interval, INT16_T* mx, INT16_T* my, INT16_T* button, INT16_T* kstate, INT16_T* kreturn, INT16_T* breturn, INT16_T* aes_global)
{
	INT16_T control[5] = {25, 16, 7, 1, 0};
	INT16_T intin[16];
	INT16_T intout[7];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = events;
	intin[1] = bclicks;
	intin[2] = bmask;
	intin[3] = bstate;
	intin[4] = m1flag;
	intin[5] = m1x;
	intin[6] = m1y;
	intin[7] = m1w;
	intin[8] = m1h;
	intin[9] = m2flag;
	intin[10] = m2x;
	intin[11] = m2y;
	intin[12] = m2w;
	intin[13] = m2h;
	*(INT32_T*)(&intin[14]) = interval;
	addrin[0] = (void*)msg;
	INT16_T result = (INT16_T)aes_call(&lcl_aespb);
	if(mx) {*mx = intout[1];}
	if(my) {*my = intout[2];}
	if(button) {*button = intout[3];}
	if(kstate) {*kstate = intout[4];}
	if(kreturn) {*kreturn = intout[5];}
	if(breturn) {*breturn = intout[6];}
	return result;
}

INT16_T evnt_multi(INT16_T events, INT16_T bclicks, INT16_T bmask, INT16_T bstate, INT16_T m1flag, INT16_T m1x, INT16_T m1y, INT16_T m1w, INT16_T m1h, INT16_T m2flag, INT16_T m2x, INT16_T m2y, INT16_T m2w, INT16_T m2h, INT16_T* msg, INT32_T interval, INT16_T* mx, INT16_T* my, INT16_T* button, INT16_T* kstate, INT16_T* kreturn, INT16_T* breturn)
{
	return mt_evnt_multi(events, bclicks, bmask, bstate, m1flag, m1x, m1y, m1w, m1h, m2flag, m2x, m2y, m2w, m2h, msg, interval, mx, my, button, kstate, kreturn, breturn, 0);
}

#else
INT16_T mt_evnt_multi(INT16_T events, INT16_T bclicks, INT16_T bmask, INT16_T bstate, INT16_T m1flag, GRECT *g1, INT16_T m2flag, GRECT *g2, INT16_T *msg, unsigned int ms, EVNTDATA *ev, INT16_T *kreturn, INT16_T *breturn, INT16_T* aes_global)
{
	INT16_T control[5] = {25, 16, 7, 1, 0};
	INT16_T intin[16];
	INT16_T intout[7];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = events;
	intin[1] = bclicks;
	intin[2] = bmask;
	intin[3] = bstate;
	if (events & MU_M1)
	{
		intin[4] = m1flag;
		*(GRECT*)(&intin[5]) = *g1;
	}
	
	if (events & MU_M2)
	{
		intin[9] = m2flag;
		*(GRECT*)(&intin[10]) = *g2;
	}
	*(INT32_T*)(&intin[14]) = ms;
	addrin[0] = (void*)msg;
	INT16_T result = (INT16_T)aes_call(&lcl_aespb);
	*ev = *(EVNTDATA*)(&intout[1]);
	*kreturn = intout[5];
	*breturn = intout[6];
	return result;
}

INT16_T evnt_multi(INT16_T events, INT16_T bclicks, INT16_T bmask, INT16_T bstate, INT16_T m1flag, INT16_T m1x, INT16_T m1y, INT16_T m1w, INT16_T m1h, INT16_T m2flag, INT16_T m2x, INT16_T m2y, INT16_T m2w, INT16_T m2h, INT16_T* msg, INT16_T locnt, INT16_T hicnt, INT16_T* mx, INT16_T* my, INT16_T* button, INT16_T* kstate, INT16_T* kreturn, INT16_T* breturn)
{
	INT16_T control[5] = {25, 16, 7, 1, 0};
	INT16_T intin[16];
	INT16_T intout[7];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		0,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = events;
	intin[1] = bclicks;
	intin[2] = bmask;
	intin[3] = bstate;
	intin[4] = m1flag;
	intin[5] = m1x;
	intin[6] = m1y;
	intin[7] = m1w;
	intin[8] = m1h;
	intin[9] = m2flag;
	intin[10] = m2x;
	intin[11] = m2y;
	intin[12] = m2w;
	intin[13] = m2h;
	intin[14] = locnt;
	intin[15] = hicnt;
	addrin[0] = (void*)msg;
	INT16_T result = (INT16_T)aes_call(&lcl_aespb);
	*mx = intout[1];
	*my = intout[2];
	*button = intout[3];
	*kstate = intout[4];
	*kreturn = intout[5];
	*breturn = intout[6];
	return result;
}
#endif // TARGET_M68K_ATARI_MINTELF

void MT_EVNT_multi(INT16_T events, INT16_T bclicks, INT16_T bmask, INT16_T bstate, MOBLK *m1, MOBLK *m2, INT16_T *msg, INT32_T ms, EVNT *ev, INT16_T* aes_global)
{
	INT16_T control[5] = {25, 16, 7, 1, 0};
	INT16_T intin[16];
	INT16_T intout[7];
	void* addrin[1];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		aes_unused_dummy_addr
	};
	intin[0] = events;
	intin[1] = bclicks;
	intin[2] = bmask;
	intin[3] = bstate;
	if (events & MU_M1)
	{
		*((MOBLK*)(&intin[4])) = *m1;
	}
	
	if (events & MU_M2)
	{
		*((MOBLK*)(&intin[9])) = *m2;
	}
	*(INT32_T*)(&intin[14]) = ms;
	addrin[0] = (void*)msg;
	aes_call(&lcl_aespb);
	*ev = *(EVNT*)(&intout[1]);
}

