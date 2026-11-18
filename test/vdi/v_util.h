/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	Helpers for the VDI tests. The programs run under GEM like the AES tests (and use their helpers).

	Two kinds of tests:
	- Binding tests: a trap #2 hook records the VDI parameter block of each call, so the contrl, intin and
	  ptsin arrays a binding builds can be compared with the VDI specification. In fake mode the call doesn't
	  reach the VDI, the hook fills intout/ptsout/contrl with known values instead, so the output side of a
	  binding can be checked too, also for functions no driver here implements (printers, cameras, ...).
	- Real tests on a virtual screen workstation, results checked by reading the screen memory directly.
*/

#ifndef V_UTIL_DEFINED
#define V_UTIL_DEFINED

#include <vdi.h>
#include "a_util.h"

// Handle used in fake mode, it never reaches the VDI.
#define FH 0x1234

extern short v_handle;			// Virtual screen workstation (0 when it couldn't be opened)
extern short v_work_out[57];	// v_opnvwk work_out
extern short v_ext_out[57];		// vq_extnd(1) work_out
extern long v_nvdi;				// NVDI cookie value (0 when missing)
extern long v_gdos;				// vq_vgdos result

// Screen geometry from Line-A (independent of the library).
extern short v_scr_w, v_scr_h, v_scr_planes, v_scr_bpl;
// A word from the Line-A variables (offset in bytes, negative for the variables before the base).
short v_linea(int offset);

int v_begin(const char* prog);
int v_end(void);

/*
	Trap #2 recorder.
*/
#define V_MAXREC	4
#define V_MAXIN		300
#define V_MAXPTS	256
#define V_MAXOUT	300
typedef struct
{
	short contrl[16];
	short intin[V_MAXIN];
	short ptsin[V_MAXPTS];
	short* intout;			// Array pointers as passed to the VDI
	short* ptsout;
	// Spy mode only, what the VDI returned:
	short nintout;			// Words written to intout (the larger of contrl[4] and the words actually changed)
	short nptsout;			// Points written to ptsout (the same)
	short contrl_out[16];
	short intout_val[V_MAXOUT];
	short ptsout_val[V_MAXOUT];
} V_REC;
extern V_REC v_rec[V_MAXREC];
extern volatile short v_nrec;		// Number of recorded calls (may be more than V_MAXREC)

// Values returned in fake mode: intout[i] = 0x100 + i, ptsout[i] = 0x200 + i unless changed after v_fake().
extern short v_fake_intout[V_MAXIN];
extern short v_fake_ptsout[V_MAXPTS];
extern short v_fake_contrl[16];			// contrl words set when their bit is in v_fake_cmask
extern unsigned short v_fake_cmask;

// Record calls and pass them on to the VDI. The VDI writes into buffers of the hook, which counts the words
// actually written (some VDIs write more than contrl[2]/[4] say) and copies them to the binding's arrays.
void v_spy(void);
// The same, but at most max_int intout and max_pts ptsout words are copied back (protects the test from a binding
// whose buffer is known to be too small, v_out_fits still reports it).
void v_spy_lim(short max_int, short max_pts);
// Record calls, don't call the VDI. Returns nint intout words and npts ptsout words (contrl[4] = nint, contrl[2] = npts / 2).
void v_fake(short nint, short npts);
// Stop recording.
void v_off(void);

// Check recorded call n: opcode, subopcode (contrl[5], not checked when sub < 0), ptsin count (points),
// intin count, handle (not checked when FH_ANY) and the intin/ptsin contents (not checked when null).
// One result line for the whole call.
#define FH_ANY	(-32768)
int v_in(const char* fn, int n, short op, short sub, short npts, short nint, short handle, const short* intin, const short* ptsin);
// Same, for a call that was expected to be the only one.
#define V_IN(fn, op, sub, npts, nint, intin, ptsin) \
	(v_nrec == 1 ? v_in(fn, 0, op, sub, npts, nint, FH, intin, ptsin) : t_check(0, fn, "%d VDI calls recorded, expected 1", v_nrec))
#define W(...) ((short[]){__VA_ARGS__})

// After a spy call: check that the VDI returned at most max_int intout words and max_pts ptsout words
// (the sizes of the binding's buffers). One result line.
int v_out_fits(const char* fn, int n, short max_int, short max_pts);

// Check that the binding's outputs are the words the VDI returned (spy mode): got[i] == (from intout ? intout : ptsout)[idx[i]].
// Only words the VDI returned (contrl[2]/[4]) are compared. One result line.
int v_maps(const char* fn, int n, const char* what, const short* got, const short* idx, int cnt, int from_intout);

// A VDI call made directly, independent of the library. Fill contrl[0], [1], [3], [5], [6], intin and ptsin.
typedef struct
{
	short contrl[16];
	short intin[300];
	short ptsin[256];
	short intout[300];
	short ptsout[256];
} V_RAW;
void v_raw(V_RAW* r);

// Check n words.
int v_words(const char* fn, const char* what, const short* got, const short* want, int n);

/*
	Screen helpers. Drawing happens inside wind_update(BEG_UPDATE) with the mouse hidden, v_scr_end redraws the desktop.
*/
void v_scr_begin(void);
void v_scr_end(void);
// Pixel value (hardware colour index, or the 16 bit value in high colour) read from the screen memory.
unsigned short v_pixel(short x, short y);
// Clear a rectangle to colour 0 (white) with vr_recfl, and return the pixel value of white.
unsigned short v_clear(short x, short y, short w, short h);
// Non zero when the pixel isn't white.
int v_ink(short x, short y);
// Number of non white pixels in a rectangle.
int v_count_ink(short x, short y, short w, short h);
// Set default attributes: replace mode, black solid lines/fill/markers/text, clipping off.
void v_defaults(short handle);

// A free area of the screen for drawing tests (inside the desktop).
extern short v_ox, v_oy;

#endif
