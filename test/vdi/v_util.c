/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

#include "v_util.h"

short v_handle;
short v_work_out[57];
short v_ext_out[57];
long v_nvdi;
long v_gdos;
short v_scr_w, v_scr_h, v_scr_planes, v_scr_bpl;
short v_ox, v_oy;

V_REC v_rec[V_MAXREC];
volatile short v_nrec;
short v_fake_intout[V_MAXIN];
short v_fake_ptsout[V_MAXPTS];
short v_fake_contrl[16];
unsigned short v_fake_cmask;

volatile short v_mode;		// 0 off, 1 spy, 2 fake
static short fake_nint, fake_npts;
long v_old_trap2;

/*
	The trap #2 handler. VDI calls (d0 = 0x73) are handed to v_hook while recording,
	everything else (AES, GDOS queries) goes straight to the old handler.
	In spy mode the old handler is called with an exception frame that returns here,
	so v_post can record what the VDI returned before going back to the caller.
	The frame has a format word when the CPU uses long frames (_longframe at 0x59e).
*/
__asm__(
	".globl v_trap2\n"
	"v_trap2:\n\t"
	"cmp.w	#0x73, %d0\n\t"
	"bne.s	9f\n\t"
	"tst.w	v_mode\n\t"
	"beq.s	9f\n\t"
	"movem.l	%d0-%d2/%a0-%a2, %sp@-\n\t"
	"move.l	%d1, %sp@-\n\t"
	"jsr	v_hook\n\t"
	"addq.l	#4, %sp\n\t"
	"tst.l	%d0\n\t"
	"movem.l	%sp@+, %d0-%d2/%a0-%a2\n\t"		// movem doesn't change the flags
	"bne.s	8f\n\t"
	"tst.w	0x59e\n\t"
	"beq.s	1f\n\t"
	"move.w	#0x88, %sp@-\n"
	"1:\n\t"
	"pea	2f\n\t"
	"move.w	%sr, %sp@-\n\t"
	"move.l	v_old_trap2, %sp@-\n\t"
	"rts\n"
	"2:\n\t"
	"movem.l	%d0-%d2/%a0-%a2, %sp@-\n\t"
	"jsr	v_post\n\t"
	"movem.l	%sp@+, %d0-%d2/%a0-%a2\n"
	"8:\n\t"
	"rte\n"
	"9:\n\t"
	"move.l	v_old_trap2, %sp@-\n\t"
	"rts\n");
void v_trap2(void);

static VDIPB* cur_pb;
static short cur_n;
static short* real_intout;
static short* real_ptsout;
static short lim_int = -1, lim_pts = -1;
#define SPY_BUF		1024
#define SENTINEL	0x7e7e
static short spy_intout[SPY_BUF], spy_ptsout[SPY_BUF];

void v_post(void)
{
	VDIPB* pb = cur_pb;
	int wi = 0, wp = 0;
	for (int i = 0; i < SPY_BUF; i++)
	{
		if (spy_intout[i] != SENTINEL) wi = i + 1;
		if (spy_ptsout[i] != SENTINEL) wp = i + 1;
	}
	if (wi < pb->contrl[4]) wi = pb->contrl[4];
	if (wp < pb->contrl[2] * 2) wp = pb->contrl[2] * 2;
	if (wi > SPY_BUF) wi = SPY_BUF;
	if (wp > SPY_BUF) wp = SPY_BUF;
	int ci = (lim_int >= 0 && wi > lim_int) ? lim_int : wi;
	int cp = (lim_pts >= 0 && wp > lim_pts) ? lim_pts : wp;
	for (int i = 0; i < ci; i++) real_intout[i] = spy_intout[i];
	for (int i = 0; i < cp; i++) real_ptsout[i] = spy_ptsout[i];
	pb->intout = real_intout;
	pb->ptsout = real_ptsout;
	if (cur_n < V_MAXREC)
	{
		V_REC* r = &v_rec[cur_n];
		r->nptsout = (short)((wp + 1) / 2);
		r->nintout = (short)wi;
		for (int i = 0; i < 16; i++)
		{
			r->contrl_out[i] = pb->contrl[i];
		}
		for (int i = 0; i < wi && i < V_MAXOUT; i++)
		{
			r->intout_val[i] = spy_intout[i];
		}
		for (int i = 0; i < wp && i < V_MAXOUT; i++)
		{
			r->ptsout_val[i] = spy_ptsout[i];
		}
	}
}

// Returns non zero when the call is done (fake mode).
int v_hook(VDIPB* pb)
{
	short n = v_nrec++;
	cur_pb = pb;
	cur_n = n;
	if (n < V_MAXREC)
	{
		V_REC* r = &v_rec[n];
		for (int i = 0; i < 16; i++)
		{
			r->contrl[i] = pb->contrl[i];
		}
		short ni = pb->contrl[3], np = pb->contrl[1] * 2;
		for (int i = 0; i < ni && i < V_MAXIN; i++)
		{
			r->intin[i] = pb->intin[i];
		}
		for (int i = 0; i < np && i < V_MAXPTS; i++)
		{
			r->ptsin[i] = pb->ptsin[i];
		}
		r->intout = pb->intout;
		r->ptsout = pb->ptsout;
	}
	if (v_mode != 2)
	{
		// Spy: let the VDI write into the hook's buffers.
		for (int i = 0; i < SPY_BUF; i++)
		{
			spy_intout[i] = SENTINEL;
			spy_ptsout[i] = SENTINEL;
		}
		real_intout = pb->intout;
		real_ptsout = pb->ptsout;
		pb->intout = spy_intout;
		pb->ptsout = spy_ptsout;
		return 0;
	}
	for (int i = 0; i < fake_nint; i++)
	{
		pb->intout[i] = v_fake_intout[i];
	}
	for (int i = 0; i < fake_npts; i++)
	{
		pb->ptsout[i] = v_fake_ptsout[i];
	}
	pb->contrl[2] = fake_npts / 2;
	pb->contrl[4] = fake_nint;
	for (int i = 0; i < 16; i++)
	{
		if (v_fake_cmask & (1 << i))
		{
			pb->contrl[i] = v_fake_contrl[i];
		}
	}
	return 1;
}

static long install_fn(void)
{
	long* vec = (long*)0x88;
	if (*vec != (long)v_trap2)
	{
		v_old_trap2 = *vec;
		*vec = (long)v_trap2;
	}
	return 0;
}

static long remove_fn(void)
{
	long* vec = (long*)0x88;
	if (*vec == (long)v_trap2)
	{
		*vec = v_old_trap2;
	}
	return 0;
}

static void arm(short mode)
{
	v_nrec = 0;
	for (int i = 0; i < V_MAXREC; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			v_rec[i].contrl[j] = -1;
		}
		v_rec[i].nintout = -1;
		v_rec[i].nptsout = -1;
	}
	t_super(install_fn);
	v_mode = mode;
}

void v_spy(void)
{
	lim_int = lim_pts = -1;
	arm(1);
}

void v_spy_lim(short max_int, short max_pts)
{
	lim_int = max_int;
	lim_pts = max_pts;
	arm(1);
}

void v_fake(short nint, short npts)
{
	fake_nint = nint;
	fake_npts = npts;
	for (int i = 0; i < V_MAXIN; i++)
	{
		v_fake_intout[i] = 0x100 + i;
	}
	for (int i = 0; i < V_MAXPTS; i++)
	{
		v_fake_ptsout[i] = 0x200 + i;
	}
	v_fake_cmask = 0;
	arm(2);
}

void v_off(void)
{
	v_mode = 0;
	t_super(remove_fn);
}

int v_in(const char* fn, int n, short op, short sub, short npts, short nint, short handle, const short* intin, const short* ptsin)
{
	if (n >= v_nrec || n >= V_MAXREC)
	{
		return t_check(0, fn, "VDI call %d not recorded (%d calls)", n, v_nrec);
	}
	V_REC* r = &v_rec[n];
	if (r->contrl[0] != op)
	{
		return t_check(0, fn, "contrl[0] (opcode) = %d, expected %d", r->contrl[0], op);
	}
	if (sub >= 0 && r->contrl[5] != sub)
	{
		return t_check(0, fn, "contrl[5] (subopcode) = %d, expected %d", r->contrl[5], sub);
	}
	if (r->contrl[1] != npts)
	{
		return t_check(0, fn, "contrl[1] (ptsin points) = %d, expected %d", r->contrl[1], npts);
	}
	if (r->contrl[3] != nint)
	{
		return t_check(0, fn, "contrl[3] (intin words) = %d, expected %d", r->contrl[3], nint);
	}
	if (handle != FH_ANY && r->contrl[6] != handle)
	{
		return t_check(0, fn, "contrl[6] (handle) = 0x%x, expected 0x%x", r->contrl[6], handle);
	}
	if (intin)
	{
		for (int i = 0; i < nint && i < V_MAXIN; i++)
		{
			if (r->intin[i] != intin[i])
			{
				return t_check(0, fn, "intin[%d] = %d (0x%x), expected %d (0x%x)", i, r->intin[i], (unsigned short)r->intin[i],
					intin[i], (unsigned short)intin[i]);
			}
		}
	}
	if (ptsin)
	{
		for (int i = 0; i < npts * 2 && i < V_MAXPTS; i++)
		{
			if (r->ptsin[i] != ptsin[i])
			{
				return t_check(0, fn, "ptsin[%d] = %d, expected %d", i, r->ptsin[i], ptsin[i]);
			}
		}
	}
	return t_check(1, fn, "op %d/%d, %d points, %d intin", op, r->contrl[5], npts, nint);
}

int v_out_fits(const char* fn, int n, short max_int, short max_pts)
{
	if (n >= v_nrec || n >= V_MAXREC || v_rec[n].nintout < 0)
	{
		return t_check(0, fn, "VDI call %d output not recorded (%d calls)", n, v_nrec);
	}
	V_REC* r = &v_rec[n];
	return t_check(r->nintout <= max_int && r->nptsout * 2 <= max_pts, fn,
		"VDI returned %d intout and %d ptsout words, the binding has room for %d and %d",
		r->nintout, r->nptsout * 2, max_int, max_pts);
}

int v_maps(const char* fn, int n, const char* what, const short* got, const short* idx, int cnt, int from_intout)
{
	if (n >= v_nrec || n >= V_MAXREC || v_rec[n].nintout < 0)
	{
		return t_check(0, fn, "VDI call %d output not recorded (%d calls)", n, v_nrec);
	}
	V_REC* r = &v_rec[n];
	const short* src = from_intout ? r->intout_val : r->ptsout_val;
	int avail = from_intout ? r->nintout : r->nptsout * 2;
	int checked = 0;
	for (int i = 0; i < cnt; i++)
	{
		if (idx[i] >= avail)
		{
			continue;
		}
		checked++;
		if (got[i] != src[idx[i]])
		{
			return t_check(0, fn, "%s[%d] = %d, the VDI returned %s[%d] = %d", what, i, got[i], from_intout ? "intout" : "ptsout",
				idx[i], src[idx[i]]);
		}
	}
	return t_check(1, fn, "%s: %d of %d words returned by the VDI match", what, checked, cnt);
}

void v_raw(V_RAW* r)
{
	VDIPB pb = { r->contrl, r->intin, r->ptsin, r->intout, r->ptsout };
	r->contrl[2] = 0;
	r->contrl[4] = 0;
	__asm__ volatile (
		"move.l	%0, %%d1\n\t"
		"moveq	#0x73, %%d0\n\t"
		"trap	#2\n\t"
		:
		: "g" (&pb)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
}

int v_words(const char* fn, const char* what, const short* got, const short* want, int n)
{
	for (int i = 0; i < n; i++)
	{
		if (got[i] != want[i])
		{
			return t_check(0, fn, "%s[%d] = %d (0x%x), expected %d (0x%x)", what, i, got[i], (unsigned short)got[i],
				want[i], (unsigned short)want[i]);
		}
	}
	return t_check(1, fn, "%s, %d words", what, n);
}

/*
	Screen.
*/
static long* linea;
static void linea_init(void)
{
	register long* a0 asm ("a0");
	__asm__ volatile (
		".word 0xa000\n\t"
		: "=r" (a0)
		:
		: "d0", "d1", "d2", "a1", "a2", "cc", "memory"
	);
	linea = a0;
}

short v_linea(int offset)
{
	return *(short*)((char*)linea + offset);
}

static void* logbase(void)
{
	register void* result asm ("d0");
	__asm__ volatile (
		"move.w	#3, %%sp@-\n\t"
		"trap	#14\n\t"
		"addq.l	#2, %%sp\n\t"
		: "=r" (result)
		:
		: "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
	return result;
}

unsigned short v_pixel(short x, short y)
{
	unsigned short* base = (unsigned short*)logbase() + (long)y * (v_scr_bpl / 2);
	if (v_scr_planes >= 16)
	{
		return base[x];
	}
	unsigned short* w = base + (x >> 4) * v_scr_planes;
	unsigned short bit = 0x8000 >> (x & 15);
	unsigned short v = 0;
	for (int p = 0; p < v_scr_planes; p++)
	{
		if (w[p] & bit)
		{
			v |= 1 << p;
		}
	}
	return v;
}

static unsigned short white;

unsigned short v_clear(short x, short y, short w, short h)
{
	short xy[4] = { x, y, (short)(x + w - 1), (short)(y + h - 1) };
	short attr[5];
	vqf_attributes(v_handle, attr);
	short mode = vswr_mode(v_handle, MD_REPLACE);
	vsf_interior(v_handle, FIS_SOLID);
	vsf_color(v_handle, 0);
	vr_recfl(v_handle, xy);
	vsf_interior(v_handle, attr[0]);
	vsf_color(v_handle, attr[1]);
	vswr_mode(v_handle, mode);
	white = v_pixel(x, y);
	return white;
}

int v_ink(short x, short y)
{
	return v_pixel(x, y) != white;
}

int v_count_ink(short x, short y, short w, short h)
{
	int n = 0;
	for (short j = y; j < y + h; j++)
	{
		for (short i = x; i < x + w; i++)
		{
			n += v_ink(i, j);
		}
	}
	return n;
}

void v_defaults(short h)
{
	vswr_mode(h, MD_REPLACE);
	vsl_type(h, SOLID);
	vsl_width(h, 1);
	vsl_color(h, 1);
	vsl_ends(h, SQUARE, SQUARE);
	vsm_type(h, 1);
	vsm_color(h, 1);
	vsf_interior(h, FIS_SOLID);
	vsf_style(h, 1);
	vsf_color(h, 1);
	vsf_perimeter(h, 1);
	vst_color(h, 1);
	vst_effects(h, 0);
	vst_rotation(h, 0);
	short d;
	vst_alignment(h, 0, 0, &d, &d);
	vs_clip(h, 0, 0);
}

void v_scr_begin(void)
{
	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, 0);
	v_defaults(v_handle);
}

void v_scr_end(void)
{
	v_defaults(v_handle);
	graf_mouse(M_ON, 0);
	wind_update(END_UPDATE);
	form_dial(FMD_FINISH, 0, 0, 0, 0, a_desk.g_x, a_desk.g_y, a_desk.g_w, a_desk.g_h);
	a_wait(100);
}

int v_begin(const char* prog)
{
	if (!a_begin(prog))
	{
		return 0;
	}
	linea_init();
	v_scr_planes = ((short*)linea)[0];
	v_scr_bpl = ((short*)linea)[-1];
	v_scr_w = ((short*)linea)[-6];
	v_scr_h = ((short*)linea)[-2];
	if (!t_cookie(0x4E564449L, &v_nvdi))	// 'NVDI'
	{
		v_nvdi = 0;
	}
	v_gdos = vq_vgdos();
	short work_in[11] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
	v_handle = a_vdi;
	v_opnvwk(work_in, &v_handle, (WS*)v_work_out);
	if (!t_check(v_handle > 0, "v_opnvwk", "v_opnvwk() handle = %d", v_handle))
	{
		v_handle = 0;
		return 0;
	}
	vq_extnd(v_handle, 1, v_ext_out);
	v_ox = a_desk.g_x + 16;
	v_oy = a_desk.g_y + 16;
	t_print("VDI handle %d, screen %dx%d %d planes %d bytes/line, NVDI %lx, GDOS %lx", v_handle, v_scr_w, v_scr_h,
		v_scr_planes, v_scr_bpl, v_nvdi, v_gdos);
	return 1;
}

int v_end(void)
{
	v_off();
	if (v_handle > 0)
	{
		v_clsvwk(v_handle);
	}
	return a_end();
}
