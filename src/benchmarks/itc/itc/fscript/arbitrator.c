/* Arbitrator between program requests to the window system and the windows.
   Does most of the stuff having to do with the IPC mechanism. */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/

#include "sys/types.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "sys/stat.h"
#include "netinet/in.h"
#include "stdio.h"
#include "netdb.h"
#include "time.h"
#include "signal.h"
#include "sys/wait.h"
#include "errno.h"
#include "window.h"
#include "font.h"
#include "menu.h"
#include "usergraphics.h"
#include "display.h"
#include "keymap.h"

#ifdef BSD41c
struct timeval timeout;
#endif BSD41c

static ConnectionSocket;

static struct PrefixTable *PrefixTable;
static char *PrefixStrings;
static int *PrefixOffsets;
static int PrefixTableLength;

ProcessInput () {
    register struct Window *w;
    char    buf[500];		/* Keystrokes to send to the process */
    register char  *bufp = buf;
    if ((w = WindowInFocus) == 0)
	w = CursorWindow == 0
	    || CursorWindow -> t.type == SplitWindowType
	    ? 0
	    : CursorWindow;
    CursorDown ();
    do {
	register    c = getchar ();
	if (w && !w -> InputDisabled)
	    if ( /* w -> RawInput */ 1) {
		register struct PrefixTable *p = PrefixTable;
		register    lim = PrefixTableLength;
		register    offset = 0;
		while (--lim >= 0) {
		    if (p -> PackedPrefixKeys == w -> KeyState) {
			if (p -> FirstKey <= c && c < p -> FirstKey + p -> NumberOfKeys
				&& (offset = PrefixOffsets[offset + c - p -> FirstKey])) {
			    if (offset == -1)
				w -> KeyState = (w -> KeyState << 8) + c;
			    else {
				register char  *s = PrefixStrings + offset;
				while (*s)
				    *bufp++ = *s++;
				w -> KeyState = 0;
			    }
			    goto DeltWith;
			}
			break;
		    }
		    offset += p->NumberOfKeys;
		    p++;
		}
		if (w -> KeyState & 0377) {
		    if (w -> KeyState & (0377 << 8)) {
			if (w -> KeyState & (0377 << 16)) {
			    if (w -> KeyState & (0377 << 24))
				*bufp++ = w -> KeyState >> 24;
			    *bufp++ = w -> KeyState >> 16;
			}
			*bufp++ = w -> KeyState >> 8;
		    }
		    *bufp++ = w -> KeyState;
		}
		*bufp++ = c;
		w -> KeyState = 0;
	DeltWith: ;
	    }
    } while (stdin -> _cnt);
    if (bufp > buf)
	write (w -> SubProcess, buf, bufp - buf);
}

int ON = 1;

NewConnection () {
    struct sockaddr_in  from;
    int     len = sizeof from;
    int     fd = accept (ConnectionSocket, &from, &len);
    register struct Window  *w;
    static  sequence;
    if (fd < 0 && errno == EWOULDBLOCK) {
	debug (("Almost blocked accepting %d\n", fd));
	return;
    }
    if (fd < 0) {
	debug (("NewConnection: error %d\n", errno));
	return;
    }
    debug (("NewConnection: %d\n", fd));
#ifndef BSD41c
    if (fd > HighestDescriptor)
	HighestDescriptor = fd;
#endif BSD41c
    w = &WindowChannel[fd];
    ioctl (fd, FIOCLEX, 0);
    ioctl (fd, FIONBIO, &ON);
    ValidDescriptors |= 1 << fd;
    w -> x = 0;
    w -> y = 0;
    w -> CurrentFont = bodyfont;
    w -> MousePrefixString[0] = 0;
    w -> ArgsExpected = 0;
    w -> HasStringArgument = 0;
    w -> InputDisabled = 0;
    w -> RawInput = 0;
    w -> DoNewlines = 1;
    w -> dot = -1;
    w -> func = f_black;
    w -> Visible = 0;
    w -> SubProcess = fd;
    w -> pgrp = -1;
    w -> KnowsAboutChange = 1;
    w -> Hidden = 0;
    w -> IHandleAquisition = 0;
    w -> AcquireFocusOnExpose = AcquireFocusOnExpose | AcquireFocusOnCreate;
    w -> MouseMotionGranularity = 2;
    w -> menu = 0;
    w -> Cursor = 0;
    w -> minwidth = MinWindowWidth;
    w -> maxwidth = 1024;
    w -> minheight = MinWindowHeight;
    w -> maxheight = 1024;
    w -> SpaceShim = 0;
    w -> CharShim = 0;
    w -> MenuPrefix[0] = 0;
    w -> KeyState = 0;
    w -> MaxRegion = -1;
    w -> CurrentRegion = -1;
    w -> RegionsAllocated = 0;  /* I believe (RNS) */
    SetWindowTitle (w, "", "");
    {
	struct hostent *h = gethostbyaddr (&from.sin_addr, sizeof from.sin_addr, from.sin_family);
	if (h)
	    strncpy (w -> hostname, h ? h -> h_name : "Host?", sizeof w -> hostname);
    }
    w -> hostname[sizeof w -> hostname - 1] = 0;
    if (!StackedMenus) /* Not translated... */
    	AddMenuEntry (&w -> menu, "Window", 0, 0, ManagersMenu (), -1, 0);
    debug (("Window connection complete\n"));
}


ProcessDataFrom (fd) {
    register struct Window *w = &WindowChannel[fd];
    char    buf[3000];
    register    n;
    n = read (fd, buf, sizeof buf);
    if (n < 0 && errno == EWOULDBLOCK) {
	debug (("Almost blocked on %d\n", fd));
	return;
    }
    CurrentViewPort = 0;
    if (n > 0)
	ToWindow (w, buf, n);
    else
	if (errno == EINTR || errno == EINPROGRESS)
	    debug (("Skipped killing %s (errno = %d, n=%d)\n", w -> name, errno, n));
	else {
	    debug (("Unusual condition (errno=%d, n=%d, fd=%d): ", errno, n, fd));
	    KillProcess (fd);
	}
}

KillProcess (fd) {
    if (fd > 0) {
	register struct Window  *w = &WindowChannel[fd];
debug (("Killed %s\n", w->name));
	if (w -> pgrp > 0)
	    killpg (w -> pgrp, SIGTERM);
	KillWindow (w);
    }
}

Eshell () {
    if (vfork () == 0) {
	close (0);
	close (1);
	close (2);
	signal (SIGPIPE, SIG_DFL);
	signal (SIGURG, SIG_IGN);
	signal (SIGTERM, SIG_DFL);
	execlp ("typescript", "typescript", 0);
	exit (-1);
    }
}

FinishedProcessHandler () {
    union wait status;
    while (wait3 (&status, WNOHANG, 0) > 0) ;
}


DispatchLoop () {
    struct sockaddr_in  sin;
    struct servent *sp;
    struct hostent *hp;
    extern  errno;
    int     MouseFD;
    struct sgttyb   MouseSG;
    char   *MouseName = (char *) getenv ("MOUSEDEV");
    AcquireFocusOnExpose = getprofileswitch ("AcquireFocusOnExpose", 0);
    AcquireFocusOnCreate = getprofileswitch ("AcquireFocusOnCreate", 0);
    {
	char   *keyboard = getprofile ("keyboard");
	char    fn[100];
	char   *home = (char *) getenv ("HOME");
	int     fd;
	if (keyboard == 0)
	    keyboard = "sun";
	if (keyboard[0] == '/')
	    strcpy (fn, keyboard);
	else
	    sprintf (fn, "%s/%s.wmap", home, keyboard);
	if ((fd = open (fn, 0)) < 0) {
	    sprintf (fn, "/usr/local/lib/%s.wmap", keyboard);
	    fd = open (fn, 0);
	}
	if (fd < 0)
	    debug (("Keymap missing\n"));
	else {
	    struct stat st;
	    register struct LoadedKeymap   *k;
	    fstat (fd, &st);
	    k = (struct LoadedKeymap   *) malloc (st.st_size);
	    read (fd, k, st.st_size);
	    close (fd);
debug(("Opened keymap %s.  len=%d. FK=%d, NK=%d\n",fn,k->PrefixTableLength, k -> PrefixTable[0].FirstKey, k -> PrefixTable[0].NumberOfKeys));
	    PrefixTable = k -> PrefixTable;
	    PrefixTableLength = k -> PrefixTableLength;
	    PrefixStrings = (char *) k;
	    PrefixOffsets = (int *) (&k -> PrefixTable[PrefixTableLength]);
	    if (PrefixTableLength <= 0 || PrefixTableLength > 30
		    || k -> PrefixTable[0].FirstKey < 0
		    || k -> PrefixTable[0].FirstKey + k -> PrefixTable[0].NumberOfKeys > 128) {
		PrefixTableLength = 0;
		debug (("Bogus keymap: %s\n", fn));
	    }
	}
    }
    putenv ("TERM", "wm");
    signal (SIGURG, SIG_IGN);
    signal (SIGPIPE, SIG_IGN);
#ifndef BSD41c
    signal (SIGTERM, SIG_IGN);
    MouseFD = open (MouseName ? MouseName : "/dev/mouse", 0);
#else BSD41c
    MouseFD = open (MouseName ? MouseName : "/dev/ttyb", 0);
#endif BSD41c
    if (MouseFD < 0) {
	printf ("Couldn't open the mouse\n");
	exit (1);
    }
    MouseSG.sg_ispeed = 9;
    MouseSG.sg_ospeed = 9;
    MouseSG.sg_erase = -1;
    MouseSG.sg_kill = -1;
    MouseSG.sg_flags = RAW | ANYP;
    ioctl (MouseFD, TIOCSETP, &MouseSG);
/*  ioctl (0, FIONBIO, &ON); */
    RawMode ();
    signal (SIGCHLD, FinishedProcessHandler);
    {
	register char  *fn = getprofile ("bodyfont");
	if (fn == 0 || (bodyfont = getpfont (fn)) == 0)
	    bodyfont = getpfont ("gacha12f");
	fonts[0].this = bodyfont;
    }
    shapefont = getpfont ("shape10");
    gray = geticon (shapefont, 'G');
    ManagersMenu();
    bzero (&sin, sizeof sin);
    ConnectionSocket = socket (AF_INET, SOCK_STREAM, 0);
    setsockopt (ConnectionSocket, SOL_SOCKET, SO_REUSEADDR, 0, 0);
    setsockopt (ConnectionSocket, SOL_SOCKET, SO_USELOOPBACK, 0, 0);
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_family = AF_INET;
    sin.sin_port = WMPORT;
    if (bind (ConnectionSocket, &sin, sizeof sin) < 0) {
	debug (("Bind failed\n"));
	CleanupScreen ();
	exit (-1);
    }
    listen (ConnectionSocket, 5);
    ioctl (ConnectionSocket, FIOCLEX, 0);
    ioctl (ConnectionSocket, FIONBIO, &ON);
    ValidDescriptors = (1 << ConnectionSocket) | (1 << 0) | (1 << MouseFD);
    debug (("Listening on %d\n", ConnectionSocket));
    InitializeMouse ();
    Eshell ();
    HighestDescriptor = ConnectionSocket > MouseFD ? ConnectionSocket : MouseFD;
    {
	register char  *startup = getprofile ("startup");
	if (startup && vfork () == 0) {
	    close (0);
	    close (1);
	    close (2);
	    open ("/dev/null", 2);
	    dup (0);
	    dup (0);
	    signal (SIGPIPE, SIG_DFL);
	    signal (SIGURG, SIG_IGN);
	    signal (SIGTERM, SIG_DFL);
	    execlp ("sh", "sh", "-c", startup, 0);
	    exit (0);
	}
    }
    rjump ();
    while (1) {
	extern int  rfds;
	register    mask,
	            fd,
	            nfds;
	rfds = MenuActive ? (1 << 0) | (1 << MouseFD) : ValidDescriptors;
	for (fd = 0; fd < NDisplays; fd++)
	    if (RootWindow[fd] && RootWindow[fd] -> t.Changed) {
		CursorDown ();
		RedrawWindows (RootWindow[fd]);
		zoomY = 0;
	    }
	nfds = select (HighestDescriptor + 1, &rfds, 0, 0, 0);
	if (nfds < 0) {
	    if (errno == EINTR)
	        continue;
	    debug(("Bad select - errno = %d\n", errno));
	    continue;	/* XXX - should shut down? */
	}
	fd = 0;
	mask = 1 << 0;
	{
	    while ((rfds & mask) == 0 && fd <= HighestDescriptor)
		mask <<= 1, fd++;
	    if (fd > 30)
		debug (("Large FD\n"));
	    if (fd == MouseFD)
		SampleMouse (MouseFD);
	    else {
		if (fd == 0)  {		/* input from the keyboard */
		    if (SnapShotDisplay)  {
			register char c = getchar();

			if (c != '\007')  /* Ctrl-G aborts */
			    (*SnapShotDisplay->d_SnapShot) ();
			SnapShotDisplay = 0;
		    }
		    else 
		        ProcessInput ();
		}
		else
		    if (fd == ConnectionSocket)
			NewConnection ();
		    else
			ProcessDataFrom (fd);
	    }
	    if (!CursorVisible)
		MoveCursor (MouseX, MouseY);
	}
    }
}
