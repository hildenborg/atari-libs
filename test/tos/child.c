/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Child program for the Pexec/Pterm tests. The first character of the command line
	selects what to do, the result is reported back to the parent as the exit code.
*/

#include <string.h>
#include <tos.h>

long _stksize = 8192;
extern BASEPAGE* _BasePage;

int main(void)
{
	BASEPAGE* bp = _BasePage;
	int len = (unsigned char)bp->p_cmdlin[0];
	char cmd = len ? bp->p_cmdlin[1] : 0;
	switch (cmd)
	{
		case 'T':	// "T <n>": Pterm(n)
		{
			int n = 0;
			for (int i = 3; i <= len; i++)
			{
				n = n * 10 + (bp->p_cmdlin[i] - '0');
			}
			Pterm(n);
		}
		case 'Z':	// Pterm0
			Pterm0();
		case 'R':	// Ptermres, keep the basepage only.
			Ptermres(256, 77);
		case 'A':	// Return the command line length.
			Pterm(len);
		case 'E':	// Look for FOO=bar in the environment.
		{
			const char* e = bp->p_env;
			while (e && *e)
			{
				if (strcmp(e, "FOO=bar") == 0)
				{
					Pterm(1);
				}
				e += strlen(e) + 1;
			}
			Pterm(2);
		}
		default:
			break;
	}
	Pterm(99);
}
