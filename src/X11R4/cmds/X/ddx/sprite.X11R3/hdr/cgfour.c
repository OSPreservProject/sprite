#ifndef lint
static char sccsid[] = "@(#)cgfour.c 1.1 87/12/21 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Color frame buffer with overlay plane (cg4) driver
 */

#include "cgfour.h"
#include "win.h"

#include "../h/param.h"
#include "../h/time.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/vmmac.h"

#include "../sun/fbio.h"

#include "../sundev/mbvar.h"
#include "../sundev/cg4reg.h"
#include "../sundev/p4reg.h"

#include "../machine/eeprom.h"
#include "../machine/enable.h"
#include "../machine/mmu.h"
#include "../machine/pte.h"

#include "../mon/sunromvec.h"

#include "../pixrect/pixrect.h"
#include "../pixrect/pr_impl_util.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/pr_planegroups.h"
#include "../pixrect/cg4var.h"

/* system enable register */
extern u_char enablereg;

/* colormap update firsthalf, toolate error counts */
int cg4_firsthalf = 0;
int cg4_toolate = 0;

/* driver per-unit data */
struct cg4_softc {
	short		type;		/* type code (0 = A, 1 = B) */
	short		flags;		/* internal flags */
#define	CG4_UPDATE_PENDING	1	/* colormap update in progress */
#undef	CG4_OVERLAY_CMAP		/* also defined in cg4var.h (urk) */
#define	CG4_OVERLAY_CMAP	2	/* has overlay colormap */
#define	CG4_OWNER_WANTS_COLOR	4	/* auto-init should display color */
	caddr_t		fb[CG4_NFBS];	/* pointers to frame buffer sections */
	int		w, h;		/* resolution */
	int		size;		/* total size of frame buffer */
	int		bw2size;	/* total size of overlay plane */
	struct proc	*owner;		/* owner of the frame buffer */
	struct fbgattr	gattr;		/* current attributes */
	u_long		*p4reg;		/* pointer to P4 register */

	/* color map stuff */
	union cg4_cmap {
#ifdef CG4A
		struct cg4a_cmap a;
#endif CG4A
		struct cg4b_cmap b;
	} *cmap;			/* pointer to hardware colormap */
	union {				/* shadow overlay color map */
		u_char	omap_char[3][2];
	} omap_image;
#define	omap_red	omap_image.omap_char[0]
#define	omap_green	omap_image.omap_char[1]
#define	omap_blue	omap_image.omap_char[2]
#define	omap_rgb	omap_image.omap_char[0]
	u_short		cmap_index;	/* colormap update index */
	u_short		cmap_count;	/* colormap update count */
	union {				/* shadow color map */
		u_long	cmap_long[CG4_CMAP_ENTRIES * 3 / sizeof(u_long)];
		u_char	cmap_char[3][CG4_CMAP_ENTRIES];
	} cmap_image;
#define	cmap_red	cmap_image.cmap_char[0]
#define	cmap_green	cmap_image.cmap_char[1]
#define	cmap_blue	cmap_image.cmap_char[2]
#define	cmap_rgb	cmap_image.cmap_char[0]

#if NWIN > 0
	Pixrect		pr;
	struct cg4_data	cgd;
#endif NWIN > 0
} cg4_softc[NCGFOUR];

/* probe size -- enough for the P4 register + colormap/status register */
#define	CG4_PROBESIZE	(NBPG + sizeof *cg4_softc[0].cmap)

/* Mainbus device data */
int cgfourprobe(), cgfourattach(), cgfourpoll();
struct  mb_device *cgfourinfo[NCGFOUR];
struct  mb_driver cgfourdriver = {
	cgfourprobe, 0, cgfourattach, 0, 0, cgfourpoll,
	CG4_PROBESIZE, "cgfour", cgfourinfo, 0, 0, 0, 0
};

/* default structure for FBIOGTYPE ioctl */
static struct fbtype cg4typedefault =  {
/*	type           h  w  depth cms size */
	FBTYPE_SUN2BW, 0, 0, 1,    2,   0
};

/* default structure for FBIOGATTR ioctl */
static struct fbgattr cg4attrdefault =  {
/*	real_type         owner */
	FBTYPE_SUN4COLOR, 0,
/* fbtype: type             h  w  depth cms  size */
	{ FBTYPE_SUN4COLOR, 0, 0, 8,    256,  0 }, 
/* fbsattr: flags           emu_type       dev_specific */
	{ FB_ATTR_AUTOINIT, FBTYPE_SUN2BW, { 0 } },
/*        emu_types */
	{ FBTYPE_SUN4COLOR, FBTYPE_SUN2BW, -1, -1}
};

/* frame buffer description table */
static struct cg4_fbdesc {
	short	depth;			/* depth, bits */
	short	group;			/* plane group */
	int	allplanes;		/* initial plane mask */
} cg4_fbdesc[CG4_NFBS + 1] = {
	{ 1, PIXPG_OVERLAY,		  0 },
	{ 1, PIXPG_OVERLAY_ENABLE,	  0 },
	{ 8, PIXPG_8BIT_COLOR,		255 }
};
#define	CG4_FBINDEX_OVERLAY	0
#define	CG4_FBINDEX_ENABLE	1
#define	CG4_FBINDEX_COLOR	2

/* initial active frame buffer */
#ifndef	CG4_INITFB
#define	CG4_INITFB	CG4_FBINDEX_COLOR
#endif

/* macros */
typedef IF68000(u_long, u_char) CMAP_T;

#define ITEMSIN(array)  (sizeof (array) / sizeof (array)[0])
#define	ENDOF(array)	((array) + ITEMSIN(array))

/* enable/disable interrupt */
#define	cg4_int_enable(softc)	_STMT( \
	if ((softc)->p4reg) \
		*(softc)->p4reg |= P4_REG_INTEN | P4_REG_INTCLR; \
	setintrenable(1);)

#define	cg4_int_disable(softc)	_STMT( \
	setintrenable(0); \
	if ((softc)->p4reg) \
		*(softc)->p4reg &= ~P4_REG_INTEN;)

/* check if color map update is pending */
#define	cgfour_update_pending(softc)	((softc)->flags & CG4_UPDATE_PENDING)

/* 
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 * Zero count argument indicates overlay color map update desired
 * (may cause bus error if hardware doesn't have overlay color map).
 */
#ifdef lint
#define	cgfour_update_cmap(softc, index, count)	\
	(softc)->cmap_index = (index) + (count);
#else lint
#define	cgfour_update_cmap(softc, index, count)	\
	if (count) \
		if ((softc)->cmap_count) { \
			if ((index) + (count) > \
				(softc)->cmap_count += (softc)->cmap_index) \
				(softc)->cmap_count = (index) + (count); \
			if ((index) < (softc)->cmap_index) \
				(softc)->cmap_index = (index); \
			(softc)->cmap_count -= (softc)->cmap_index; \
		} \
		else { \
			(softc)->cmap_index = (index); \
			(softc)->cmap_count = (count); \
			(softc)->flags |= CG4_UPDATE_PENDING; \
		} \
	else \
		(softc)->flags |= CG4_UPDATE_PENDING
#endif lint

/* forward references */
static void cgfour_set_enable_plane();
static void cgfour_reset_cmap();
static void cgfour_cmap_bcopy();
static void cgfour_reset_b();
static void cgfourintr_a();
static void cgfourintr_b();


#if NWIN > 0

/* SunWindows specific stuff */

/* kernel pixrect ops vector */
struct pixrectops cg4_ops = {
	mem_rop,
	cg4_putcolormap,
	cg4_putattributes
};

cg4_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg4_softc *softc = &cg4_softc[cg4_d(pr)->fd];
	register u_int rindex = (u_int) index;
	register u_int rcount = (u_int) count;
	register u_char *map;
	register u_int entries;

	switch (softc->cgd.active) {
	case CG4_FBINDEX_COLOR:
		map = softc->cmap_rgb;
		entries = CG4_CMAP_ENTRIES;
		break;

	case CG4_FBINDEX_OVERLAY:
		if (softc->flags & CG4_OVERLAY_CMAP) {
			map = softc->omap_rgb;
			entries = 2;
			break;
		}
		/* fall through */

	default:
		if (mem_putcolormap(&softc->pr, index, count, 
			red, green, blue))
			return PIX_ERR;

		softc->cgd.fb[softc->cgd.active].mprp.mpr.md_flags =
			softc->cgd.mprp.mpr.md_flags;

		return 0;
	}

	/* check arguments */
	if (rindex >= entries || 
		rindex + rcount > entries) 
		return PIX_ERR;

	if (rcount == 0)
		return 0;

	/* lock out updates of the hardware colormap */
	if (cgfour_update_pending(softc))
		cg4_int_disable(softc);

	switch (softc->type) {
	case 0:
#ifdef CG4A
		map += rindex;
		bcopy((caddr_t) red,   (caddr_t) map, rcount);
		map += entries;
		bcopy((caddr_t) green, (caddr_t) map, rcount);
		map += entries;
		bcopy((caddr_t) blue,  (caddr_t) map, rcount);
		break;
	case 1:
#endif CG4A
		map += rindex * 3;
		cgfour_cmap_bcopy(red,   map++, rcount);
		cgfour_cmap_bcopy(green, map++, rcount);
		cgfour_cmap_bcopy(blue,  map,   rcount);
		break;
	}

	/* overlay colormap update */
	if (entries <= 2)
		rcount = 0;

	cgfour_update_cmap(softc, rindex, rcount);

	/* enable interrupt so we can load the hardware colormap */
	cg4_int_enable(softc);

	return 0;
}

#endif	NWIN > 0

/*
 * Determine if a cgfour exists at the given address.
 */
/*ARGSUSED*/
cgfourprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	static struct cg4_ptab {
		int	cmap_addr;		/* color map address */
		int	fb_addr[CG4_NFBS];	/* frame buffer addresses */
	} cg4_ptab[CG4_NTYPES] = {
#ifdef CG4A
	{	CG4A_PGT_CMAP | btop(CG4A_ADDR_CMAP), {
	  	CG4A_PGT_FB | btop(CG4A_ADDR_OVERLAY), 
		CG4A_PGT_FB | btop(CG4A_ADDR_ENABLE), 
		CG4A_PGT_FB | btop(CG4A_ADDR_COLOR)
	} },
#endif CG4A
	{	CG4B_PGT_CMAP | btop(CG4B_ADDR_CMAP), {
		CG4B_PGT_FB | btop(CG4B_ADDR_OVERLAY), 
		CG4B_PGT_FB | btop(CG4B_ADDR_ENABLE), 
		CG4B_PGT_FB | btop(CG4B_ADDR_COLOR) 
	} } };

	register struct cg4_softc *softc = &cg4_softc[unit];
	register struct cg4_ptab *ptab = cg4_ptab;
	int pages1, pages8;
	long kmx, rmalloc();

	/* make sure this unit is not in use */
	if (softc->cmap)
		return 0;

	/* probe the colormap and determine the cg4 type */
	{
		register union cg4_cmap *cmap = (union cg4_cmap *) reg + NBPG;

#ifdef CG4A
		softc->type = 0;
#else  CG4A
		softc->type = 1;
#endif CG4A

		while (1) {
			fbmapin((caddr_t) cmap, ptab->cmap_addr, sizeof *cmap);

			if (peekc((char *) cmap) != -1) 
				break;

			if (++ptab >= ENDOF(cg4_ptab))
				return 0;

			softc->type++;
		}

		softc->cmap = cmap;
	}

	/* 
	 * Determine frame buffer resolution:
	 *
	 * 1. If this is a P4 frame buffer, use the P4 type code.
	 * 2. Otherwise, assume it's a 3/110.  Use the EEPROM size code
	 *    to guess whether it's 1152 x 900 or 1024 x 1024.
	 */
	{
		int id;

		if ((id = p4probe(reg, &softc->w, &softc->h)) >= 0) {
			if (id != P4_ID_COLOR8P1)
				return 0;

			softc->p4reg = (u_long *) reg;
		}
		else {
			softc->p4reg = 0;

			softc->w = 1152;
			softc->h = 900;

			if (EEPROM->ee_diag.eed_scrsize == EED_SCR_1024X1024) {
				softc->w = 1024;
				softc->h = 1024;
			}
		}

		pages1 = btoc(mpr_linebytes(softc->w, 1) * softc->h);
		pages8 = btoc(mpr_linebytes(softc->w, 8) * softc->h);
	}

	softc->size = 0;
	softc->bw2size = ctob(pages1);

	/* map frame buffer sections */
	{
		register caddr_t fbaddr;
		register int fb;

		/* allocate kernel space for all sections */
		if ((kmx = rmalloc(kernelmap, 
			(long) (pages1 + pages1 + pages8))) == 
			0L) {
			printf("cgfourprobe: out of kernelmap\n");
			fbmapout((caddr_t) softc->cmap, sizeof *softc->cmap);
			return 0;
		}

		/* compute virtual addr. of first section */
		fbaddr = (caddr_t) kmxtob(kmx);

		/* map each frame buffer section */
		for (fb = 0; fb < CG4_NFBS; fb++) {
			register int pages;

			pages = cg4_fbdesc[fb].depth == 1 ? pages1 : pages8;

			fbmapin(fbaddr, ptab->fb_addr[fb], ctob(pages));

			softc->fb[fb] = fbaddr;
			softc->size += ctob(pages);

			if (peekc((char *) fbaddr) != -1) {
				fbaddr += ctob(pages);
				continue;
			}

			/* this section missing, map out everything */
			fbmapout((caddr_t) softc->cmap, sizeof *softc->cmap);
			fbmapout(softc->fb[0], softc->size);

			(void) rmfree(kernelmap, 
				(long) (pages1 + pages1 + pages8), kmx);

			return 0;
		}
	}

	/* probe succeeded */
	return CG4_PROBESIZE;
}


cgfourattach(md)
	register struct mb_device *md;
{
	register struct cg4_softc *softc = &cg4_softc[md->md_unit];

	softc->flags = 0;
	softc->owner = NULL;

 	softc->gattr = cg4attrdefault;
	softc->gattr.fbtype.fb_height = softc->h;
	softc->gattr.fbtype.fb_width =  softc->w;
	softc->gattr.fbtype.fb_size = softc->size;

	/* 
	 * Initialize hardware colormap and software colormap images.
	 * It might make sense to read the hardware colormap here. 
	 */
	if (softc->type) {
		cgfour_reset_b(softc);
		softc->flags = CG4_OVERLAY_CMAP;
	}
	cgfour_reset_cmap(softc, softc->omap_rgb, 2);
	cgfour_reset_cmap(softc, softc->cmap_rgb, CG4_CMAP_ENTRIES);
	cgfour_update_cmap(softc, 0, CG4_CMAP_ENTRIES);
}

cgfouropen(dev, flag)
	dev_t dev;
	int flag;
{
	return fbopen(dev, flag, NCGFOUR, cgfourinfo);
}

/*ARGSUSED*/
cgfourclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct cg4_softc *softc = &cg4_softc[minor(dev)];

	softc->flags &= CG4_UPDATE_PENDING | CG4_OVERLAY_CMAP;
	softc->owner = NULL;

 	softc->gattr = cg4attrdefault;
	softc->gattr.fbtype.fb_height = softc->h;
	softc->gattr.fbtype.fb_width =  softc->w;
	softc->gattr.fbtype.fb_size = softc->size;

	/* re-initialize overlay colormap (this is a hack!) */
	if (softc->flags & CG4_OVERLAY_CMAP) {
		if (cgfour_update_pending(softc))
			cg4_int_disable(softc);
		cgfour_reset_cmap(softc, softc->omap_rgb, 2);
		cgfour_update_cmap(softc, 0, 0);
		cg4_int_enable(softc);
	}
}

/*ARGSUSED*/
cgfourmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register struct cg4_softc *softc = &cg4_softc[minor(dev)];

	/* 
	 * Initialize overlay enable plane if necessary.
	 *
	 * If the owner wants color (as inferred from its use of the
	 * FBIOGATTR ioctl), set to display color planes.
	 * (This is a hack.)
	 *
	 * Note this will work for a non-owner process as long as
	 * the autoinit bit is set.
	 */
	if (softc->gattr.sattr.flags & FB_ATTR_AUTOINIT) {
		cgfour_set_enable_plane(softc->fb[CG4_FBINDEX_ENABLE], 
			softc->w, softc->h,
			softc->flags & CG4_OWNER_WANTS_COLOR ? 0 : 1);

		softc->gattr.sattr.flags &= ~FB_ATTR_AUTOINIT;
	}

	if ((u_int) off >= softc->size)
		return -1;

	return fbgetpage(softc->fb[0] + off);
}

/*ARGSUSED*/
cgfourioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct cg4_softc *softc = &cg4_softc[minor(dev)];
	static u_char cmapbuf[CG4_CMAP_ENTRIES];

	switch (cmd) {

	case FBIOPUTCMAP: 
	case FBIOGETCMAP: {
		register struct fbcmap *cmap = (struct fbcmap *) data;
		register u_int index = (u_int) cmap->index;
		register u_int count = (u_int) cmap->count;
		register u_char *map;
		register u_int entries;

		switch (PIX_ATTRGROUP(index)) {
		case 0:
		case PIXPG_8BIT_COLOR:
			map = softc->cmap_rgb;
			entries = CG4_CMAP_ENTRIES;
			break;

		case PIXPG_OVERLAY:
			if (softc->flags & CG4_OVERLAY_CMAP) {
				map = softc->omap_rgb;
				entries = 2;
				break;
			}
			/* fall through */

		default:
			return EINVAL;
		}

		if ((index &= PIX_ALL_PLANES) >= entries || 
			index + count > entries)
			return EINVAL;

		if (count == 0)
			return 0;

		if (cmd == FBIOPUTCMAP) { 
			register int error;

			if (cgfour_update_pending(softc))
				cg4_int_disable(softc);

			switch (softc->type) {
#ifdef CG4A
			case 0:
				error = copyin((caddr_t) cmap->red, 
					(caddr_t) (map += index), count) ||
					copyin((caddr_t) cmap->green, 
					(caddr_t) (map += entries), count) ||
					copyin((caddr_t) cmap->blue, 
					(caddr_t) (map += entries), count);
				break;
#endif CG4A
			case 1:
				map += index * 3;

				if (error = copyin((caddr_t) cmap->red, 
					(caddr_t) cmapbuf, count))
					break;
				cgfour_cmap_bcopy(cmapbuf, map++, count);

				if (error = copyin((caddr_t) cmap->green, 
					(caddr_t) cmapbuf, count))
					break;
				cgfour_cmap_bcopy(cmapbuf, map++, count);

				if (error = copyin((caddr_t) cmap->blue, 
					(caddr_t) cmapbuf, count))
					break;
				cgfour_cmap_bcopy(cmapbuf, map, count);

				break;
			}

			if (error) {
				if (cgfour_update_pending(softc)) 
					cg4_int_enable(softc);
				return EFAULT;
			}

			/* overlay colormap update */
			if (entries <= 2)
				count = 0;

			cgfour_update_cmap(softc, index, count);
			cg4_int_enable(softc);
		}
		else {
			/* FBIOGETCMAP */
			switch (softc->type) {
#ifdef CG4A
			case 0:
				if (copyout((caddr_t) (map += index),
					(caddr_t) cmap->red, count) ||
					copyout((caddr_t) (map += entries),
					(caddr_t) cmap->green, count) ||
					copyout((caddr_t) (map += entries),
					(caddr_t) cmap->blue, count))
					return EFAULT;

				break;
#endif CG4A
			case 1: 
				map += index * 3;
				cgfour_cmap_bcopy(cmapbuf, map++, -count);
				if (copyout((caddr_t) cmapbuf,
					(caddr_t) cmap->red, count))
					return EFAULT;

				cgfour_cmap_bcopy(cmapbuf, map++, -count);
				if (copyout((caddr_t) cmapbuf,
					(caddr_t) cmap->green, count))
					return EFAULT;

				cgfour_cmap_bcopy(cmapbuf, map, -count);
				if (copyout((caddr_t) cmapbuf,
					(caddr_t) cmap->blue, count))
					return EFAULT;

				break;
			}
		}
	}
	break;

	case FBIOSATTR: {
		register struct fbsattr *sattr = (struct fbsattr *) data;

#ifdef ONLY_OWNER_CAN_SATTR
		/* this can only happen for the owner */
		if (softc->owner != u.u_procp)
			return ENOTTY;
#endif ONLY_OWNER_CAN_SATTR

		softc->gattr.sattr.flags = sattr->flags;

		if (sattr->emu_type != -1)
			softc->gattr.sattr.emu_type = sattr->emu_type;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC) {
			bcopy((char *) sattr->dev_specific,
				(char *) softc->gattr.sattr.dev_specific,
				sizeof sattr->dev_specific);

			if (softc->gattr.sattr.dev_specific
				[FB_ATTR_CG4_SETOWNER_CMD] == 1) {
				struct proc *pfind();
				register struct proc *newowner = 0;

				if (softc->gattr.sattr.dev_specific
					[FB_ATTR_CG4_SETOWNER_PID] > 0 &&
					(newowner = pfind(
					softc->gattr.sattr.dev_specific
						[FB_ATTR_CG4_SETOWNER_PID]))) {
					softc->owner = newowner;
					softc->gattr.owner = newowner->p_pid;
				}

				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG4_SETOWNER_CMD] = 0;
				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG4_SETOWNER_PID] = 0;

				if (!newowner)
					return ESRCH;
			}
		}
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		/* set owner if not owned or previous owner is dead */
		if (softc->owner == NULL || 
			softc->owner->p_stat == NULL ||
			softc->owner->p_pid != softc->gattr.owner) {

			softc->owner = u.u_procp;
			softc->gattr.owner = u.u_procp->p_pid;
			softc->gattr.sattr.flags |= FB_ATTR_AUTOINIT;
		}

		*gattr = softc->gattr;

		if (u.u_procp == softc->owner) {
			gattr->owner = 0;
			softc->flags |= CG4_OWNER_WANTS_COLOR;
		}
	}
	break;

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		/* set owner if not owned or previous owner is dead */
		if (softc->owner == NULL || 
			softc->owner->p_stat == NULL || 
			softc->owner->p_pid != softc->gattr.owner) {

			softc->owner = u.u_procp;
			softc->gattr.owner = u.u_procp->p_pid;
			softc->gattr.sattr.flags |= FB_ATTR_AUTOINIT;
		}

		switch(softc->gattr.sattr.emu_type) {
		case FBTYPE_SUN2BW:
			*fb = cg4typedefault;
			fb->fb_height = softc->h;
			fb->fb_width  = softc->w;
			fb->fb_size = softc->bw2size;
			break;

		case FBTYPE_SUN4COLOR:
		default:
			*fb = softc->gattr.fbtype;
			break;
		}
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT: {
		struct fbpixrect *fbpr = (struct fbpixrect *) data;
		register struct cg4fb *fbp;
		register struct cg4_fbdesc *descp;
		register int i;

		/* "Allocate" pixrect and private data */
		fbpr->fbpr_pixrect = &softc->pr;
		softc->pr.pr_data = (caddr_t) &softc->cgd;

		/* initialize pixrect */
		softc->pr.pr_ops = &cg4_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;

		/* initialize private data */
		softc->cgd.flags = 0;
		softc->cgd.planes = 0;
		softc->cgd.fd = minor(dev);

		for (fbp = softc->cgd.fb, descp = cg4_fbdesc, i = 0; 
			i < CG4_NFBS; fbp++, descp++, i++) {
			fbp->group = descp->group;
			fbp->depth = descp->depth;
			fbp->mprp.mpr.md_linebytes = 
				mpr_linebytes(softc->w, descp->depth);
			fbp->mprp.mpr.md_image = (short *) softc->fb[i];
			fbp->mprp.mpr.md_offset.x = 0;
			fbp->mprp.mpr.md_offset.y = 0;
			fbp->mprp.mpr.md_primary = 0;
			fbp->mprp.mpr.md_flags = descp->allplanes != 0
				? MP_DISPLAY | MP_PLANEMASK
				: MP_DISPLAY;
			fbp->mprp.planes = descp->allplanes;
		}

		/* set up pixrect initial state */
		{
			int initplanes = 
				PIX_GROUP(cg4_fbdesc[CG4_INITFB].group) |
				cg4_fbdesc[CG4_INITFB].allplanes;

			(void) cg4_putattributes(&softc->pr, &initplanes);
		}

		/* enable video */
		if (softc->p4reg)
			*softc->p4reg |= P4_REG_VIDEO;
		else
			setvideoenable(1);
	}
	break;

#endif NWIN > 0

	case FBIOSVIDEO: {
		register int on = * (int *) data & FBVIDEO_ON;

		if (softc->p4reg)
			*softc->p4reg = *softc->p4reg & 
				~(P4_REG_INTCLR | P4_REG_VIDEO) |
				(on ? P4_REG_VIDEO : 0);
		else
			setvideoenable((u_int) on);
	}
	break;

	/* get video flags */
	case FBIOGVIDEO:
		* (int *) data = 
			(softc->p4reg ?
				*softc->p4reg & P4_REG_VIDEO :
				enablereg & ENA_VIDEO) ?
				FBVIDEO_ON : FBVIDEO_OFF;
		break;

	default:
		return ENOTTY;

	} /* switch(cmd) */

	return 0;
}

cgfourpoll()
{
	register int serviced = 0;
	register struct cg4_softc *softc;

	/* 
	 * Look for any frame buffers that were expecting an interrupt.
	 * For P4 frame buffers, check the interrupt status bit too.
	 */
	for (softc = cg4_softc; softc < ENDOF(cg4_softc); softc++)
		if (cgfour_update_pending(softc)) {
			switch (softc->type) {
#ifdef CG4A
			case 0:
				cgfourintr_a(softc); 
				serviced++; 
				break;
#endif CG4A
			case 1:
				if (!softc->p4reg ||
					*softc->p4reg & P4_REG_INT)
					cgfourintr_b(softc); 
					serviced++;
				}
				break;
			}
		/* catch stray P4 interrupts */
		else if (softc->p4reg && 
			*softc->p4reg & P4_REG_INT) {
			cg4_int_disable(softc);
			serviced++;
		}

	return serviced;
}

#ifdef CG4A

static void
cgfourintr_a(softc) 
	register struct cg4_softc *softc;
{
	register struct cg4a_cmap *cmap = &softc->cmap->a;

	/* make sure we're in the first half of vertical blanking period */
	if (cmap->status & CG4A_STATUS_FIRSTHALF) {
		register LOOP_T count = softc->cmap_count;
		register u_long *in  = &softc->cmap_image.cmap_long[0];
		register CMAP_T *out = (CMAP_T *) cmap->red;

		/* loading most or all of colormap, use unrolled loop */
#ifdef mc68000
		if (count >= 160) {
			count = CG4_CMAP_ENTRIES * 3 / sizeof *out / 8 - 1;

			PR_LOOPVP(count,
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++);
		}
		/* small to medium colormap load, use rolled loop */
		else 
#endif mc68000
		{
			register int index = softc->cmap_index;

			/* convert count to longs */
			count = (count + (index & 3) + 3) >> 2;

			/* round index to long boundary */
			index &= ~3;

			PTR_INCR(u_long *, in, index);
			PTR_INCR(CMAP_T *, out, index);

			index = ((CG4_CMAP_ENTRIES >> 2) - count) << 2;
			count--;

			PR_LOOPP(3 - 1,
				PR_LOOPP(count,
					IF68000(
						*out++ = *in++
						,
						register u_long tmp;

						tmp = *in++;
						*out++ = tmp >> 24;
						*out++ = tmp >> 16;
						*out++ = tmp >> 8;
						*out++ = tmp));
				PTR_INCR(u_long *, in, index);
				PTR_INCR(CMAP_T *, out, index));
		}

		/* update toolate statistics */
		if (cmap->status & CG4A_STATUS_TOOLATE)
			cg4_toolate++;

		softc->cmap_count = 0;
		softc->flags &= ~CG4_UPDATE_PENDING;
		setintrenable(0);
	}
	/* missed first half of blanking, catch the next retrace */
	else {
		cg4_firsthalf++;
		setintrenable(0);
		setintrenable(1);
	}
}
#endif CG4A

static void
cgfourintr_b(softc) 
	register struct cg4_softc *softc;
{
	register struct cg4b_cmap *cmap = &softc->cmap->b;

	/* load overlay color map */
	{
		register u_char *in = softc->omap_rgb;
		register u_char *out = &cmap->omap;

		/* background color */
		cmap->addr = 1;
		*out = *in++;
		*out = *in++;
		*out = *in++;

		/* foreground color */
		cmap->addr = 3;
		*out = *in++;
		*out = *in++;
		*out = *in;
	}

	/* load main color map */
	{
		register LOOP_T count = softc->cmap_count;
#ifdef mc68000
		register u_char *in = &softc->cmap_image.cmap_char[0][0];
#else mc68000
		register u_long *in = &softc->cmap_image.cmap_long[0];
		register u_long tmp;
#endif  mc68000

		{
			register LOOP_T index = softc->cmap_index;

			/* convert count to multiple of 12 bytes */
			count = ((count + (index & 3) + 3) >> 2) * 3;

			/* round index to 4 entry boundary */
			index &= ~3;

			cmap->addr = index;
			PTR_INCR(IF68000(u_char *, u_long *), in, index * 3);
		}

		{
			register u_char *out = &cmap->cmap;

			/* copy 4 bytes (4/3 RGB entries) per loop iteration */
			PR_LOOPV(count, IF68000(
				*out = *in++;
				*out = *in++;
				*out = *in++;
				*out = *in++;
				,
				tmp = *in++;
				*out = tmp >> 24;
				*out = tmp >> 16;
				*out = tmp >> 8;
				*out = tmp;
				));
		}
	}

	softc->cmap_count = 0;
	softc->flags &= ~CG4_UPDATE_PENDING;
	cg4_int_disable(softc);
}

/* enable display of overlay plane or color planes */
static void
cgfour_set_enable_plane(image, w, h, overlay)
	caddr_t image;
	int w, h;
	int overlay;
{
	register u_long *lp = (u_long *) image, on, count;

	/* 1s enable the overlay plane, 0s enable the color planes */
	on = 0;
	if (overlay)
		on = ~on;

	/* compute number of 32-bit longs to be set */
	count = pr_product(mpr_linebytes(w, 1), h) >> 2;

	/* make sure count is a multiple of 8 */
	while ((count & 7) != 0) {
		*lp++ = on;
		count--;
	}

	PR_LOOP(count >> 3,
		*lp++ = on;
		*lp++ = on;
		*lp++ = on;
		*lp++ = on;
		*lp++ = on;
		*lp++ = on;
		*lp++ = on;
		*lp++ = on);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
cgfour_reset_cmap(softc, cmap, entries)
	register struct cg4_softc *softc;
	register u_char *cmap;
	register u_int entries;
{
	bzero((char *) cmap, entries * 3);
	switch (softc->type) {
#ifdef CG4A
	case 0:
		* cmap             = 255;
		*(cmap += entries) = 255;
		*(cmap += entries) = 255;
		break;
#endif CG4A
	case 1:
		*cmap++ = 255;
		*cmap++ = 255;
		*cmap   = 255;
		break;
	}
}

/*
 * Copy colormap entries between red, green, or blue array and
 * interspersed rgb array.
 *
 * count > 0 : copy count bytes from buf to rgb
 * count < 0 : copy -count bytes from rgb to buf
 */
static void
cgfour_cmap_bcopy(bufp, rgb, count)
	register u_char *bufp, *rgb;
	u_int count;
{
	register LOOP_T rcount = count;

	if (--rcount >= 0)
		PR_LOOPVP(rcount,
			*rgb = *bufp++;
			rgb += 3);
	else {
		rcount = -rcount - 2;
		PR_LOOPVP(rcount,
			*bufp++ = *rgb;
			rgb += 3);
	}
}

/* 
 * Initialize type B (Brooktree) color map chip.
 * This should be done by the boot PROM, but who knows?
 */
static void
cgfour_reset_b(softc)
	struct cg4_softc *softc;
{
	register struct cg4b_cmap *cmap = &softc->cmap->b;
	register u_char *p;

	/* control register initial values (addr, data) */
	static u_char ctrltab[4 * 2] = {
		/* read mask = all 1s */
		4, 0xff,

		/* blink mask = all 0s*/
		5, 0,

		/* 
		 * command register
		 *  CR7   0	4:1 multiplexing
		 *  CR6   1	use color palette RAM
		 *  CR5:4 11	blink divisor = 65536
		 *  CR3:2 00	OL blink disabled
		 *  CR1:0 11	OL display enabled
		 */
		6, 0x73,

		/* test register = 0 */
		7, 0
	};

	/* overlay color map initial values */
	static u_char otab[12] = {
		/* OL color 0 (not used) = yellow */
		255, 255,   0,

		/* OL color 1 (overlay plane background) = white */
		255, 255, 255,

		/* OL color 2 (not used) = cyan */
		  0, 255, 255,

		/* OL color 3 (overlay plane foreground) = black */
		  0,   0,   0
	};

	/* load control registers */
	p = ctrltab;
	PR_LOOPP(ITEMSIN(ctrltab) / 2 - 1, 
		cmap->addr = *p++;
		cmap->ctrl = *p++);

	/* load overlay color map */
	cmap->addr = 0;
	p = otab;
	PR_LOOPP(ITEMSIN(otab) - 1, cmap->omap = *p++);
}

