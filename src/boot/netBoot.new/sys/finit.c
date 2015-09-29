/*
 *	@(#)finit.c 1.5 86/05/24 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../sun3/cpu.addrs.h"
#include "../h/globram.h"
#include "../h/dpy.h"
#include "../h/video.h"
#include "../h/enable.h"
#include "../h/fbio.h"
#include "../h/eeprom.h"

extern unsigned char f_bitmap[], f_index[], f_data_hi[], f_data_lo[];

finit(newx, newy)
	unsigned newx, newy;
{
unsigned short t_cols,t_rows;  /* for cols and rows  calc */

	/* Expand the compressed font into RAM */
	fexpand (font, f_bitmap, FBITMAPSIZE, f_index, f_data_hi, f_data_lo);
#ifdef PRISM

        gp->g_fbtype = FBTYPE_SUN4COLOR;           

#else PRISM
	if (init_scolor() >= 0)   /* check for color */
                gp->g_fbtype = FBTYPE_SUN2COLOR;
	else {
		gp->g_fbtype = FBTYPE_SUN2BW;
	}
#endif	PRISM
	/*
	 * Decide whether we are 1024*1024 or 1192*900 and/or color.
	 */
	if(EEPROM->ee_diag.eed_scrsize == EED_SCR_1024X1024) {
		SCRWIDTH = 1024;
		SCRHEIGHT = 1024;
		WINTOP = 16+224/2;	/* Like Sun-1 but centered in 1024 */
		WINLEFT	= 16;
	} else if (EEPROM->ee_diag.eed_scrsize == EED_SCR_1600X1280 &&
                   gp->g_fbtype != FBTYPE_SUN2COLOR){
                SCRWIDTH = 1600;
                SCRHEIGHT = 1280;
		t_cols = EEPROM->ee_diag.eed_colsize;
		t_rows = EEPROM->ee_diag.eed_rowsize;
		if(t_cols < 10) t_cols = 10;
		if(t_cols > (char)120) t_cols = 120;
		if(t_rows < 10) t_rows = 10;
		if(t_rows > 48) t_rows = 48;
#ifdef SIRIUS
		BOTTOM = t_rows;
		RIGHT = t_cols;
#endif SIRIUS
		/* now initialize to appropriate size */
		t_cols = (120 - t_cols) / 2;  /* gives "centered" loc */
		t_cols = (t_cols * CHRWIDTH) + 64 ;
		WINLEFT = t_cols ;
		WINLEFT += 70;
		/* now for the row */
		t_rows = 48 - t_rows;
		t_rows >>= 1 ;  /* by 2 */ 
		t_rows = (t_rows * CHRHEIGHT) + 56;
		WINTOP = t_rows;
		WINTOP += 30;  /* tried 60  */
	} else {
		SCRWIDTH = 1152;
		SCRHEIGHT = 900;
		WINTOP = 56;
		WINLEFT	= 64;
	}

	/* set for pixrect code */
#ifdef SIRIUS
        if(gp->g_fbtype == FBTYPE_SUN2COLOR) {  /* sirius can have color brd */
		RIGHT = 80;
		BOTTOM = 34; 	/* but must adhere to 1152 */
	}

        WINBOT = WINTOP+BOTTOM*CHRHEIGHT;
#endif SIRIUS

	/*
	 * Set up all the various garbage for the rasterop (pixrect) code.
	 */
	GXBase = (int)VIDEOMEM_BASE;
	fbdata.md_linebytes = SCRWIDTH/8;
/*	fbdata.md_image = GXBase;  they are identical */
	fbdata.md_offset.x = 0;
	fbdata.md_offset.y = 0;
/*	fbdata.md_primary is unreferenced */

/*	fbpr.pr_ops is unreferenced */
/*	fbpr.pr_size.x is unreferenced */
/*	fbpr.pr_size.y is unreferenced */
/*	fbpr.pr_depth is unreferenced */
	fbpr.pr_data = (caddr_t)&fbdata;

	fbpos.pr = &fbpr;
/*	fbpos.pos.x is set up by the call to pos() below */
/*	fbpos.pos.y is set up by the call to pos() below */

	chardata.md_linebytes = 2;
/*	chardata.md_image is filled in at run time */
	chardata.md_offset.x = 0;
	chardata.md_offset.y = 0;
/*	chardata.md_primary is unreferenced */

/*	charpr.pr_ops is unreferenced */
	charpr.pr_size.x = CHRWIDTH;
	charpr.pr_size.y = CHRHEIGHT;
/*	charpr.pr_depth is unreferenced */
	charpr.pr_data = (caddr_t)&chardata;

	charpos.pr = &charpr;
	charpos.pos.x = 0;
	charpos.pos.y = 0;
	
	/* Sun-2 0-bits are white, 1-bits are black. */
	fillfunc = POX_CLR;			/* White screen */
	chrfunc = POX_SRC;			/* No inverse video */
	state = ALPHA;
	cursor = BLOCKCURSOR;
	scrlins = 1;				/* scroll by 1's initially */
#ifdef PRISM
        fill_colormaps();       /*
                                 * fill the 3 color maps with a default
                                 * pattern.  fill_colormaps() is in
                                 * diagmenus.c
                                 */
        set_enable(get_enable() | ENA_VIDEO);
                        /* turn on video; normally done in init_scolor, but we
                         * bypass init_scolor since we don't load in scutils.c
                         * for the prism.
                         */
#endif PRISM


	pos(newx, newy);			/* Set ax, ay, dcok */
}
