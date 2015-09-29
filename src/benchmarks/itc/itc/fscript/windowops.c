/*****************************************\
* 					  *
* 	File: windowops.c		  *
* 	Copyright (c) 1984 IBM		  *
* 	Date: Sun Mar 25 18:06:39 1984	  *
* 	Author: James Gosling		  *
* 					  *
* Commands to manipulate windows.	  *
* 					  *
* HISTORY				  *
* 					  *
\*****************************************/

#include "stdio.h"
#include "menu.h"
#include "window.h"
#include "display.h"

static ReshapeOnExpose;
struct display *SnapShotDisplay = 0;
int shutdown = 0;

Exit () {
    register i;
    register struct Window *w = WindowChannel;

    shutdown = 1;
    for (i = 0; i <= HighestDescriptor; i++, w++)
	if (w -> SubProcess > 0) KillWindow (w);
    {	register i;
	register struct icon *fill = geticon (shapefont, 'F');
	for (i = 0; i<NDisplays; i++) {
	    register struct display *d = &displays[i];
	    if (d->d_FillTrapezoid) {
		display = *d;
		CurrentDisplay = d;
		CurrentViewPort = &display.screen;
		HW_SelectColor (f_BlackOnWhite|f_CharacterContext);
		HW_FillTrapezoid (0,0,display.screen.width,0,display.screen.height-1,display.screen.width,fill);
	    }
	}
    }
    CleanupScreen ();
    printf ("\033[33f\033[JToodles...\n");
    exit (1);
}

static
Reshape () {
    if (CursorWindow && CursorWindow -> t.type != SplitWindowType
	    && CursorWindow -> SubProcess > 0) {
	Reshapee = CursorWindow;
	Reshapee_x = -1;
	DeleteWindow (CursorWindow);
    }
}

static    int     left,
            right,
            top,
            bottom;

static struct SplitWindow *FindContainer (w)
register struct SplitWindow *w; {
    if (w) {
	if (w -> t.ViewPort.top > top
		|| w -> t.ViewPort.left > left
		|| w -> t.ViewPort.top + w -> t.ViewPort.height <= bottom
		|| w -> t.ViewPort.left + w -> t.ViewPort.width <= right)
	    return 0;
	if (w -> t.type == SplitWindowType) {
	    register struct SplitWindow *tw;
	    if ((tw = FindContainer (w -> top)) || (tw = FindContainer (w -> bottom)))
		return tw;
	}
    }
    return w;
}

FinishReshape (x, y) {
    register struct SplitWindow *target;
    if (x < Reshapee_x) {
	left = x;
	right = Reshapee_x;
    }
    else {
	left = Reshapee_x;
	right = x;
    }
    if (y < Reshapee_y) {
	top = y;
	bottom = Reshapee_y;
    }
    else {
	top = Reshapee_y;
	bottom = y;
    }
    /* debug (("reshape: top=%d, bottom=%d, left=%d, right=%d\n", top, bottom, left, right)); */
    if (target = FindContainer (RootWindow[CursorDisplayNumber])) {
	int     horizontal,
	        cutabove = 0,
	        pos;
	if ((bottom - top) * 1000 / target -> t.ViewPort.height
		> (right - left) * 1000 / target -> t.ViewPort.width) {
	    horizontal = 0;
	    if (left - target -> t.ViewPort.left
		    < target -> t.ViewPort.left + target -> t.ViewPort.width - right)
		pos = right, cutabove = 2;
	    else
		pos = left;
	}
	else {
	    horizontal = 1;
	    if (top - target -> t.ViewPort.top
		    < target -> t.ViewPort.top + target -> t.ViewPort.height - bottom)
		pos = bottom, cutabove = 2;
	    else
		pos = top;
	}
	if (!SplitWindow (target, pos, cutabove | horizontal, Reshapee))
	    debug (("Window recreation failed pos=%d, horizontal=%d, window=%o\n", pos, horizontal, target));
    }
    else
	debug (("Couldn't find target\n"));
    ShowProcess (Reshapee, 1);
    Reshapee = 0;
    Reshapee_x = -1;
}

static
Hide () {
    DeleteWindow (CursorWindow);
}

static
Kill () {
    if (CursorWindow && CursorWindow -> t.type != SplitWindowType) {
	KillProcess (CursorWindow -> SubProcess);
	DeleteWindow (CursorWindow);
    }
}

MarkTotallyChanged  (w)
register struct SplitWindow *w; {
    if (w) {
	w -> t.Changed = 1;
	switch (w -> t.type) {
	    case SplitWindowType: 
		MarkTotallyChanged (w -> top);
		MarkTotallyChanged (w -> bottom);
		break;
	    case WindowType: 
		w -> t.TotallyChanged = 1;
		((struct Window *) w) -> KnowsAboutChange = 0;
		break;
	}
    }
}

RedrawTotalDisplay () {
    register    i;
    for (i = 0; i < NDisplays; i++)
	MarkTotallyChanged (RootWindow[i]);
}

#define W ((struct Window *) w)

FindWindowSize (w, horizontal, min)
register struct SplitWindow *w; {
    if (w) {
	switch (w -> t.type) {
	    case SplitWindowType: 
		{
		    register    t1 = FindWindowSize (w -> top, horizontal, min);
		    register    t2 = FindWindowSize (w -> bottom, horizontal, min);
		    if (w -> SplitHorizontally == horizontal)
			if (t1 > t2)
			    return t1;
			else
			    return t2;
		    else
			return t1 + t2;
		}
		break;
	    case WindowType: 
		return min ? horizontal ? W -> minwidth : W -> minheight
			   : horizontal ? W -> maxwidth : W -> maxheight;
		break;
	}
    }
    return 0;
}

ShovelAsideFrom (child)
register struct SplitWindow *child; {
    register struct SplitWindow *w;
    register    pos;
    if (child == 0 || (w = child -> t.parent) == 0)
	return;
    ShovelAsideFrom (w);
    pos = child != w -> top
	? FindWindowSize (w -> top, !w -> SplitHorizontally, 1)
		+ (w -> SplitHorizontally ? w -> t.ViewPort.top : w -> t.ViewPort.left)
	: (w -> SplitHorizontally ? w -> t.ViewPort.top + w -> t.ViewPort.height
			   : w -> t.ViewPort.left + w -> t.ViewPort.width)
		- FindWindowSize (w -> bottom, !w -> SplitHorizontally, 1);
    MoveSplit (w, pos, pos);
}

static
ExpandWindow () {
    if (CursorWindow && CursorWindow -> t.type != SplitWindowType)
	ShovelAsideFrom (CursorWindow);
}

static SmallSum, LargeSum, HeightThreshhold;

static SumWindowStackSizes (w, horizontal)
register struct SplitWindow *w; {
    if (w -> t.type != SplitWindowType || w -> SplitHorizontally != horizontal) {
	register    height = FindWindowSize (w, !horizontal, 0);
	if (height < HeightThreshhold)
	    SmallSum += height;
	else
	    LargeSum += height;
    }
    else {
	SumWindowStackSizes (w -> top, horizontal);
	SumWindowStackSizes (w -> bottom, horizontal);
    }
}

static
AssignEqualizedSpace (w, horizontal, DoSmall)
register struct SplitWindow *w; {
/* debug (("Begin AssignEqualizedSpace pos=%d\n", SmallSum)); */
    if (w -> t.type == SplitWindowType && w -> SplitHorizontally == horizontal) {
	AssignEqualizedSpace (w -> top, horizontal, DoSmall);
/* debug (("  Moving with pos=%d\n", SmallSum)); */
	MoveSplit (w, SmallSum, SmallSum);
	AssignEqualizedSpace (w -> bottom, horizontal, DoSmall);
    } else {
	int dsize = FindWindowSize (w, !horizontal, 0);
	SmallSum += dsize < HeightThreshhold && DoSmall ? dsize : dsize*HeightThreshhold/LargeSum;
/* debug (("  Sum at %d,%d = %d (ds=%d,ht=%d)\n", w -> t.ViewPort.left,w -> t.ViewPort.top, SmallSum,dsize,HeightThreshhold)); */
    }
/* debug (("End\n")); */
}

EqualizeStack (w, AdjustVerticals)
register struct SplitWindow *w; {
    register struct SplitWindow *p;
    register    DoSmall;
    if (w == 0)
	return;
    if (w -> t.type != SplitWindowType)
	w = w -> t.parent;
    if (w && (w -> SplitHorizontally || AdjustVerticals)) {
	while ((p = w -> t.parent) && p -> SplitHorizontally == w -> SplitHorizontally)
	    w = p;
	SmallSum = 0;
	LargeSum = 0;
	HeightThreshhold = w -> SplitHorizontally ? w -> t.ViewPort.height : w -> t.ViewPort.width;
	SumWindowStackSizes (w, w -> SplitHorizontally);
	if (SmallSum > (HeightThreshhold >> 1)) {
	    LargeSum += SmallSum;
	    DoSmall = 0;
	}
	else {
	    HeightThreshhold -= SmallSum;
	    DoSmall = 1;
	}
	SmallSum = w -> SplitHorizontally ? w -> t.ViewPort.top : w -> t.ViewPort.left;
	AssignEqualizedSpace (w, w -> SplitHorizontally, DoSmall);
    }
}

StraightenOut (w, ParentsDirection)
register struct SplitWindow *w; {
    if (w && w -> t.type == SplitWindowType) {
	if (w -> SplitHorizontally != ParentsDirection)
	    EqualizeStack (w, 1);
	StraightenOut (w -> top, w -> SplitHorizontally);
	StraightenOut (w -> bottom, w -> SplitHorizontally);
    }
}

StraightenAllOut () {
    register    i;
    for (i = 0; i < NDisplays; i++)
	StraightenOut (RootWindow[i], 2);
}


static Expose (name)
char   *name; {
    register    fd;
    register struct Window *w = WindowChannel;
    for (fd = 0; fd < sizeof WindowChannel / sizeof WindowChannel[0]; fd++, w++)
	if (w -> MenuTitle[0] == name[0] && strcmp (w -> MenuTitle, name) == 0) {
	    w -> AcquireFocusOnExpose = AcquireFocusOnExpose;
	    if (ReshapeOnExpose) {
		Reshapee = w;
		Reshapee_x = -1;
	    }
	    else
		ShowProcess (w, 1);
	}
}

FiddleExposeMenu(program, add)
char *program;
int add;
{
    char menustring[100];
    strcpy(menustring,"Expose,");
    strcat(menustring,program);
    AddMenuTranslated(0, menustring, add? Expose: 0);
}

SetWindowTitle (w, name, progname)
register struct Window *w;
char   *name,
       *progname; {
    register struct Window *p;
    char    buf[200];
    int     seq = 1;
    char   *fname,
           *fpname,
	   *DigitStart = 0;
    int     lenn,
            lenp;
    if (name && !progname && !*w->ProgramName) {
	progname = name;
	name = 0;
    }
    else if (progname && *w->ProgramName && !name && !*w->name) {
	name = w->ProgramName;
    }
    if (name) {
	strncpy (w -> name, name, sizeof w -> name);
	w -> name[sizeof w -> name - 1] = 0;
    }
    if (progname) {
	strncpy (w -> ProgramName, progname, sizeof w -> ProgramName);
	w -> ProgramName[sizeof w -> ProgramName - 1] = 0;
    }
    fname = w -> name;
    fpname = w -> ProgramName;
    if (lenp = strlen (fpname))
	lenp++;
    if (lenp >= sizeof w -> MenuTitle / 2) {
	fpname += lenp >> 1;
	lenp -= lenp >> 1;
    }
    lenn = strlen (fname);
    while (seq < 20) {
	if (lenp + lenn >= sizeof w -> MenuTitle) {
	    register char  *s = fname + sizeof w -> MenuTitle - lenp + 1;
	    fname = s;
	    while (*s) {
		if (*s == ' ' || *s == '/') {
		    while (*++s == ' ');
		    if (*s)
			fname = s;
		    break;
		}
		s++;
	    }
	}
	sprintf (buf, fpname[0] ? "%s %s" : "%s%s", fpname, fname);
	buf[sizeof w -> MenuTitle - 1] = 0;
	for (p = &WindowChannel[HighestDescriptor]; p >= WindowChannel; p--)
	    if (p != w && p -> SubProcess > 0 && strcmp (p -> MenuTitle, buf) == 0)
		break;
	if (p < WindowChannel)
	    break;
	if (DigitStart == 0)
	    if (w -> name[0] == 0) DigitStart = w -> name;
	    else
		if (lenn + 3 < sizeof w -> name) {
		    DigitStart = w -> name + lenn;
		    *DigitStart++ = '-';
		} else DigitStart = w -> name + sizeof w -> name - 3;
	sprintf (DigitStart, "%d", ++seq);
	lenn = strlen (fname);
    }
    if (w->Hidden)
    	FiddleExposeMenu (w->MenuTitle, 0);
    strcpy (w -> MenuTitle, buf);
    w -> NameWidth = StringWidth (ModeFont, w -> name);
    if (w -> Hidden)
	FiddleExposeMenu (w->MenuTitle, 1);
}

ShowProcess (w, SendSig)
register struct Window *w; {
    w -> Hidden = 0;
    debug (("Sent OOB for reshow to %s\n", w -> name));
    send (w -> SubProcess, "E", 1, 1 /* SOF_OOB */ );
    w -> KnowsAboutChange = 1;
    FiddleExposeMenu (w->MenuTitle, 0);
}

KillWindow (w)
register struct Window *w; {
    if (w) {
	DeleteWindow (w);
	if (w -> SubProcess >= 0) {
	    send (w -> SubProcess, "K", 1, 1 /* SOF_OOB */);
	    close (w -> SubProcess);
	    ZapRegionsForProc (w -> SubProcess);
	    debug (("  closing %s  errno=%d\n", w -> name, errno));
	    ValidDescriptors &= ~(1 << w -> SubProcess);
	    FiddleExposeMenu(w->MenuTitle, 0);
	    w -> SubProcess = -1;
	}
    }
}

HideProcess (fd) {
    if (fd > 0) {
	register struct Window *w = &WindowChannel[fd];
	debug (("Hiding %d\n", fd));
	if (!w -> Hidden) {
	    if (w -> SubProcess >= 0) {
		send (w -> SubProcess, "H", 1, 1 /* SOF_OOB */);
		w -> KnowsAboutChange = 1;
	    }
	    w -> Hidden = 1;
	    FiddleExposeMenu(w->MenuTitle, 1);
	}
    }
}

SnapShot () {
    if (display.d_SnapShot)
/*	(*display.d_SnapShot) ();  */
	SnapShotDisplay = &display;
}

struct menu *ManagersMenu () {
    if (!LManagersMenu) {
	extern Eshell ();
	struct menu *AllocateMenu();
	ReshapeOnExpose = getprofileswitch("ReshapeOnExpose", 0);
	LManagersMenu = AllocateMenu("WINDOW");
	AddMenuTranslated (0, "Reshape", Reshape);
	AddMenuTranslated (0, "Hide", Hide);
	AddMenuTranslated (0, "Expose,dildo", Hide);
	AddMenuTranslated (0, "Expose,dildo", 0);
	AddMenuTranslated (0, "Expand", ExpandWindow);
	AddMenuTranslated (0, "Arrange windows nicely", StraightenAllOut);
	AddMenuTranslated (0, "Make program go away", Kill);
	AddMenuTranslated (0, "Start new Shell", Eshell);
	AddMenuTranslated (0, "Redraw display", RedrawTotalDisplay);
	AddMenuTranslated (0, "Snapshot display", SnapShot);
	AddMenuTranslated (0, "Shutdown Everything", Exit);
    }
    return LManagersMenu;
}
