#include "aes_def.h"

XFSL_DIALOG* mt_fslx_do(const INT8_T* title, INT8_T* path, INT16_T pathlen, INT8_T* fname, INT16_T fnamelen, const INT8_T* patterns, XFSL_FILTER filter, INT8_T* paths, INT16_T* sort_mode, INT16_T flags, INT16_T* button, INT16_T* nfiles, INT8_T** pattern, INT16_T* aes_global)
{
	INT16_T control[5] = {194, 4, 2, 6, 2};
	INT16_T intin[4];
	INT16_T intout[3];
	void* addrin[6];
	void* addrout[2];
	AESPB lcl_aespb =
	{
		control,
		aes_global,
		intin,
		intout,
		addrin,
		addrout
	};
	intin[0] = pathlen;
	intin[1] = fnamelen;
	intin[2] = *sort_mode;
	intin[3] = flags;
	addrin[0] = (void*)title;
	addrin[1] = (void*)path;
	addrin[2] = (void*)fname;
	addrin[3] = (void*)patterns;
	addrin[4] = (void*)filter;
	addrin[5] = (void*)paths;
	aes_call(&lcl_aespb);
	*button = intout[0];
	*nfiles = intout[1];
	*sort_mode = intout[2];
	*pattern = addrout[1];
	return (XFSL_DIALOG*)addrout[0];
}

XFSL_DIALOG* fslx_do(const INT8_T* title, INT8_T* path, INT16_T pathlen, INT8_T* fname, INT16_T fnamelen, const INT8_T* patterns, XFSL_FILTER filter, INT8_T* paths, INT16_T* sort_mode, INT16_T flags, INT16_T* button, INT16_T* nfiles, INT8_T** pattern)
{
	return mt_fslx_do(title, path, pathlen, fname, fnamelen, patterns, filter, paths, sort_mode, flags, button, nfiles, pattern, 0);
}

