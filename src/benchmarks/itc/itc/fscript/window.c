/***********************************************************************\
* 								        *
* 	File: window.c						        *
* 	Copyright (c) 1984 IBM					        *
* 	Date: Tue Jan  3 16:55:39 1984				        *
* 	Author: James Gosling					        *
* 								        *
* Window manipulation routines.					        *
* 								        *
* HISTORY							        *
*   3-Jan-84  James Gosling (jag) at the Information Technology Centre  *
* 	Fixed bug in DeleteWindow which caused it to fail when an       *
* 	invisible window was deleted.				        *
* 								        *
\***********************************************************************/



#include "stdio.h"
#include "sys/ioctl.h"
#include "sgtty.h"
#include "window.h"
#include "font.h"
#include "menu.h"
#include "display.h"

#define W ((struct Window *) w)

static BorderStyle;
static BorderWidth;
static BlackEdgeWidth;
static AutomaticEqualization;
static MinWidth;
static Initialized;
static UnacceptablySmallWindowExists;
static FocusHighlightStyle;
static AlwaysZoom;

static struct icon *LastCursorDrawn;
static struct icon *DefaultCursor;
static struct icon *VerticalCursor;
static struct icon *HorizontalCursor;
static struct icon *UpperLeftCursor;
static struct icon *LowerRightCursor;
static struct icon *RightFingerCursor;
struct SplitWindow  *MovingBar = 0;

RestoreToCurrentCursor () {
    LastCursorDrawn = CursorWindow ? CursorWindow -> Cursor : DefaultCursor;
}

static
DrawCursor () {
    register struct ViewPort   *ov;
    register struct icon   *c;
    if (CursorDisplay != CurrentDisplay) {
	CurrentDisplay = CursorDisplay;
	display = *CurrentDisplay;
    }
    ov = CurrentViewPort;
    "IBM sucks";
    CurrentViewPort = &display.screen;
    if ((c = LastCursorDrawn) != 0 || (c = DefaultCursor) != 0) {
	HW_SelectColor (f_invert | f_CharacterContext);
	HW_DrawIcon (CursorX, CursorY, c);
    }
    CurrentViewPort = ov;
}

CURSORDOWN () {
    if (CursorVisible) {
	DrawCursor ();
	CursorVisible = 0;
    }
}

MoveCursor (x, y) {
    if (x != CursorX || y != CursorY || !CursorVisible) {
	register struct SplitWindow *w = RootWindow[CursorDisplayNumber];
	CursorDown ();
	CursorX = x;
	CursorY = y;
	{
	    struct display *d = &displays[CursorDisplayNumber];

	    x += d->screen.left;
	    y += d->screen.top;
	}
	if (MovingBar)
	    LastCursorDrawn = MovingBar -> SplitHorizontally ? HorizontalCursor : VerticalCursor;
	else
	    if (Reshapee)
		LastCursorDrawn = Reshapee_x < 0 ? UpperLeftCursor : LowerRightCursor;
	    else
		if (MenuActive) {
		    LastCursorDrawn = RightFingerCursor;
		    MenuActive = 1;
		}
		else {
		    while (w && w -> t.type == SplitWindowType) {
			register    diff = w -> SplitValue - (w -> SplitHorizontally ? y : x);
			if (diff < -BorderWidth)
			    w = w -> bottom;
			else
			    if (diff >= BorderWidth)
				w = w -> top;
			    else
				break;
		    }
		    CursorWindow = W;
		    if (w == 0)
			LastCursorDrawn = 0;
		    else if (w -> t.type == SplitWindowType)
				if (w -> SplitHorizontally)
				    LastCursorDrawn = HorizontalCursor;
				else
				    LastCursorDrawn = VerticalCursor;
			 else {
			     register struct WindowRegion *r;
			     register area = 999999;
			     if (CursorWindow != WindowInFocus && FocusFollowsCursor)
				SetInputFocus (W);
			     LastCursorDrawn = W -> Cursor;
			     if (W -> MaxRegion >= 0)
			     for (r = &W -> regions[W -> MaxRegion];
					r >= W -> regions; r--) {
				struct icon *Cursor;
				if (x >= r -> region.left
				 && y >= r -> region.top
				 && area > r -> area
				 && (Cursor = W -> regions[r->linked].Cursor)
				 && x < r -> region.left + r -> region.width
				 && y < r -> region.top + r -> region.height) {
				     LastCursorDrawn = Cursor;
				     area = r -> area;
				 }
 			    }			
			 }
		}
	DrawCursor ();
	CursorVisible = 1;
    }
}

MarkWindowChanged(w)
register struct Window *w; {
    register struct SplitWindow *p;
    w -> t.Changed = 1;
    w -> t.TotallyChanged = 1;
    p = w -> t.parent;
    while (p && !p -> t.Changed) {
	p -> t.Changed = 1;
	p = p -> t.parent;
    }
}

MoveSplit (w, new_x, new_y)
register struct SplitWindow *w;
register    new_x,
            new_y; {
    if (w -> t.type == SplitWindowType) {
	register    xdelt,
	            ydelt;
	if (w -> SplitHorizontally) {
	    xdelt = 0;
	    ydelt = new_y - w -> SplitValue;
	    if (-3 <= ydelt && ydelt <= 3)
		return;
	    w -> SplitValue = new_y;
	}
	else {
	    xdelt = new_x - w -> SplitValue;
	    ydelt = 0;
	    if (-3 <= xdelt && xdelt <= 3)
		return;
	    w -> SplitValue = new_x;
	}
	UnacceptablySmallWindowExists = 0;
	AdjustWindow (w -> top, 0, 0, xdelt, ydelt);
	AdjustWindow (w -> bottom, xdelt, ydelt, -xdelt, -ydelt);
	while (UnacceptablySmallWindowExists) {
	    register    i;
	    UnacceptablySmallWindowExists = 0;
	    for (i = 0; i < NDisplays; i++)
		DeleteNegativeWindow (RootWindow[i]);
	}
    }
}

DeleteNegativeWindow (w)
register struct SplitWindow *w; {
    if (!w -> t.deleted &&
	(w -> t.ViewPort.width < MinWindowWidth || w -> t.ViewPort.height < MinWindowHeight)) {
/* debug (("DeleteNegativeWindow...\n")); */
	DeleteWindow (w);
	UnacceptablySmallWindowExists = 1;
	return 1;
    }
    if (w -> t.type == SplitWindowType)
	return DeleteNegativeWindow (w -> top)
	    || DeleteNegativeWindow (w -> bottom);
    return 0;
}

DeleteWindow (w)
register struct SplitWindow *w; {
    register struct SplitWindow *p,
                               *other;
    if (w && !w -> t.deleted) {
	register    MyWidth = w -> t.ViewPort.width;
	register    MyHeight = w -> t.ViewPort.height;
	int     Visible = 1;
	w -> t.deleted = 1;
	if (w -> t.type == SplitWindowType) {
	    DeleteWindow (w -> top);
	    DeleteWindow (w -> bottom);
	}
	else {
	    if (W -> SubProcess > 0)
		HideProcess (W -> SubProcess);
	    if (W == WindowInFocus) GiveUpInputFocus ();
	    Visible = W -> Visible;
	    W -> Visible = 0;
	}
	if (Visible) {
	    register i;
	    w -> t.display = 0;
	    for (i = 0; i < NDisplays && RootWindow[i] != w; i++) ;
	    if (i < NDisplays) {
		RootWindow[i] = 0;
		if (! shutdown)  {
		    CurrentViewPort = &display.screen;
		    HW_FillTrapezoid(0,0,display.screen.width,0,display.screen.height-1,display.screen.width,gray);
		}
		return;
	    }
	    if ((p = w -> t.parent) == 0) {
		debug (("Orphan window!  (%s)\n", W -> name));
		return;
	    }
	    else
		if (p -> t.deleted)
		    return;
	    if (p -> top == w)
		if (p -> SplitHorizontally)
		    AdjustWindow (other = p -> bottom, 0, -MyHeight, 0, MyHeight);
		else
		    AdjustWindow (other = p -> bottom, -MyWidth, 0, MyWidth, 0);
	    else
		if (p -> SplitHorizontally)
		    AdjustWindow (other = p -> top, 0, 0, 0, MyHeight);
		else
		    AdjustWindow (other = p -> top, 0, 0, MyWidth, 0);
	    w = p;
	    if (p = p -> t.parent)
		if (p -> top == w)
		    p -> top = other;
		else
		    p -> bottom = other;
	    else {
		for (i = 0; i < NDisplays && RootWindow[i] != w; i++);
		if (i < NDisplays)
		    RootWindow[i] = other;
		else
		    debug (("Window not in root list!\n"));
	    }
	    other -> t.parent = p;
	    if (AutomaticEqualization) EqualizeStack (p, 0);
	}
    }
}

AdjustWindow (w, Dl, Dt, Dw, Dh)
register struct Window *w; {
    if (w -> t.type == WindowType)
	SetWindowSize (w,
		w -> t.ViewPort.left + Dl,
		w -> t.ViewPort.top + Dt,
		w -> t.ViewPort.width + Dw,
		w -> t.ViewPort.height + Dh);
    else {
	w -> t.ViewPort.top += Dt;
	w -> t.ViewPort.left += Dl;
	w -> t.ViewPort.width += Dw;
	w -> t.ViewPort.height += Dh;
	if (((struct SplitWindow *) w) -> SplitHorizontally) {
	    if (Dt || Dw)
		AdjustWindow (((struct SplitWindow *) w) -> top, Dl, Dt, Dw, Dh);
	    if (!Dt || Dw)
		AdjustWindow (((struct SplitWindow *) w) -> bottom, Dl, Dt, Dw, Dh);
	}
	else {
	    if (Dl || Dh)
		AdjustWindow (((struct SplitWindow *) w) -> top, Dl, Dt, Dw, Dh);
	    if (!Dl || Dh)
		AdjustWindow (((struct SplitWindow *) w) -> bottom, Dl, Dt, Dw, Dh);
	}
    }
}

SetWindowSize (p, l, t, w, h)
register struct Window *p; {
    switch (p -> t.type) {
	case WindowType: 
	    if (p -> t.ViewPort.top != t ||
		    p -> t.ViewPort.left != l ||
		    p -> t.ViewPort.width != w ||
		    p -> t.ViewPort.height != h) {
		p -> t.ViewPort.top = t;
		p -> t.ViewPort.left = l;
		p -> t.ViewPort.width = w;
		p -> t.ViewPort.height = h;
		if (w < MinWindowWidth || h < MinWindowHeight)
		    UnacceptablySmallWindowExists = 1;
		p -> SubViewPort.top = t + ModeFont -> newline.y + 1 + BorderWidth;
		p -> SubViewPort.left = l + BorderWidth;
		p -> SubViewPort.width = w - 2*BorderWidth;
		p -> SubViewPort.height = h - 1 - 2*BorderWidth - ModeFont -> newline.y;
		p -> BasicSubViewPort = p -> SubViewPort;
		p -> ModeViewPort.top = t + BorderWidth;
		p -> ModeViewPort.left = l + BorderWidth;
		p -> ModeViewPort.width = w - 2*BorderWidth;
		p -> ModeViewPort.height = ModeFont -> newline.y;
		p -> KnowsAboutChange = 0;
		MarkWindowChanged (p);
	    }
	    break;
    }
}

AllocateWindow (p, l, t, w, h)
register struct Window *p; {
    p -> t.type = WindowType;
    p -> t.Changed = 1;
    p -> t.TotallyChanged = 1;
    p -> t.parent = 0;
    p -> Visible = 1;
    p -> t.display = 0;
    SetWindowSize (p, l, t, w, h);
    p -> t.deleted = 0;
}

SplitWindow (w, pos, horizontal, p)
register struct Window *w,
                       *p; {
    register struct SplitWindow *up;
    int     cutabove = horizontal & 2;
    horizontal &= 1;
    up = new (struct SplitWindow);
    up -> t.ViewPort = w -> t.ViewPort;
    if (horizontal) {
	if (pos <= w -> t.ViewPort.top)
	    return 0;
	if (cutabove) {
	    AllocateWindow (p, w -> t.ViewPort.left, w -> t.ViewPort.top, w -> t.ViewPort.width,
		    pos - w -> t.ViewPort.top);
	    AdjustWindow (w, 0, p -> t.ViewPort.height, 0, -p -> t.ViewPort.height);
	}
	else {
	    AllocateWindow (p, w -> t.ViewPort.left, pos, w -> t.ViewPort.width,
		    w -> t.ViewPort.top + w -> t.ViewPort.height - pos);
	    AdjustWindow (w, 0, 0, 0, -p -> t.ViewPort.height);
	}
    }
    else {
	if (pos <= w -> t.ViewPort.left)
	    return 0;
	if (cutabove) {
	    AllocateWindow (p, w -> t.ViewPort.left, w -> t.ViewPort.top,
		    pos - w -> t.ViewPort.left, w -> t.ViewPort.height);
	    AdjustWindow (w, p -> t.ViewPort.width, 0, -p -> t.ViewPort.width, 0);
	}
	else {
	    AllocateWindow (p, pos, w -> t.ViewPort.top,
		    w -> t.ViewPort.left + w -> t.ViewPort.width - pos, w -> t.ViewPort.height);
	    AdjustWindow (w, 0, 0, -p -> t.ViewPort.width, 0);
	}
    }
    up -> t.type = SplitWindowType;
    up -> t.Changed = 1;
    up -> t.TotallyChanged = 1;
    up -> t.deleted = 0;
    up -> SplitValue = pos;
    up -> SplitHorizontally = horizontal;
    up -> t.parent = w -> t.parent;
    w -> t.parent = p -> t.parent = up;
    up -> t.display = p -> t.display = w -> t.display;
    if (cutabove) {
	up -> top = (struct SplitWindow *) p;
	up -> bottom = (struct SplitWindow *) w;
    }
    else {
	up -> top = (struct SplitWindow *) w;
	up -> bottom = (struct SplitWindow *) p;
    }
    if (up -> t.parent)
	if (up -> t.parent -> top == (struct SplitWindow   *) w)
	                                                        up -> t.parent -> top = up;
	else
	    up -> t.parent -> bottom = up;
    else {
	register    i;
	for (i = 0; i < NDisplays && RootWindow[i] != (struct SplitWindow *) w; i++);
	if (i < NDisplays)
	    RootWindow[i] = up;
	else
	    debug (("Can't find root pointer in split\n"));
    }
    while (UnacceptablySmallWindowExists) {
	register    i;
	for (i = 0; i < NDisplays; i++)
	    DeleteNegativeWindow (RootWindow[i]);
	UnacceptablySmallWindowExists = 0;
    }
    return 1;
}

static BestSplitCost;
static BestArea;
static BestSplitPosition;
static BestSplitDirection;
static struct Window *BestWindowToSplit;
static desired_minwidth;
static desired_maxwidth;
static desired_minheight;
static desired_maxheight;

PrintWindowStructure (w, depth)
register struct SplitWindow *w; {
    printf ("%*s", depth * 4, "");
    if (w == 0)
	printf ("NIL\n");
    else {
	printf ("%c %7oa %7op %3dx %3dy %3dw %3dh",
		w -> t.type != SplitWindowType ? ' '
		: w -> SplitHorizontally ? '-' : '|',
		w, w->t.parent,
		w -> t.ViewPort.left,
		w -> t.ViewPort.top,
		w -> t.ViewPort.width,
		w -> t.ViewPort.height);
	if (w -> t.type == SplitWindowType) {
	    printf (" %3ds", w -> SplitValue);
	}
	else {
	    printf (" %3dbx %3dby %3dp",
		    W -> SubViewPort.left, W -> SubViewPort.top,
		    W -> SubProcess);
	}
	printf ("\n");
	if (w -> t.type == SplitWindowType) {
	    PrintWindowStructure (w -> top, depth + 1);
	    PrintWindowStructure (w -> bottom, depth + 1);
	}
    }
}

static struct zoomBoxLine {
    short x1,y1,x2,y2;
} zoomBox[4];

#define zoomSteps 30
#define zoomWidth 3

static DrawZoomBoxLine(p1,p2,scale)
register struct zoomBoxLine *p1,*p2;
register    scale; {
    HW_vector (p1->x1 + (p1->x2 - p1->x1) * scale / zoomSteps,
	    p1->y1 + (p1->y2 - p1->y1) * scale / zoomSteps,
	    p2->x1 + (p2->x2 - p2->x1) * scale / zoomSteps,
	    p2->y1 + (p2->y2 - p2->y1) * scale / zoomSteps);
}

static DrawZoomBox(scale) {
    DrawZoomBoxLine (&zoomBox[0], &zoomBox[1], scale);
    DrawZoomBoxLine (&zoomBox[1], &zoomBox[2], scale);
    DrawZoomBoxLine (&zoomBox[2], &zoomBox[3], scale);
    DrawZoomBoxLine (&zoomBox[3], &zoomBox[0], scale);
}

CreateWindow (w)
register struct Window *w; {
    register    i;
    register struct display *d;
    for (i = 0; i < NDisplays && RootWindow[i]; i++);
    debug (("CreateWindow: i=%d, NDisplays=%d\n", i, NDisplays));
    if (i < NDisplays) {
	d = &displays[i];
	AllocateWindow (w, d->screen.left, d->screen.top, d -> screen.width, d -> screen.height);
	RootWindow[i] = (struct SplitWindow *) w;
	w -> t.display = d;
	debug (("window %o on display %o\n", w, d));
	MarkWindowChanged (w);
    }
    else {
	BestSplitCost = 0;
	BestArea = 0;
	desired_minwidth = w -> minwidth;
	desired_maxwidth = w -> maxwidth;
	desired_minheight = w -> minheight;
	desired_maxheight = w -> maxheight;
	BestWindowToSplit = 0;
	for (i = 0; i < NDisplays; i++)
	    FindWindowToSplit (RootWindow[i]);
	debug (("window %o created by splitting %o\n", w, BestWindowToSplit));
	if (BestWindowToSplit == 0
		|| SplitWindow (BestWindowToSplit,
		    BestSplitPosition, BestSplitDirection, w) == 0)
	    debug (("  No window to split\n"));
    }
    if (AutomaticEqualization)
	EqualizeStack (w, 0);
    if (zoomY > 0 || AlwaysZoom) {
	register struct ViewPort   *ovp = CurrentViewPort;
	register    i;
	if (CurrentDisplay != w -> t.display) {
	    if (w -> t.display == 0) {
		debug (("Null display pointer in window %s\n", w -> name));
		return;
	    }
	    CurrentDisplay = w -> t.display;
	    display = *CurrentDisplay;
	}
	CurrentViewPort = &display.screen;
	HW_SelectColor (f_invert);
	if (zoomY <= 0) {
	    zoomX = w -> t.ViewPort.left + (w -> t.ViewPort.width>>1);
	    zoomY = w -> t.ViewPort.top + (w -> t.ViewPort.height>>1);
	    zoomW = zoomH = 0;
	}
	zoomBox[0].x1 = zoomX;
	zoomBox[0].y1 = zoomY;
	zoomBox[0].x2 = w -> t.ViewPort.left;
	zoomBox[0].y2 = w -> t.ViewPort.top;
	zoomBox[1].x1 = zoomX + zoomW;
	zoomBox[1].y1 = zoomY;
	zoomBox[1].x2 = w -> t.ViewPort.left + w -> t.ViewPort.width;
	zoomBox[1].y2 = w -> t.ViewPort.top;
	zoomBox[2].x1 = zoomX + zoomW;
	zoomBox[2].y1 = zoomY + zoomH;
	zoomBox[2].x2 = w -> t.ViewPort.left + w -> t.ViewPort.width;
	zoomBox[2].y2 = w -> t.ViewPort.top + w -> t.ViewPort.height;
	zoomBox[3].x1 = zoomX;
	zoomBox[3].y1 = zoomY + zoomH;
	zoomBox[3].x2 = w -> t.ViewPort.left;
	zoomBox[3].y2 = w -> t.ViewPort.top + w -> t.ViewPort.height;
	for (i = 0; i <= zoomSteps; i++) {
	    DrawZoomBox (i);
	    if (i >= zoomWidth)
		DrawZoomBox (i - zoomWidth);
	}
	i -= zoomWidth;
	while (i <= zoomSteps)
	    DrawZoomBox (i++);
	CurrentViewPort = ovp;
    }
}

SelectWindow (w)
register struct Window *w; {
    CurrentWindow = w;
    CurrentViewPort = &CurrentWindow -> SubViewPort;
}

static
TrySplit (size, orthosize, minsize, maxsize, orthomaxsize, dminsize, dmaxsize, orthodmaxsize)
register    size; {
    register    T;
    int     area;
/*  T = (maxsize - dmaxsize + size) >> 1; */
    T = size * maxsize / (maxsize + dmaxsize);
    if (T < minsize)
	T = minsize;
    if (T > maxsize)
	T = maxsize;
    if (size - T < dminsize)
	T = size - dminsize;
    if (size - T > dmaxsize)
	T = size - dmaxsize;
    if (T >= minsize && T < size) {
	register    e1;
	register    newsize = size - T;
	area = size * orthosize;
	if (newsize >= dmaxsize)
	    if (orthosize >= orthodmaxsize)
		e1 = (4 << 28) - area;
	    else
		e1 = (3 << 28) + orthosize;
	else
	    if (T >= maxsize)
		if (orthosize >= orthomaxsize)
		    e1 = (area << 2) + 3;
		else
		    e1 = (area << 2) + 2;
	    else {
		e1 = area << 2;
		if (size > orthosize)
		    e1++;
	    }
	if (e1 > BestSplitCost	/* || e1==BestSplitCost && area>BestArea 
		 */ ) {
	    BestSplitCost = e1;
	    BestArea = area;
	    BestSplitPosition = T;
	    return 1;
	}
    }
    return 0;
}

/* find a window to split using a least-squares fit to minimize the deviation
   from nominal window heights */
FindWindowToSplit (w)
register struct Window *w; {
    switch (w -> t.type) {
	case SplitWindowType: 
	    FindWindowToSplit (((struct SplitWindow *) w) -> top);
	    FindWindowToSplit (((struct SplitWindow *) w) -> bottom);
	    break;
	case WindowType: 
	    {
		if (TrySplit (w -> t.ViewPort.height, w -> t.ViewPort.width,
			    w -> minheight, w -> maxheight, w -> maxwidth,
			    desired_minheight, desired_maxheight, desired_maxwidth)) {
		    BestSplitPosition += w -> t.ViewPort.top;
		    BestSplitDirection = 1;
		    BestWindowToSplit = w;
		}
		if (TrySplit (w -> t.ViewPort.width, w -> t.ViewPort.height,
			    w -> maxwidth < MinWidth || w -> minwidth > MinWidth
					? w -> minwidth : MinWidth,
				w -> maxwidth, w -> maxheight,
			    w -> maxwidth < MinWidth || desired_minwidth > MinWidth
					? desired_minwidth : MinWidth,
				desired_maxwidth, desired_maxheight)) {
		    BestSplitPosition += w -> t.ViewPort.left;
		    BestSplitDirection = 0;
		    BestWindowToSplit = w;
		}
	    }
	    break;
    }
}

RedrawWindows (w)
register struct SplitWindow *w; {
    if (w -> t.Changed) {
	switch (w -> t.type) {
	    case SplitWindowType: 
		RedrawWindows (w -> top);
		RedrawWindows (w -> bottom);
		break;
	    case WindowType: 
		if (w -> t.TotallyChanged) {
		    DrawWindowFrame (w);
		    w -> t.TotallyChanged = 0;
		}
		break;
	}
	w -> t.Changed = 0;
    }
}

DrawWindowFrame(w)
register struct Window *w; {
    register struct ViewPort   *ovp = CurrentViewPort;
    register    x2,
                y2;
    register    TopCorner = BorderWidth - BlackEdgeWidth;
    CurrentViewPort = &w -> t.ViewPort;
    if (EmphasizedWindow == w && BorderStyle != 0)
	EmphasizedWindow = 0;
    if (CurrentDisplay != w -> t.display) {
	if (w -> t.display == 0) {
	    debug (("Null display pointer in window %s\n", w -> name));
	    return;
	}
	CurrentDisplay = w -> t.display;
	display = *CurrentDisplay;
    }
    x2 = w -> t.ViewPort.width - BorderWidth;
    y2 = w -> t.ViewPort.height - BorderWidth;
    if (BorderStyle) {
	extern NOP();
	if (display.d_FillTrapezoid != NOP) {
	HW_SelectColor (f_BlackOnWhite | f_CharacterContext);
	HW_FillTrapezoid (0,0,w -> t.ViewPort.width,0,BorderWidth-1,w -> t.ViewPort.width,gray);
	HW_FillTrapezoid (0,y2,w -> t.ViewPort.width,0,y2+BorderWidth-1,w -> t.ViewPort.width,gray);
	HW_FillTrapezoid (0,0,BorderWidth,0,w -> t.ViewPort.height-1,BorderWidth,gray);
	HW_FillTrapezoid (x2,0,BorderWidth,x2,w -> t.ViewPort.height-1,BorderWidth,gray);
	} else {
	HW_SelectColor (f_white);
	HW_RasterSmash (0, 0, w -> t.ViewPort.width, BorderWidth);
	HW_RasterSmash (0, y2, w -> t.ViewPort.width, BorderWidth);
	HW_RasterSmash (0, 0, BorderWidth, w -> t.ViewPort.height);
	HW_RasterSmash (x2, 0, BorderWidth, w -> t.ViewPort.height);
	}
    }
    HW_SelectColor (f_black);
    {
	register    SizeDelta = 2 * (BorderWidth - BlackEdgeWidth);
	HW_RasterSmash (TopCorner, TopCorner, w -> t.ViewPort.width - SizeDelta, BlackEdgeWidth);
	HW_RasterSmash (TopCorner, y2, w -> t.ViewPort.width - SizeDelta, BlackEdgeWidth);
	HW_RasterSmash (TopCorner, TopCorner, BlackEdgeWidth, w -> t.ViewPort.height - SizeDelta);
	HW_RasterSmash (x2, TopCorner, BlackEdgeWidth, w -> t.ViewPort.height - SizeDelta);
    }
    y2 = ModeFont -> newline.y + BorderWidth;
    HW_vector (TopCorner, y2, x2, y2);
    DrawModeLine (w);
    if (w -> SubProcess >= 0) {
	if (!w -> KnowsAboutChange) {
	    send (w -> SubProcess, "R", 1, 1 /* SOF_OOB */ );
	    w -> KnowsAboutChange = 1;
	}
	else
	    debug (("Omitted sending redraw signal to %s\n", w -> name));
    }
    CurrentViewPort = ovp;
}

DrawModeLine (w)
register struct Window *w; {
    if (w) {
	register struct ViewPort   *ovp = CurrentViewPort;
	int     xo = ModeFont -> NWtoOrigin.x;
	int     yo = ModeFont -> NWtoOrigin.y;
	CurrentViewPort = &w -> ModeViewPort;
	if (EmphasizedWindow ==  w && BorderStyle == 0)
	    EmphasizedWindow = 0;
	HW_SelectColor (f_white);
	HW_RasterSmash (0, 0, w -> t.ViewPort.width, w -> t.ViewPort.height);
	HW_SelectColor (f_black | f_CharacterContext);
	if (w -> SubProcess >= 0) {
	    register    LeftSpace = (w -> ModeViewPort.width - w -> NameWidth) >> 1;
	    register    HostWidth = StringWidth (ModeFont, w -> hostname);
	    register    ProgWidth = w -> ProgramName[0] ? StringWidth (ModeFont, w -> ProgramName) : 999999;
	    if (LeftSpace > ProgWidth)
		HW_DrawString (xo, yo, ModeFont, w -> ProgramName, 0, 0, 0);
	    HW_DrawString (xo + LeftSpace, yo, ModeFont, w -> name, 0, 0, 0);
	    if (LeftSpace - xo > HostWidth)
		HW_DrawString (w -> ModeViewPort.width - HostWidth - 2, yo, ModeFont, w -> hostname, 0, 0, 0);
	}
	else
	    HW_DrawString (xo, yo, ModeFont, " No attached process!", 0, 0, 0);
	CurrentViewPort = ovp;
	if (WindowInFocus == w)
	    SetInputFocus (w);
    }
}

FlipEmphasis (w)
register struct Window *w; {
    if (w && w -> Visible) {
	register struct ViewPort   *ovp = CurrentViewPort;
	register t;
	if (CurrentDisplay != w -> t.display) {
	    if (w -> t.display == 0) {
		debug (("Null display pointer in window %s\n", w -> name));
		return;
	    }
	    CurrentDisplay = w -> t.display;
	    display = *CurrentDisplay;
	}
	HW_SelectColor (f_invert);
	switch (FocusHighlightStyle) {
	    case 1: 
		CurrentViewPort = &w -> ModeViewPort;
		HW_RasterSmash (0, 0, w -> t.ViewPort.width, w -> t.ViewPort.height);
		break;
	    case 2: 
		CurrentViewPort = &w -> t.ViewPort;
		t = BorderWidth-BlackEdgeWidth;
		HW_RasterSmash (0, 0, w -> t.ViewPort.width, t);
		HW_RasterSmash (0, w -> t.ViewPort.height - t, w -> t.ViewPort.width, t);
		HW_RasterSmash (0, 0, t, w -> t.ViewPort.height);
		HW_RasterSmash (w -> t.ViewPort.width - t, 0, t, w -> t.ViewPort.height);
		break;
	}
	CurrentViewPort = ovp;
    }
}

#define FocusStackSize 10
static struct Window *FocusStack[FocusStackSize];
static int FocusStackFill;

SetInputFocus (w)
register struct Window *w; {
    if (w && w -> Visible) {
	register struct display *OldDisplay = CurrentDisplay;
	if (w != EmphasizedWindow) {
	    if (EmphasizedWindow)
		FlipEmphasis (EmphasizedWindow);
	    FlipEmphasis (w);
	    EmphasizedWindow = w;
	}
	if (WindowInFocus != w) {
	    FocusStack[FocusStackFill++] = WindowInFocus;
	    if (FocusStackFill >= FocusStackSize)
		FocusStackFill = 0;
	    WindowInFocus = w;
	}
	if (CurrentDisplay != OldDisplay) {
	    CurrentDisplay = OldDisplay;
	    display = *CurrentDisplay;
	}
    }
}

GiveUpInputFocus () {
    register struct Window *w;
    register    n = FocusStackSize;
    while (--n >= 0) {
	if (--FocusStackFill < 0)
	    FocusStackFill = FocusStackSize - 1;
	if (w = FocusStack[FocusStackFill]) {
	    FocusStack[FocusStackFill] = 0;
	    if (w != WindowInFocus && w -> Visible) {
		SetInputFocus (w);
		return;
	    }
	}
    }
    for (w = &WindowChannel[HighestDescriptor]; w >= WindowChannel; w--)
	if (w -> SubProcess > 0 && w -> Visible) {
	    SetInputFocus (w);
	    return;
	}
}

struct font *iconfont;

InitializeWindowSystem () {
    InitializeMenuSystem();
    iconfont = getpfont ("icon12");
    CursorDisplay = &displays[0];
    AutomaticEqualization = getprofileswitch ("equalize", 0);
    FocusFollowsCursor = getprofileswitch ("FocusFollowsCursor", 0);
    AlwaysZoom = getprofileswitch ("AlwaysZoom", 0);
    MinWidth = getprofileint ("MinWidth", 100);
    if (ModeFont == 0) {
	char   *s = getprofile ("modefont");
	if (s == 0 || (ModeFont = getpfont (s)) == 0)
	    ModeFont = getpfont ("cmr12");
    }
    if (iconfont == 0 || ModeFont == 0) {
	printf ("Can't initialize fonts.\n");
	exit (1);
    }
    DefaultCursor = geticon (iconfont, 'a');
    HorizontalCursor = geticon (iconfont, 'h');
    VerticalCursor = geticon (iconfont, 'v');
    RightFingerCursor = StackedMenus? DefaultCursor: geticon (iconfont, 'f');
    UpperLeftCursor = geticon (iconfont, 'u');
    LowerRightCursor = geticon (iconfont, 'l');
    BorderStyle = getprofileint ("borderstyle", 5);
    if (BorderStyle < 0 || BorderStyle > 6)
	BorderStyle = 0;
    BlackEdgeWidth = 1;
    BorderWidth = BorderStyle+1;
    if (BorderStyle>2) BlackEdgeWidth++;
    FocusHighlightStyle = getprofileint ("focushighlightstyle", 1);
}
