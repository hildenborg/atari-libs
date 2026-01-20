#include "aes_def.h"

void* mt_fslx_do(char* title, char* path, short pathlen, char* fname, short fnamelen, char* patterns, XFSL_FILTER* filter, char* paths, short* sort_mode, short flags, short* button, short* nfiles, char** pattern, short* aes_global)
{
	short control[5] = {194, 4, 2, 6, 2};
	short intin[4];
	short intout[3];
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
	return addrout[0];
}

void* fslx_do(char* title, char* path, short pathlen, char* fname, short fnamelen, char* patterns, XFSL_FILTER* filter, char* paths, short* sort_mode, short flags, short* button, short* nfiles, char** pattern)
{
	return mt_fslx_do(title, path, pathlen, fname, fnamelen, patterns, filter, paths, sort_mode, flags, button, nfiles, pattern, 0);
}

