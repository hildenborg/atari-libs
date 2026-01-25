/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include <stdio.h>
#include <tos.h>
#include <aes.h>
#include <vdi.h>
#include "test.h"

int main (int argc, char **argv)
{
	short app_id = appl_init();
	if (app_id < 0)
	{
		return -1;
	}

	FILE* fp = fopen("testrun.txt", "w");

	short wchar, hchar, wbox, hbox;
	glbl_handle = graf_handle(&wchar, &hchar, &wbox, &hbox);

	short work[11];
	WS ws;
	for (short i = 1; i < 10; i++)
	{
		work[i] = 1;
	}
	work[0] = Getrez() + 2;
	work[10] = 2;
	v_opnvwk(work, &glbl_handle, &ws);

	test_all_calls(fp);

	appl_exit();
	fclose(fp);
	return 0;
}

