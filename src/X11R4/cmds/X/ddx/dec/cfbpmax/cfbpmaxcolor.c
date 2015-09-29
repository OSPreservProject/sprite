/* $Header: cfbpmaxcolor.c,v 1.2 89/07/19 21:27:02 keith Exp $ */

/* 
 * cfbpmaxcolor.c - device specific color routines, stored in screen
 * 
 * Author:	susan
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Aug 20 1988
 */

/* $Log:	cfbpmaxcolor.c,v $
 * Revision 1.2  89/07/19  21:27:02  keith
 * update to new interfaces, remove duplicated code
 * 
 * Revision 1.1  89/07/13  17:53:18  keith
 * Initial revision
 * 
 * Revision 2.0  89/02/28  13:14:53  erik
 * version from /usr/src/pmax
 * 
 * Revision 1.6  89/02/28  13:14:53  joel
 * cfbpmaxStoreColors shouldn't store if not installed colormap
 * 
 * Revision 1.5  88/11/30  17:39:53  joel
 * Made cfbpmaxInstallColormap DEALLOCATE memory it had allocated
 * 
 * Revision 1.3  88/11/30  16:24:55  todd
 * Changed constants used for grey scale colormap
 * 
 * Revision 1.2  88/11/01  18:03:15  todd
 *  can now create multiple visuals
 * 
 * Revision 1.1  88/08/22  10:23:33  joel
 * Initial revision
 *  */



static char rcs_ident[] = "$Header: cfbpmaxcolor.c,v 1.2 89/07/19 21:27:02 keith Exp $";

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <errno.h>
#include <sys/devio.h>
#include <machine/pmioctl.h>
#include <machine/dc7085cons.h>
#include "X.h"          /* required for DoRed ... */
#include "Xproto.h"     /* required for xColorItem */

#include "misc.h"       /* required for colormapst.h */
#include "resource.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "cfbpmaxcolor.h"

#define NOMAPYET        (ColormapPtr) 1

static ColormapPtr pInstalledMap = NOMAPYET;

/* these next two are DIX routines */
extern int      TellLostMap();
extern int      TellGainedMap();

void
cfbpmaxStoreColors(pmap, ndef, pdefs)
    ColormapPtr pmap;
    int         ndef;
    xColorItem  *pdefs;
{
    int		idef;
    ColorMap	map;

    if (pmap != pInstalledMap)
	return;

    for(idef = 0; idef < ndef; idef++)
    {
	map.Map = 0;
	map.index = (short) pdefs[idef].pixel;
	map.Entry.red = pdefs[idef].red >>8;
	map.Entry.green = pdefs[idef].green>>8;
	map.Entry.blue = pdefs[idef].blue>>8;
	ioctl(fdPM, QIOSETCMAP, &map);
    }

}

void
cfbpmaxInstallColormap(pcmap)
	ColormapPtr	pcmap;
{
    int         entries = pcmap->pVisual->ColormapEntries;
    Pixel *     ppix;
    xrgb *      prgb;
    xColorItem *defs;
    int         i;

    if ( pcmap == pInstalledMap)
        return;

    ppix = (Pixel *)ALLOCATE_LOCAL( entries * sizeof(Pixel));
    prgb = (xrgb *)ALLOCATE_LOCAL( entries * sizeof(xrgb));
    defs = (xColorItem *)ALLOCATE_LOCAL(entries * sizeof(xColorItem));

    if ( pInstalledMap != NOMAPYET)
        WalkTree( pcmap->pScreen, TellLostMap, &pInstalledMap->mid);
    pInstalledMap = pcmap;
    for ( i=0; i<entries; i++)
        ppix[i] = i;
    QueryColors( pcmap, entries, ppix, prgb);
    for ( i=0; i<entries; i++) /* convert xrgbs to xColorItems */
    {
        defs[i].pixel = ppix[i];
        defs[i].red = prgb[i].red;
        defs[i].green = prgb[i].green;
        defs[i].blue = prgb[i].blue;
        defs[i].flags =  DoRed|DoGreen|DoBlue;
    }
    cfbpmaxStoreColors( pcmap, entries, defs);
    WalkTree(pcmap->pScreen, TellGainedMap, &pcmap->mid);

    DEALLOCATE_LOCAL(ppix);
    DEALLOCATE_LOCAL(prgb);
    DEALLOCATE_LOCAL(defs);
}


void
cfbpmaxUninstallColormap(pcmap)
    ColormapPtr pcmap;
{
    /*  Replace installed colormap with default colormap */

    ColormapPtr defColormap;

    if ( pcmap != pInstalledMap)
        return;

    defColormap = (ColormapPtr) LookupIDByType( pcmap->pScreen->defColormap,
                        RT_COLORMAP);

    if (defColormap == pInstalledMap)
        return;

    (*pcmap->pScreen->InstallColormap) (defColormap);
}

int
cfbpmaxListInstalledColormaps( pscr, pcmaps)
    ScreenPtr   pscr;
    Colormap *  pcmaps;
{
    *pcmaps = pInstalledMap->mid;
    return 1;
}

