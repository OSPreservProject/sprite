/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: tfmfont.c,v 1.1 88/02/11 17:09:00 jim Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "types.h"
#include "conv.h"
#include "font.h"
#include "tfm.h"

/*
 * TFM font operations.  This defines three fonts:
 *
 *	box   - prints as little square boxes, outlining what TeX
 *		thinks is the character.
 *	blank - prints as entirely blank.
 *	invis - prints as entirely blank.
 *
 * The first two also complain that no font is available in the
 * requested size; these are intended to be used as a last resort
 * so that users can always print DVI files.  You should configure
 * in exactly one of box or blank.
 *
 * TODO:
 *	base box edge widths on Conversion.c_dpi
 */
int	box_read(), blank_read(), invis_read();
int	tfm_getgly(), tfm_rasterise(), tfm_freefont();

	/* magnifications are unused in tfm fonts */
struct	fontops boxops =	/* `boxtops'?  Is this a cereal driver? */
	{ "box", 0.0, box_read, tfm_getgly, tfm_rasterise, tfm_freefont };
struct	fontops blankops =
	{ "blank", 0.0, blank_read, tfm_getgly, tfm_rasterise, tfm_freefont };
struct	fontops invisops =
	{ "invis", 0.0, invis_read, tfm_getgly, tfm_rasterise, tfm_freefont };

/*
 * Local info.
 */
struct tfm_details {
	int	tfm_edge;		/* box edge widths, in pixels */
	struct	conversion tfm_conv;	/* conv. from scaled pts to pixels */
	struct	tfmdata tfm_data;	/* the TFM file data */
};

/*
 * Get the tfm_details from font f.
 */
#define	ftotfm(f) ((struct tfm_details *) (f)->f_details)

extern	int errno;
char	*malloc();

/*
 * Read a Box font.
 */
static int
box_read(f)
	struct font *f;
{

	return (do_read(f, 0, 1));
}

/*
 * Read a Blank font.
 */
static int
blank_read(f)
	struct font *f;
{

	return (do_read(f, 1, 1));
}

/*
 * Read an Invisible font.
 */
static int
invis_read(f)
	struct font *f;
{

	return (do_read(f, 1, 0));
}

/*
 * Read a TFM font.  It is blank if `blank'; complain if `complain'.
 */
static int
do_read(f, blank, complain)
	register struct font *f;
	int blank, complain;
{
	register struct tfm_details *tfm;
	FILE *fd;

	if ((fd = fopen(f->f_path, "r")) == 0)
		return (-1);
	if ((tfm = (struct tfm_details *) malloc(sizeof (*tfm))) == NULL)
		goto fail;
	if (readtfmfile(fd, &tfm->tfm_data, blank))
		goto fail;
	if (blank)
		tfm->tfm_edge = 0;
	else {
		tfm->tfm_edge = 2;	/* XXX should be based on dpi */
		tfm->tfm_conv = Conversion;
		tfm->tfm_conv.c_fromsp *= DMagFactor(f->f_scaled);
					/* XXX !data abstraction */
	}
	if (FontHasGlyphs(f, tfm->tfm_data.t_hdr.th_bc,
			  tfm->tfm_data.t_hdr.th_ec + 1))
		goto fail;
	f->f_details = (char *) tfm;
	(void) fclose(fd);
	if (complain)
		error(0, 0, "Warning: no font for %s", Font_TeXName(f));
	return (0);

fail:
	(void) fclose(fd);
	if (tfm != NULL)
		free((char *) tfm);
	return (-1);
}

/*
 * Obtain the specified range of glyphs.
 */
static int
tfm_getgly(f, l, h)
	register struct font *f;
	int l;
	register int h;
{
	register struct tfm_details *tfm = ftotfm(f);
	register struct glyph *g;
	register int i;
	register struct char_info_word *ci;
#define	t (&tfm->tfm_data)
	i32 ScaleOneWidth();
#define ftop(fix) cfromSP(&tfm->tfm_conv, ScaleOneWidth(fix, f->f_dvimag))

	for (i = l; i < h; i++) {
		ci = &t->t_ci[i - t->t_hdr.th_bc];
		/* zero widths mark invalid characters */
		if (ci->ci_width == 0)
			continue;
		g = f->f_gly[i];
		g->g_flags = GF_VALID;
		g->g_tfmwidth = t->t_width[UnSign8(ci->ci_width)];
		if (tfm->tfm_edge != 0) {
			g->g_xorigin = 0;
			g->g_yorigin = ftop(t->t_height[T_CI_H(ci)]);
			g->g_width = ftop(g->g_tfmwidth);
			g->g_height = g->g_yorigin +
				ftop(t->t_depth[T_CI_D(ci)]);
		}
	}
	return (0);
#undef t
}

/*
 * Obtain rasters for the specified glyphs.
 *
 * IGNORES tfm->tfm_edge: 2 HARDCODED FOR NOW
 */
static int
tfm_rasterise(f, l, h)
	struct font *f;
	int l, h;
{
	register struct glyph *g;
	register char *p;
	register int w, j, i;
	struct tfm_details *tfm = ftotfm(f);
#define EDGE 2

	if (tfm->tfm_edge == 0)
		return;
if (tfm->tfm_edge != 2) panic("tfm_rasterise");
	for (i = l; i < h; i++) {
		g = f->f_gly[i];
		if ((g->g_flags & GF_VALID) == 0 || !HASRASTER(g))
			continue;
		w = (g->g_width + 7) >> 3;
		p = malloc((unsigned) (g->g_height * w));
		if (p == NULL)
			return (-1);
		g->g_raster = p;
		g->g_rotation = ROT_NORM;
		if (g->g_width < 2 * EDGE) {
			w = 2 * EDGE - g->g_width;
			for (j = g->g_height; --j >= 0;)
				*p++ = 0xf0 << w;
		} else {
			bzero(p, g->g_height * w);
			for (j = 0; j < g->g_height;) {
				if (j < EDGE || j >= g->g_height - EDGE) {
					register int k = w;

					while (--k > 0)
						*p++ = 0xff;
					*p++ = 0xff << ((8 - g->g_width) & 7);
					j++;
					continue;
				}
				/* the following depends on EDGE==2 */
				*p = 0xc0;
				p += w - ((g->g_width & 7) == 1 ? 2 : 1);
				*p++ |= 0xc0 >> ((g->g_width - EDGE) & 7);
				if ((g->g_width & 7) == 1)
					*p++ = 0x80;
				/* end dependencies */
				if (++j == EDGE && g->g_height >= 2 * EDGE) {
					register int n = g->g_height - EDGE;

					p += (n - j) * w;
					j = n;
				}
			}
		}
	}
	return (0);
}

/*
 * Discard the font details.
 */
static
tfm_freefont(f)
	struct font *f;
{

	free(f->f_details);
}
