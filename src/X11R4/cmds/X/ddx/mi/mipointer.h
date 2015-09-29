/*
 * mipointer.h
 *
 */

/* $XConsortium: mipointer.h,v 5.2 89/06/21 11:16:15 rws Exp $ */

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

typedef struct {
    Bool	(*RealizeCursor)();	/* pScreen, pCursor */
    Bool	(*UnrealizeCursor)();	/* pScreen, pCursor */
    void	(*DisplayCursor)();	/* pScreen, pCursor, x, y */
    void	(*UndisplayCursor)();	/* pScreen, pCursor */
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct {
    long	(*EventTime)();		/* pScreen */
    Bool	(*CursorOffScreen)();	/* pScreen, x, y */
    void	(*CrossScreen)();	/* pScreen, entering */
    void	(*QueueEvent)();	/* pxE, pPointer, pScreen */
} miPointerCursorFuncRec, *miPointerCursorFuncPtr;

extern void miPointerPosition (),	miRegisterPointerDevice();
extern void miPointerDeltaCursor (),	miPointerMoveCursor();
extern Bool miPointerInitialize ();
