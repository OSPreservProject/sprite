/*-
 * sunCursor.c --
 *	Functions for maintaining the Sun software cursor...
 *
 */

#define NEED_EVENTS
#include    "sun.h"
#include    <windowstr.h>
#include    <regionstr.h>
#include    <dix.h>
#include    <dixstruct.h>
#include    <opaque.h>

#include    <servermd.h>
#include    "mipointer.h"

/*
 * The following struct is from win_cursor.h.  This file can't be included 
 * directly, because it drags in all of the SunView attribute stuff along 
 * with it.
 */

#ifdef SUN_WINDOWS

struct cursor {
    short       cur_xhot, cur_yhot;	/* offset of mouse position from shape */
    int         cur_function;		/* relationship of shape to screen */
    struct pixrect *cur_shape;		/* memory image to use */
    int         flags;			/* various options */
    short       horiz_hair_thickness;	/* horizontal crosshair height */
    int         horiz_hair_op;		/* drawing op       */
    int         horiz_hair_color;	/* color            */
    short       horiz_hair_length;	/* width           */
    short       horiz_hair_gap;		/* gap             */
    short       vert_hair_thickness;	/* vertical crosshair width  */
    int         vert_hair_op;		/* drawing op       */
    int         vert_hair_color;	/* color            */
    short       vert_hair_length;	/* height           */
    short       vert_hair_gap;		/* gap              */
};
#endif SUN_WINDOWS

void
sunInitCursor ()
{
    if ( sunUseSunWindows() ) {
#ifdef SUN_WINDOWS
	static struct cursor cs;
	static struct pixrect pr;
	/* 
	 * Give the pixwin an empty cursor so that the kernel's cursor drawing 
	 * doesn't conflict with our cursor drawing.
	 */
	cs.cur_xhot = cs.cur_yhot = cs.cur_function = 0;
	cs.flags = 0;
	cs.cur_shape = &pr;
	pr.pr_size.x = pr.pr_size.y = 0;
	win_setcursor( windowFd, &cs );
#endif SUN_WINDOWS
    }
}


#ifdef SUN_WINDOWS
/*
 * We need to find out when dix warps the mouse so we can
 * keep SunWindows in sync.
 */

Bool (*realSetCursorPosition)();
extern int sunIgnoreEvent;


Bool
sunSetCursorPosition(pScreen, x, y, generateEvent)
	ScreenPtr pScreen;
	int x, y;
	Bool generateEvent;
{
	(*realSetCursorPosition)(pScreen, x, y, generateEvent);
	if (sunUseSunWindows())
	    if (!sunIgnoreEvent)
		win_setmouseposition(windowFd, x, y);
	return TRUE;
}
#endif







