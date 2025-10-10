#include "line_a.h"

/*
	The line_a api is really cryptic for a C user.
	And as such it is not easy to implement either.
	There is a lot of values in Linea that are expected to be set manually before
	calling a function, but the documentations disagree.
	The code for the bindings also differ between functions.
	As even Atari gave up support for line_a, so do I.
	I will leave all code as is, but disable generation in gen.py and remove compilation from makefile.
	If anyone else feels like giving it a go, then feel free to do so.
	/Mikael Hildenborg.
*/

LINEA *Linea;
VDIESC *Vdiesc;
FONT_HDR **Fonts;
LINEA_FUNP *Linea_funp;

void linea_init(void)
{
	__asm__ volatile (
		".dc.w	0xa000\n\t"
		"move.l	%%a0, %0@\n\t"
		"lea	%%a0@(-910), %%a0\n\t"
		"move.l	%%a0, %1@\n\t"
		"move.l	%%a1, %2@\n\t"
		"move.l	%%a2, %3@\n\t"
		:
		: "g" (&Linea), "g" (&Vdiesc), "g" (&Fonts), "g" (&Linea_funp)
		: "d0", "d1", "d2", "a0", "a1", "a2"
	);
}

void put_pixel(INT16_T x, INT16_T y, INT16_T color)
{

}

INT16_T get_pixel(INT16_T x, INT16_T y)
{

	return 0;
}

void draw_line(INT16_T x1, INT16_T y1, INT16_T x2, INT16_T y2)
{

}

void horizontal_line(INT16_T x1, INT16_T y1, INT16_T x2)
{

}

void filled_rect(INT16_T x1, INT16_T y1, INT16_T x2, INT16_T y2)
{

}

void filled_polygon(INT16_T *xy, INT16_T count)
{

}

void bit_blt(BITBLT *bitblt)
{

}

void text_blt(INT16_T x, INT16_T y, UINT8_T c)
{

}

void show_mouse(INT16_T flag)
{

}

void hide_mouse(void)
{

}

void transform_mouse(MFORM *mform)
{

}

void undraw_sprite(SSB *ssb)
{

}

void draw_sprite(INT16_T x, INT16_T y, SDB *sdb, SSB *ssb)
{

}

void copy_raster(void)
{

}

void seed_fill(void)
{

}

void set_fg_bp(INT16_T auswahl)
{

}

void set_ln_mask(INT16_T mask)
{

}

void set_wrt_mode(INT16_T modus)
{

}

void set_pattern(INT16_T *pattern, INT16_T mask, INT16_T multifill)
{

}

void set_clip(INT16_T x1, INT16_T y1, INT16_T x2, INT16_T y2, INT16_T modus)
{

}

void set_text_blt(FONT_HDR *font, INT16_T scale, INT16_T style, INT16_T chup, INT16_T text_fg, INT16_T text_bg)
{

}

void draw_circle(INT16_T x, INT16_T y, INT16_T radius, INT16_T color)
{

}

void print_string(INT16_T x, INT16_T y, INT16_T xoff, INT8_T *string)
{

}

