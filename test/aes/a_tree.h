/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	The dialog tree used by a_objc.c and a_graf.c, built in memory.
*/

#ifndef A_TREE_DEFINED
#define A_TREE_DEFINED

#include <string.h>
#include "a_util.h"

enum { D_ROOT, D_TITLE, D_EDIT, D_OK, D_CANCEL, D_CHAR, D_COUNT };

static OBJECT tree[D_COUNT];
static char text[10];
static TEDINFO ted = { text, "Name: ________", "XXXXXXXX", IBM, 0, TE_LEFT, 0x1180, 0, 0, 9, 15 };

static void build_tree(void)
{
	memset(text, 0, sizeof(text));
	a_obj(tree, D_ROOT, -1, D_TITLE, D_CHAR, G_BOX, NONE, OUTLINED, (void*)0x00021100L, 0, 0, 30, 8);
	a_obj(tree, D_TITLE, D_EDIT, -1, -1, G_STRING, NONE, NORMAL, "Test dialog", 2, 1, 11, 1);
	a_obj(tree, D_EDIT, D_OK, -1, -1, G_FTEXT, EDITABLE, NORMAL, &ted, 2, 3, 14, 1);
	a_obj(tree, D_OK, D_CANCEL, -1, -1, G_BUTTON, SELECTABLE | DEFAULT | EXIT, NORMAL, "OK", 4, 5, 8, 1);
	a_obj(tree, D_CANCEL, D_CHAR, -1, -1, G_BUTTON, SELECTABLE | EXIT, NORMAL, "Cancel", 16, 5, 8, 1);
	a_obj(tree, D_CHAR, D_ROOT, -1, -1, G_BOXCHAR, LASTOB, NORMAL, (void*)(('X' << 24) | 0x00011100L), 26, 1, 2, 1);
	a_fix_tree(tree, D_COUNT);
}

__attribute__((unused)) static void center_of(short obj, short* x, short* y)
{
	objc_offset(tree, obj, x, y);
	*x += tree[obj].ob_width / 2;
	*y += tree[obj].ob_height / 2;
}

#endif
