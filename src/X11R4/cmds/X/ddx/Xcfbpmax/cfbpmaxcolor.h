/* $Header: cfbpmaxcolor.h,v 1.2 89/07/19 21:27:04 keith Exp $ */

/* 
 * cfbpmaxcolor.h - declarations for cfbpmaxcolor.c
 * 
 * Author:	susan
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Aug 20 1988
 */

/* $Log:	cfbpmaxcolor.h,v $
 * Revision 1.2  89/07/19  21:27:04  keith
 * update to new interfaces, remove duplicated code
 * 
 * Revision 1.1  89/07/18  21:30:50  keith
 * Initial revision
 * 
 * Revision 2.0  88/08/22  14:22:11  erik
 * version from /usr/src/pmax
 * 
 * Revision 1.1  88/08/20  17:24:58  joel
 * Initial revision
 *  */


extern void cfbpmaxStoreColors();
extern void cfbpmaxInstallColormap();
extern void cfbpmaxUninstallColormap();
extern int cfbpmaxListInstalledColormaps();

extern int fdPM;   /* this is the file descriptor for screen so
		    can do IOCTL to colormap */
