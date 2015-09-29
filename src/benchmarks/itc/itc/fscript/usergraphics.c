/* User graphics package.  Simply communicates with the window manager */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/


#include "stdio.h"
#include "sys/types.h"
#include "sys/ioctl.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#include "errno.h"
#include "usergraphics.h"
#include "signal.h"
#include "graphicops.c"

int errno;
static int SignalPrimed;

int wm_CurrentFunction = f_black;

extern FlagRedraw ();

static char *PrefixBufferFill;
static char PrefixBuffer[100];

static
SIGURGhandler () {
    char URGop[5];
    URGop[0] = 'R';
    recv (fileno(winin), URGop, sizeof URGop, 1);
    FlagRedraw (URGop[0]);
    if (URGop[0] == wm_FlagKill) exit (0);
    signal (SIGURG, SIGURGhandler);
}

struct wm_window *wm_NewWindow(host)
char   *host; {
    register struct wm_window  *w;
    register    winfd;
    struct sockaddr_in  sin;
    struct servent *sp;
    struct hostent *hp;
    static  pgrp = -1;
    static char myname[50];
    extern char ProgramName[];
    int     mynamel = sizeof myname;
    register    s;
    if (host == 0) {
	char   *s;
	if (myname[0] == 0)
	    if (s = (char *) getenv ("WMHOST"))
		strcpy (myname, s);
	    else
		gethostname (myname, &mynamel);
	host = myname;
    }
    if (pgrp < 0)
	setpgrp (0, (pgrp = getpid ()));
    bzero (&sin, sizeof sin);
    winfd = socket (AF_INET, SOCK_STREAM, 0);
    if (winfd < 0) {
/*	fprintf (stderr, "socket() failed: %d\n", errno); */
	return 0;
    }
    ioctl (winfd, SIOCSPGRP, &pgrp);
    setsockopt (winfd, SOL_SOCKET, SO_USELOOPBACK, 0, 0);
    hp = gethostbyname (host);
    if (hp == 0) {
/*	fprintf (stderr, "Couldn't find host %s\n", host); */
	return 0;
    }
    sin.sin_family = AF_INET;
    bcopy (hp -> h_addr, &sin.sin_addr, hp -> h_length);
    sin.sin_port = htons (WMPORT);
    if (connect (winfd, &sin, sizeof (sin)) < 0) {
/*	fprintf (stderr, "Connect failed: %d\n", errno); */
	return 0;
    }
    w = (struct wm_window  *) malloc (sizeof *w);
    w -> ins._file = winfd;
    w -> outs._file = winfd;
    w -> ins._flag = _IOREAD;
    w -> outs._flag = _IOWRT;
    setbuffer (outfile (w), w -> outb, sizeof w -> outb);
    setbuffer (infile (w), w -> inb, sizeof w -> inb);
    if (!SignalPrimed) {
	SignalPrimed = 1;
	signal (SIGURG, SIGURGhandler);
    }
    w -> a.nFonts = 0;
    w -> a.font = (struct font **)
                                        malloc ((w -> a.nSlots = 40)
	*                                   sizeof (struct font *));
    wm_SelectWindow (w);
    CurFont = wm_FontStruct (0);
    GR_SEND (GR_MYPGRPIS, pgrp);
    if (ProgramName[0])
	wm_SetProgramName (ProgramName);
    return w;
}

GR_SEND (op, arg) {
    register    FILE * f = winout;
    register int   *p = &arg;
    register struct GR_opinfo  *o = &GR_opinfo[op];
    register    n = o -> nargs;
/*  errno = 0; */
    putc (op|0200, f);
    while (--n >= 0) {
	putc (*p, f);
	putc (*p >> 8, f);
	p++;
    }
    if (o -> HasStringArgument) {
	register char  *s = (char *) * p;
	do
	    putc (*s, f);
	while (*s++);
    }
    if (ferror (f)) {
	int fd = creat ("/tmp/D:wm_ERRNO", 0644);
	write (fd, &errno, sizeof errno);
	close (fd);
	exit (0);
    }
}

static
UndoRead () {
    register new = PrefixBufferFill - PrefixBuffer;
    if (new > 0) {
	register FILE *f = winin;
	register char *s, *d;
	if (f -> _cnt == 0) {
	    f -> _ptr = f -> _base;
	} else {
	    register room = f -> _ptr - f -> _base;
	    if (room < new) {
		close (creat ("/tmp/Input lost in RPC", 0666));
		new = room;
	    }
	    f -> _ptr -= new;
	}
	f -> _cnt += new;
	s = PrefixBuffer;
	d = f -> _ptr;
	while (--new>=0) *d++ = *s++;
    }
    PrefixBufferFill = PrefixBuffer;
}

GR_WAITFOR (op, WaitForOp) {
    register    FILE * f = winin;
    register struct GR_opinfo  *o = &GR_opinfo[op];
    register    n,
                retl = o -> nargs;
    fflush (winout);
    PrefixBufferFill = PrefixBuffer;
/*  errno = 0; */
    if (WaitForOp)
	while ((n = getc (f)) != (op | 0200))
	    if (ferror (f) && errno != EINTR) {
		int     fd = creat ("/tmp/D:wm_ERRNO(wait)", 0644);
		write (fd, &errno, sizeof errno);
		close (fd);
		exit (0);
	    } else if (feof (f)) {
		int     fd = creat ("/tmp/D:HitEOF", 0644);
		close (fd);
		exit (0);
	    } else if (PrefixBufferFill < PrefixBuffer + sizeof PrefixBuffer)
		*PrefixBufferFill++ = n;
    for (n = 0; n < retl; n++) {
	register    T = getc (f) & 0377;
	uarg[n] = (short) (T + (getc (f) << 8));
    }
    if (op != ((int) GR_HEREISFONT) && PrefixBufferFill > PrefixBuffer)
	UndoRead ();
}

wm_SelectWindow(w)
register struct wm_window  *w; {
    if (CurrentUserWindow) {
	CurrentUserWindowParameters -> CurFont = CurFont;
    }
    if (CurrentUserWindow = w) {
	CurrentUserWindowParameters = &CurrentUserWindow -> a;
	winin = &w -> ins;
	winout = &w -> outs;
    }
    else {
	static struct wm_window_aux stdio_aux;
	CurrentUserWindowParameters = &stdio_aux;
	if (!SignalPrimed) {
	    SignalPrimed = 1;
	    signal (SIGURG, SIGURGhandler);
	}
	winin = stdin;
	winout = stdout;
	if (CurrentUserWindowParameters -> nFonts == 0) {
	    CurrentUserWindowParameters -> font = (struct font **)
			malloc ((CurrentUserWindowParameters -> nSlots = 40)
				*sizeof (struct font *));
	    CurrentUserWindowParameters -> CurFont = wm_FontStruct (0);
	}
    }
    CurFont = CurrentUserWindowParameters -> CurFont;
    wm_CurrentFunction = 1 << 30;
}

struct font *wm_FontStruct (index) {
    register struct font  **f;
    if (CurrentUserWindowParameters -> nSlots <= index)
	CurrentUserWindowParameters -> font = (struct font **)
		realloc (CurrentUserWindowParameters -> font,
			 (CurrentUserWindowParameters->nSlots = index*3/2)
			 	* sizeof (struct font *));
    while (CurrentUserWindowParameters -> nFonts <= index)
	CurrentUserWindowParameters -> font[CurrentUserWindowParameters -> nFonts++] = 0;
    f = &CurrentUserWindowParameters -> font[index];
    if (*f == 0) {
	register    n;
	register char *p;
	register FILE *WININ = winin;
	register struct font *nf;
	GR_SEND (GR_SENDFONT, index);
	GR_WAITFOR (GR_HEREISFONT, 1);
	*f = nf = (struct font  *) malloc (uarg[0]);
	p = (char *) nf;
	n = uarg[0];
	while (--n>=0)
	    *p++ = getc(WININ);
	if ((nf -> magic&0xffff) == ((FONTMAGIC>>8) | ((FONTMAGIC&0377)<<8))) {
	    			/* Its time to byte swap the font! */
	    swab (&nf -> magic, &nf -> magic, 2*sizeof(short));
	    swab (&nf -> NWtoOrigin, &nf -> NWtoOrigin, 10*sizeof(short));
	    swab (&nf -> fn.rotation, &nf -> fn.rotation, 1*sizeof(short));
	    swab (&nf -> NIcons, &nf -> NIcons, uarg[0] - ((int) &nf -> NIcons) + ((int) nf));
	}
	if (PrefixBufferFill > PrefixBuffer) UndoRead ();
	nf -> magic = index;
    }
    return * f;
}
