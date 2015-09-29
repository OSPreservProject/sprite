/* Mouse dispatching for the simple window manager */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/


#include "stdio.h"
#include "sys/ioctl.h"
#include "sgtty.h"
#include "framebuf.h"
#include "window.h"
#include "menu.h"
#include "display.h"
#include "usergraphics.h"


struct display *CursorDisplay;
int CursorDisplayNumber;

int  MouseX = 400;
int  MouseY = 400;
static  MouseState = 0;
static  MouseButtons;
static  OldButtons;
struct SplitWindow  *MovingBar;

InitializeMouse () {
    struct display *dp = &displays[0];

    MouseX = dp->screen.width/2;
    MouseY = dp->screen.height/2;
}

SampleMouse (fd) {
    register struct Window *w;
    register    Button;
    int     nleft;
    int     ButtonInteresting;
    char    ibuf[48];	/* should be divisible by 3 */
    register char  *p;
    static short    OldMouseButtons;
    short  *OldMouseButtonsP;
    int     RealMouseX,
            RealMouseY;
    nleft = read (fd, ibuf, sizeof ibuf);
    p = ibuf;
/*
    if (nleft % 3 != 0) debug(("Mouse items %d\n",nleft));
*/
    while (--nleft >= 0) {
/*
	debug(("Mouse: state %d token %o\n",MouseState,*p));
*/
	switch (MouseState) {
	    register incr;

      	    case 0: 
		MouseState++;
		if ((*p++ & 0370) == 0200)
		    MouseButtons = (p[-1] & 7) ^ 7;
		break;
	    case 1: 
		MouseState++;
		incr = *p++;
		MouseX += incr < 0 ? (incr << 1) + 1 : incr ? (incr << 1) - 1 : incr;
		break;
	    case 2: 
		MouseState = 0;
		incr = *p++;
		MouseY -= incr < 0 ? (incr << 1) + 1 : incr ? (incr << 1) - 1 : incr;
		break;
	}
    }
    if (MouseState != 0) return;
    if (MouseX < 0)
	if (CursorDisplayNumber == 0 && NDisplays == 2) {
	    CursorDown ();
	    CursorDisplayNumber = 1;
	    CursorDisplay = &displays[1];
	    CurrentDisplay = CursorDisplay;
	    display = *CurrentDisplay;
	    MouseX += display.screen.width;
	    MouseY = MouseY * displays[1].screen.height / displays[0].screen.height;
	}
	else
	    MouseX = 0;
    if (MouseX >= display.screen.width)
	if (CursorDisplayNumber == 1 && NDisplays == 2) {
	    CursorDown ();
	    CursorDisplayNumber = 0;
	    CursorDisplay = &displays[0];
	    CurrentDisplay = CursorDisplay;
	    MouseX -= display.screen.width;
	    MouseY = MouseY * displays[0].screen.height / displays[1].screen.height;
	    display = *CurrentDisplay;
	}
	else
	    MouseX = display.screen.width - 1;
    if (MouseY < 0)
	MouseY = 0;
    if (MouseY >= display.screen.height)
	MouseY = display.screen.height - 1;
    MoveCursor (MouseX, MouseY);
    if (w = CursorWindow) {
	OldMouseButtonsP = w -> t.type == SplitWindowType
	    ? &OldMouseButtons
	    : &w -> OldMouseButtons;
	if (ButtonInteresting = (MouseButtons | *OldMouseButtonsP)) {
	    if ((ButtonInteresting & 5) && w -> SubProcess > 0 && w -> t.type != SplitWindowType) {
		register    ActionMask = 0;
		if (!FocusFollowsCursor && WindowInFocus != w && !w -> IHandleAquisition) {
		    if ((MouseButtons & 5) == 0 && !w -> InputDisabled)
			SetInputFocus (w);
		}
		else
		    if (w -> MouseInterest) {
			if (ButtonInteresting & (1 << RightButton))
			    ActionMask = MouseMask (RightButton,
				    (((*OldMouseButtonsP >> RightButton) & 1) << 1)
				    + ((MouseButtons >> RightButton) & 1));
			if (ButtonInteresting & (1 << LeftButton))
			    ActionMask |= MouseMask (LeftButton,
				    (((*OldMouseButtonsP >> LeftButton) & 1) << 1)
				    + ((MouseButtons >> LeftButton) & 1));
			if (w -> MouseInterest & ActionMask) {
			    register    t1;
			    if ((ActionMask & (MouseMask (LeftButton, DownTransition) | MouseMask (LeftButton, UpTransition) | MouseMask (RightButton, DownTransition) | MouseMask (RightButton, UpTransition)))
				    || ((t1 = MouseX - w -> OldMouseX) >= 0 ? t1 : -t1) >= w -> MouseMotionGranularity
				    || ((t1 = MouseY - w -> OldMouseY) >= 0 ? t1 : -t1) >= w -> MouseMotionGranularity
				) {
				GR_SEND (w, GR_MOUSECHANGE, w -> MouseInterest & ActionMask,
					MouseX - CursorWindow -> SubViewPort.left + display.screen.left,
					MouseY - CursorWindow -> SubViewPort.top + display.screen.top);
				w -> OldMouseX = MouseX;
				w -> OldMouseY = MouseY;
			    }
			}
		    }
	    }
	    RealMouseX = MouseX + display.screen.left;
	    RealMouseY = MouseY + display.screen.top;
	    if (ButtonInteresting & 2) {
		if (MovingBar) {
		    if ((MouseButtons & 2) == 0) {
			MoveSplit (MovingBar, RealMouseX, RealMouseY);
			MovingBar = 0;
		    }
		}
		else
		    if (Reshapee) {
			if (Reshapee_x < 0) {
			    Reshapee_x = RealMouseX;
			    Reshapee_y = RealMouseY;
			}
			else
			    if ((MouseButtons & 2) == 0)
				FinishReshape (RealMouseX, RealMouseY);
		    }
		    else
			if (MenuActive || w -> t.type != SplitWindowType)
			    LisaStyleMenu (MouseX, MouseY,
				    (*OldMouseButtonsP & 2) + ((MouseButtons >> 1) & 1), 1);
			else
			    if ((*OldMouseButtonsP & 2) == 0)
				MovingBar = (struct SplitWindow *) w;
	    }
	}
	else
	    if (MenuActive) {
		LisaStyleMenu (MouseX, MouseY,
			(OldMouseButtons & 2) + ((MouseButtons >> 1) & 1), 1);
	    }
	*OldMouseButtonsP = MouseButtons;
    }
    else {
/*
	debug(("Calling LisaStyleMenu: x %d y %d oldmouse %d newmouse %d\n",
		MouseX,MouseY,OldMouseButtons,MouseButtons));
*/
	LisaStyleMenu (MouseX, MouseY,
		(OldMouseButtons & 2) + ((MouseButtons >> 1) & 1), 1);
	OldMouseButtons = MouseButtons;
    }
}
