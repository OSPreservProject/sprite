/* A simple window manager */

/****************************************
 *	James Gosling, 1983		*
 *	Copyright (c) 1983 IBM		*
 ****************************************/

#include "framebuf.h"
#include "stdio.h"
#include "font.h"
#include "menu.h"
#include "window.h"
#include "display.h"
#include "ctype.h"

char ProgramName[] = "wm";
int	SmallScreen = 0;

extern abort ();

main (argc, argv)
char  **argv; {
#ifdef DEBUG
    unlink ("/tmp/debug");
    freopen ("/tmp/debug", "w", stdout);
#endif
     while (--argc) {
 	++argv;
 	if (**argv == '-') switch (*++*argv) {
 	    case 'S':
 	        SmallScreen++;
 		break;
	    case 'L':
	    	SmallScreen = 0;
		break;
 	}
     }
    InitializeSun1bw ();
    InitializeSun1Color ();
    if (NDisplays <= 0)
	printf ("I couldn't find any display devices.\n");
    else {
	BuildFontDirectory ();
	InitializeWindowSystem ();
	DispatchLoop ();
    }
}
