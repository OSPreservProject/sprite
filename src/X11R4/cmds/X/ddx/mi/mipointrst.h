/*
 * mipointrst.h
 *
 */

/* $XConsortium: mipointrst.h,v 5.1 89/06/21 11:16:59 rws Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/

# include   <mipointer.h>
# include   <input.h>

typedef struct {
    ScreenPtr		    pScreen;    /* current screen */
    CursorPtr		    pCursor;    /* current cursor */
    BoxRec		    limits;	/* current constraints */
    int			    x, y;	/* hot spot location */
    DevicePtr		    pPointer;   /* pointer device structure */
    miPointerCursorFuncPtr  funcs;	/* device-specific methods */
} miPointerRec, *miPointerPtr;

typedef struct {
    miPointerSpriteFuncPtr  funcs;
    miPointerPtr	    pPointer;
    Bool		    (*CloseScreen)();
} miPointerScreenRec, *miPointerScreenPtr;
