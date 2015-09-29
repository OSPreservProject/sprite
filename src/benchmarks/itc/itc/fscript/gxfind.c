
/*
 * This module defines the external integer variables
 *	GXBase		Base virtual address of the frame buffer
 *	GXConfig	Longword defining various config bits, see
 *			/usr/include/sys/ioctl.h
 *	GXDeviceName	Char * to name of the device we opened or attempted,
 *			NULL if standalone.
 *	GXFile		File descriptor of the frame buffer if running
 *			under Unix, or -1 otherwise
 * and contains the routine GXfind() which will set them.
 *
 * Written 21 Apr 82 by JCGilmore.
 *	Modified for 4.1c by James Gosling.
 */

#include "sgtty.h"
#include "stdio.h"
#include "framebuf.h"


int	GXBase, 		/* Default (standalone pgm) address */
	GXConfig = GXCPresent+GXCLandscape;   /* and config flags */
char *	GXDeviceName = NULL;	/* Name of gfx dev eg "/dev/gfx" in Unix */
int	GXFile = -1;		/* File descriptor of /dev/gfx in Unix */

int
GXfind()
{
	register ps = getpagesize();
	GXDeviceName = (char *)getenv ("GraphicsDev");
	if (GXDeviceName == NULL) GXDeviceName = "/dev/console";
	GXFile = open (GXDeviceName, 1);
	if (GXFile < 0) {
		fprintf (stderr, "GXfind: %s not found\n", GXDeviceName);
		return -1;
	}
	GXBase = malloc (0x20000+ps);
	GXBase = (GXBase+ps-1) & ~(ps-1);
	mmap (GXBase, 0x20000, 2, 1, GXFile, 0);
	return 0;
}
