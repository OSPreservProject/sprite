/* The major window output driving routine */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/


#include "stdio.h"
#include "window.h"
#include "font.h"
#include "menu.h"
#include "usergraphics.h"
#include "graphicops.c"
#include "display.h"

#define HaveViewPort() (CurrentViewPort != 0 || NEEDVIEWPORT(w) != 0)
#define NeedViewPort() { if (!HaveViewPort ()) break;}

#define uarg(n) (short) (((unsigned char) w->args[n*2+1]) + (((unsigned char) w->args[n*2+2])<<8))


#define RingSize 10
struct CutBuffer {
    char   *buf;
    int     size;
    int     used;
}   CutRing[RingSize];
int RingPosition;


#define RegionID(proc,hid) (((hid)<<5)+(proc))
struct SavedRegion {
    short  *buf;
    int     size;
    short   width;
    short   height;
    int     id;
};
static struct SavedRegion *SavedRegion;
int NSavedRegion;
int NUsedRegion;

ZapRegionsForProc (pnum) {
    register struct SavedRegion *q;
    if (NUsedRegion)
	for (q = SavedRegion + NUsedRegion; --q >= SavedRegion;)
	    if ((q -> id & 037) == pnum)
		q -> id = 0;
}

static struct SavedRegion  *LookupRegion (id, create) {
    register struct SavedRegion *p,
                               *q;
    register    i;
    if (SavedRegion == 0)
	if (create)
	    SavedRegion = (struct SavedRegion *)
			malloc (sizeof (struct SavedRegion)
				* (NSavedRegion = 10));
	else
	    return 0;
    q = 0;
    for (i = NUsedRegion, p = SavedRegion; --i >= 0;)
	if (p -> id == id)
	    return p;
	else
	    if (p -> id == 0 && (q == 0 ||
			(p -> size >= create && p -> size < q -> size)))
		q = p;
    if (!create)
	return 0;
    if (q == 0) {
	if (NUsedRegion >= NSavedRegion)
	    SavedRegion = (struct SavedRegion *)
			realloc (SavedRegion, sizeof (struct SavedRegion)
				* (NSavedRegion += 10));
	q = &SavedRegion[NUsedRegion++];
	q -> size = 0;
	q -> buf = 0;
    }
    q -> id = id;
    SetRegionSize (q, create);
    return q;
}

SetRegionSize (p, size)
register struct SavedRegion *p; {
    if (p -> size < size) {
	if (p -> buf)
	    free (p -> buf);
	p -> buf = (short *) malloc (size);
	p -> size = size;
    }
}


struct ViewPort *
NEEDVIEWPORT(w)
register struct Window *w; {
    if (w -> Hidden) {
	static struct ViewPort  NullVP = {
	    0, 0, 0, 0
	};
	CurrentViewPort = &NullVP;
	return CurrentViewPort;
    }
    if (!w -> Visible) {
	CreateWindow (w);
	w -> KnowsAboutChange = 1;
	if (w -> AcquireFocusOnExpose) {
	    SetInputFocus (w);
	    w -> AcquireFocusOnExpose = AcquireFocusOnExpose;
	}
    }
    CurrentViewPort = &w -> SubViewPort;
    if (CurrentDisplay != w -> t.display && w -> t.display) {
	CurrentDisplay = w -> t.display;
	display = *CurrentDisplay;
    }
    return CurrentViewPort;
}


GR_SEND (w, op, arg)
enum GraphicsOperations op;
register struct Window  *w; {
    register int   *p = &arg;
    register struct GR_opinfo  *o = &GR_opinfo[(int) op];
    register    n = o -> nargs;
    char    buf[50];
    register char  *bp = buf;
    if (op != GR_MOUSECHANGE || w -> MousePrefixString[0] == 0) {
	*bp++ = ((int) op) | 0200;
	while (--n >= 0) {
	    *bp++ = *p;
	    *bp++ = *p >> 8;
	    p++;
	}
    }
    else {
	register char  *s = w -> MousePrefixString;
	register char  *sl = &w -> MousePrefixString[sizeof w -> MousePrefixString];
	while (*s && s < sl)
	    *bp++ = *s++;
	while (--n >= 0) {
	    *bp++ = *p & 0177;
	    *bp++ = (*p >> 7) & 0177;
	    p++;
	}
    }
    if (o -> HasStringArgument) {
	register char  *s = (char *) * p;
	do
	    *bp++ = *s;
	while (*s++);
    }
    {
	int     tries = 10;
	while (write (w -> SubProcess, buf, bp - buf) < 0 && --tries > 0)
	    sleep (1);
	if (tries <= 0)
	    write (0, "\r\nToo many retries...\r\n", 23);
    }
}


int rfds;			/* DEBUG */

ToWindow (w, s, n)
register struct Window *w;
register char  *s; {
    register char  *p;
    int     leop = -1;
    static struct CutBuffer *CurrentCut;
    CursorDown ();
/*  CurrentViewPort = &w -> t.ViewPort; */
    if (CurrentDisplay != w -> t.display && w -> t.display) {
	CurrentDisplay = w -> t.display;
	display = *CurrentDisplay;
    }
 /* debug (("Recieved %d bytes from %s (%o)  %o,%o,%o...\n", n, w -> name,
    w, s[0]&0377, s[1]&0377, s[2]&0377)); */
    if (CurrentCut) {
	register struct CutBuffer  *p;
	register    c;
PerformCut: 
	p = CurrentCut;
	if (p -> buf == 0)
	    p -> buf = (char *) malloc (p -> size = 200);
	while (--n >= 0 && (c = *s++)) {
	    if (p -> used >= p -> size)
		p -> buf = (char *) realloc (p -> buf, p -> size = p -> size * 3 / 2);
	    p -> buf[p -> used++] = c;
	}
	if (c == 0)
	    CurrentCut = 0;
    }
    while (n > 0) {
	if (w -> ArgsExpected) {
	    w -> args[w -> ThisArg++] = *s++;
	    if (w -> ThisArg >= sizeof w -> args - 2)
		debug (("ARG OVERFLOW!  op=%o\n", w -> args[0] & 0377));
	    n--;
	    if (w -> ThisArg >= w -> ArgsExpected &&
	    	( !w -> HasStringArgument ||
			(w->ThisArg > w->ArgsExpected && s[-1] == 0))) {
		leop = w -> args[0];
/* debug (("  op %o (%d)\n", leop&0177, w -> ThisArg)); */
		switch (w -> args[0] & 0177) {
		    default: 
			{
			    static int  erp;
			    if (!erp) {
				erp++;
				debug (("Bad opcode: %o\n", w -> args[0] & 0377));
			    }
			    break;
			}
		    case GR_MOVE: 
			w -> x = uarg (0);
			w -> y = uarg (1);
			break;
		    case GR_LINE: 
			NeedViewPort ();
			HW_SelectColor (w -> func);
			HW_vector (w -> x, w -> y, uarg (0), uarg (1));
			w -> x = uarg (0);
			w -> y = uarg (1);
			break;
		    case GR_SETFUNC: 
			w -> func = uarg (0);
			break;
		    case GR_SETTTL: 
			SetWindowTitle (w, &w -> args[1], 0);
			if (w -> Visible)
			    DrawModeLine (w);
			break;
		    case GR_SETPROGRAMNAME: 
			SetWindowTitle (w, 0, &w -> args[1]);
			if (w -> Visible)
			    DrawModeLine (w);
			break;
		    case GR_SETDIMS: 
			{
			    int     xmin = uarg (0),
			            ymin = uarg (2);
			    w -> minwidth = xmin < MinWindowWidth ? MinWindowWidth : xmin;
			    w -> maxwidth = uarg (1);
			    w -> minheight = ymin < MinWindowHeight ? MinWindowHeight : ymin;
			    w -> maxheight = uarg (3);
			}
			break;
		    case GR_SETMOUSEINTEREST: 
			w -> MouseInterest = uarg (0);
			break;
		    case GR_RASTEROP: 
			NeedViewPort ();
			HW_SelectColor (w -> func);
			HW_RasterOp (uarg (0), uarg (1), uarg (2), uarg (3), uarg (4), uarg (5));
			break;
		    case GR_FILLTRAPEZOID: 
			{
			    register    n = uarg (6);
			    register struct font   *f;
			    struct icon *shape;
			    if ((f = n < 0 || n >= nFonts ? shapefont : fonts[n].this) == 0)
				f = shapefont;
			    if (f && (n = uarg (7)) >= 0 && n <= 0177
				    && (shape = geticon (f, n))) {
				register    f = w -> func | f_CharacterContext;
				NeedViewPort ();
				HW_SelectColor (f);
				HW_FillTrapezoid (uarg (0), uarg (1), uarg (2), uarg (3), uarg (4), uarg (5), shape);
			    }
			}
			break;
		    case GR_RASTERSMASH: 
			NeedViewPort ();
			HW_SelectColor (w -> func);
			HW_RasterSmash (uarg (0), uarg (1), uarg (2), uarg (3));
			break;
		    case GR_DEFINEFONT: 
			{
			    extern  FontIndex;
			/* Ugh...  side effect from getpfont */
			    register struct font   *f = getpfont (&w -> args[1]);
			    GR_SEND (w, GR_DEFINEFONTR, FontIndex);
			}
			break;
		    case GR_SELECTFONT: 
			{
			    register    n = uarg (0);
			    if ((w -> CurrentFont = n < 0 || n >= nFonts ? bodyfont : fonts[n].this) == 0)
				w -> CurrentFont = bodyfont;
			}
			break;
		    case GR_MYPGRPIS: 
			w -> pgrp = uarg (0);
			break;
		    case GR_SETMOUSEPREFIX: 
			strncpy (w -> MousePrefixString, &w -> args[1], sizeof w -> MousePrefixString);
			break;
		    case GR_ADDMENU: 
			AddMenuTranslated(w, &w->args[1], 0);
			break;
		    case GR_SETCURSOR: 
			{
			    register    n = uarg (0);
			    extern struct font *iconfont;
			    register struct font   *f;
			    struct icon *NewCursor;
			    if ((f = n < 0 || n >= nFonts ? iconfont : fonts[n].this) == 0)
				f = iconfont;
			    if (f && (n = uarg (1)) >= 0 && n <= 0177
				    && (NewCursor = geticon (f, n)))
				if (w -> CurrentRegion < 0)
				    w -> Cursor = NewCursor;
				else
				    w -> regions[w -> CurrentRegion].Cursor = NewCursor;
			}
			break;
		    case GR_SETCHARSHIM: 
			w -> CharShim = uarg (0);
			break;
		    case GR_SETSPACESHIM: 
			w -> SpaceShim = uarg (0);
			break;
		    case GR_SENDFONT: 
			{
			    register    n = uarg (0);
			    struct font *f = n < 0 || n >= nFonts ? bodyfont : fonts[n].this;
			    register char  *p;
			    int     len,
			            written;
			    if (f == 0)
				GR_SEND (w, GR_HEREISFONT, 0);
			    else {
				GR_SEND (w, GR_HEREISFONT, f -> NonSpecificLength);


/* Unix design flaw: All the following screwing around is necessary
   because the channel has non-blocking mode set.  non-blocking has to
   be set for input operations -- but it isnt possible to set the
   blocking/nonblocking flag independantly for input and output */
				len = f -> NonSpecificLength;
				p = (char *) f;
				while (len > 0) {
				    n = len > 2048 ? 2048 : len;
				    errno = 0;
				    written = write (w -> SubProcess, p, n);
				    if (written > 0) {
					len -= written;
					p += written;
				    }
				    else
					sleep (1);
				}
/*			    write (w -> SubProcess, f, f -> NonSpecificLength); */
			    }
			    break;
			}
		    case GR_DEFINECOLOR: 
			{
			    struct hw_color *p = 0;
			    HW_DefineColor (w, &w -> args[3], uarg (0), &p);
			    if (p)
				GR_SEND (w, GR_HEREISCOLOR, 0, 0);
			    else
				GR_SEND (w, GR_HEREISCOLOR, p -> index, p -> range);
			    break;
			}
		    case GR_SELECTCOLOR: 
			w -> func = uarg (0);
			break;
		    case GR_SETMOUSEGRID: 
			w -> MouseMotionGranularity = uarg (0);
			break;
		    case GR_DEFINEREGION: 
		    case GR_SELECTREGION: 
		    case GR_LINKREGION:
		    case GR_NAMEREGION:
			{
			    int     id = uarg (0);
			    if (id >= w -> RegionsAllocated) {
				int     ntop = id + 4;
				if (w -> RegionsAllocated)
				    w -> regions = (struct WindowRegion *)
				                   realloc (w -> regions, sizeof (struct WindowRegion) * ntop);
				else
				w -> regions = (struct WindowRegion *)
				                   malloc (sizeof (struct WindowRegion) * ntop);
				if (w -> regions == 0) {
				    w -> RegionsAllocated = 0;
				    w -> CurrentRegion = -1;
				    w -> MaxRegion = -1;
				    break;
				}
				while (w -> RegionsAllocated < ntop) {
				    register struct WindowRegion   *r = &w -> regions[w -> RegionsAllocated];
				    r -> region.width = 0;
				    r -> region.height = 0;
				    r -> area = 0;
				    r -> name = 0;
				    r -> linked = w->RegionsAllocated++;
				    r -> menu = 0;
				    r -> Cursor = 0;
				}
			    }
			    if (id > w->MaxRegion)
			        w->MaxRegion = id;
			    switch (w -> args[0] & 0177) {
			        case GR_DEFINEREGION: {
				    register struct WindowRegion   *r = &w -> regions[id];
				    NeedViewPort ();
				    r -> region.left = uarg (1) + w -> SubViewPort.left;
				    r -> region.top = uarg (2) + w -> SubViewPort.top;
				    r -> region.width = uarg (3);
				    r -> region.height = uarg (4);
				    r -> area = r -> region.width * r -> region.height;
				    break;
				}
				case GR_SELECTREGION: {
				    w -> CurrentRegion = id;
				    break;
				}
				case GR_LINKREGION: {
				    int linkedto = uarg(1);
				    if (linkedto <= w->MaxRegion)
				        w->regions[id].linked = linkedto;
				    break;
				}
				case GR_NAMEREGION: {
				    register char **np = &w->regions[id].name;
				    debug(("NAMEREGION: id = %d, np = %x, name = %s\n",
				    	id, np, &w->args[3]));
				    if (*np)
				    	free(*np);
				    (*np) = (char *) malloc(strlen(&w->args[3]) + 1);
				    strcpy(*np, &w->args[3]);
				    break;
				}
			    }
			break;
			}
		    case GR_SETCLIPRECTANGLE: 
			{
			    int     x = uarg (0),
			            y = uarg (1),
			            width = uarg (2),
			            height = uarg (3);
			    NeedViewPort ();
			    if (width < 0)
				x += width, width = -width;
			    if (height < 0)
				y += height, height = -height;
			    if (x < 0)
				width += x, x = 0;
			    if (y < 0)
				height += y, y = 0;
			    if (x + width > w -> SubViewPort.width)
				width = w -> SubViewPort.width - x;
			    if (y + height > w -> SubViewPort.height)
				height = w -> SubViewPort.height - y;
			    if (width <= 0 || height <= 0)
				width = 0, height = 0;
			    w -> SubViewPort.top = w -> BasicSubViewPort.top + y;
			    w -> SubViewPort.left = w -> BasicSubViewPort.left + x;
			    w -> SubViewPort.width = width;
			    w -> SubViewPort.height = height;
			}
			break;
		    case GR_READFROMCUTBUFFER: {
			    register    i = uarg (0) + RingPosition - 1;
			    register struct CutBuffer  *p;
			    while (i < 0)
				i += RingSize;
			    while (i >= RingSize)
				i -= RingSize;
			    p = &CutRing[i];
			    GR_SEND (w, GR_HEREISCUTBUFFER, i = p -> used);
			    while (i > 0) {
				register    n = write (w -> SubProcess, p -> buf + p -> used - i, i);
				if (n <= 0) {
				    debug (("Cut buffer write failed n=%d,errno=%d\n", n, errno));
				    break;
				}
				else
				    i -= n;
			    }
			    write (w -> SubProcess, "", 1);
			}
			break;
		    case GR_ROTATECUTRING: 
			RingPosition = uarg (0) + RingPosition - 1;
			while (RingPosition < 0)
			    RingPosition += RingSize;
			while (RingPosition >= RingSize)
			    RingPosition -= RingSize;
			break;
		    case GR_SAVEREGION: 
			{
			    register struct SavedRegion *p;
			    register    id = RegionID (w -> SubProcess, uarg (0));
			    int     x = uarg (1),
			            y = uarg (2),
			            width = uarg (3),
			            height = uarg (4);
			    struct raster   SaveArea;
			    NeedViewPort ();
			    HW_SelectColor (f_copy);
			    if (p = LookupRegion (id, 0))
				p -> id = 0;
			    p = LookupRegion (id, HW_SizeofRaster (width, height));
			    debug (("SaveArea p=%x, bits=%x, id=%x, w=%d, h=%d\n", p, p -> buf, p -> id, width, height));
			    SaveArea.bits = p -> buf;
			    SaveArea.width = p -> width = width;
			    SaveArea.height = p -> height = height;
			    HW_CopyScreenToMemory (x, y, 0, 0, &SaveArea, width, height);
			}
			break;
		    case GR_RESTOREREGION: 
			{
			    register struct SavedRegion *p;
			    register    id = RegionID (w -> SubProcess, uarg (0));
			    if (p = LookupRegion (id, 0)) {
				struct raster   SaveArea;
				NeedViewPort ();
				HW_SelectColor (w -> func);
				debug (("RestoreArea p=%x bits=%x id=%x w=%d h=%d\n", p, p -> buf, p -> id, p -> width, p -> height));
				SaveArea.bits = p -> buf;
				SaveArea.width = p -> width;
				SaveArea.height = p -> height;
				HW_CopyMemoryToScreen (0, 0, &SaveArea, uarg (1), uarg (2), p -> width, p -> height);
			    }
			}
			break;
		    case GR_FORGETREGION: 
			{
			    register struct SavedRegion *p;
			    register    id = RegionID (w -> SubProcess, uarg (0));
			    if (p = LookupRegion (id, 0))
				p -> id = 0;
			}
			break;
		    case GR_ZOOMFROM:
			if (HaveViewPort()) {
			zoomX = uarg(0)+CurrentViewPort->left;
			zoomY = uarg(1)+CurrentViewPort->top;
			zoomW = uarg(2);
			zoomH = uarg(3);
			}
			break;
		    case GR_SETMENUPREFIX:
			w->MenuPrefix[0]=0;
			if (w->MenuPrefix[1] = w->args[1]){
			    if (w->MenuPrefix[2] = w->args[2]) {
				if (w->MenuPrefix[3] = w->args[3])
				    w->MenuPrefix[0]++;
				w->MenuPrefix[0]++;
			    }
			    w->MenuPrefix[0]++;
			}
			break;
		}
		w -> ArgsExpected = 0;
		w -> HasStringArgument = 0;
	    }
	}
	else {
	    register char   c;
	    for (p = s; ((c = *p) > 015 || c > 0 && c < 010) && n > 0; p++, n--);
	    if (p > s && HaveViewPort ()) {
		register    f = w -> func | f_CharacterContext;
		HW_SelectColor (f);
		w -> x = HW_DrawString (w -> x, w -> y, w -> CurrentFont, s,
			p - s, w -> CharShim, w -> SpaceShim);
		w -> y = LastY;
	    }
	    if (n > 0)
		if (c & 0200)
		    if ((c & 0177) >= sizeof GR_opinfo / sizeof GR_opinfo[0])
			debug (("Really bogus opcode: %o  '%10.10s'\n", c & 0177, p - 10));
		    else {
			register struct GR_opinfo  *o = &GR_opinfo[c & 0177];
			if (o -> nargs == 0 && !o -> HasStringArgument) {
			    leop = c;
/* debug (("  op %o\n", leop&0177)); */
			    switch (c & 0177) {
				default: 
				    {
					static int  erp;
					if (!erp) {
					    erp++;
					    debug (("Bad opcode: %o\n", w -> args[0] & 0377));
					}
					break;
				    }
				case GR_GETDIMS: 
				    if (HaveViewPort ())
					GR_SEND (w, GR_GETDIMSR, ScreenWidth, ScreenHeight);
				    else
					GR_SEND (w, GR_GETDIMSR, 0, 0);
				    break;
				case GR_SETRAWIN: 
				    w -> RawInput = 1;
				    break;
				case GR_DISABLEINPUT: 
				    w -> InputDisabled = 1;
				    break;
				case GR_ENABLEINPUT: 
				    w -> InputDisabled = 0;
				    break;
				case GR_DISABLENEWLINES: 
				    w -> DoNewlines = 0;
				    break;
				case GR_DELETEWINDOW: 
				    KillWindow (w);
				    break;
				case GR_HIDEME: 
				    if (!w -> Hidden) {
					w -> KnowsAboutChange = 1;
					DeleteWindow (w);
				    }
				    break;
				case GR_EXPOSEME: 
				    if (w -> Hidden) {
					w -> KnowsAboutChange = 1;
					ShowProcess (w, 0);
				    }
				    break;
				case GR_IHANDLEACQUISITION: 
				    w -> IHandleAquisition = 1;
				    break;
				case GR_ACQUIREINPUTFOCUS: 
				    if (WindowInFocus != w) {
					NeedViewPort ();
					SetInputFocus (w);
				    }
				    break;
				case GR_GIVEUPINPUTFOCUS: 
				    if (WindowInFocus == w)
					GiveUpInputFocus ();
				    break;
				case GR_WRITETOCUTBUFFER: 
				    CurrentCut = &CutRing[RingPosition];
				    CurrentCut -> used = 0;
				    RingPosition++;
				    if (RingPosition >= RingSize)
					RingPosition = 0;
				    n--;
				    s = p + 1;
				    goto PerformCut;
			    }
			}
			else {
			    w -> args[0] = c;
			    w -> ArgsExpected = o -> nargs * 2 + 1;
			    if (w -> ArgsExpected < 0)
				debug (("%ds (%o)   ******************\n", w -> ArgsExpected, c & 0377));
			    w -> HasStringArgument = o -> HasStringArgument;
			    w -> ThisArg = 1;
			}
		    }
		else
		    if (HaveViewPort ())
			switch (c) {
			    case '\n': {
				    register    dist;
				    w -> x = 0;
				    w -> y += w -> CurrentFont -> newline.y;
				    dist = w -> y + w -> CurrentFont -> newline.y - ScreenHeight;
				    if (dist >= 0 && w -> DoNewlines) {
					register    n2 = n;
					s = p;
					while (--n2 > 0)
					    if (*++s == '\n')
						dist += w -> CurrentFont -> newline.y;
					HW_SelectColor (f_copy);
					HW_RasterOp (0, dist, 0, 0,
						ScreenWidth, ScreenHeight - dist);
					HW_SelectColor (f_white);
					HW_RasterSmash (0, ScreenHeight - dist,
						ScreenWidth, dist);
					w -> y -= dist;
				    }
				}
				break;
			    case '\t': {
				    register    nx;
				    register struct icon   *c = &w -> CurrentFont -> chars['n'];
				    register    tw;
				    if (c -> OffsetToGeneric)
					tw = ((struct IconGenericPart  *) (((int) c) + c -> OffsetToGeneric)) -> Spacing.x * 8;
				    else
				    tw = 1;
				    nx = (w -> x + tw) / tw * tw;
				    HW_SelectColor (f_white);
				    HW_RasterSmash (w -> x, w -> y, nx - w -> x, w -> CurrentFont -> newline.y);
				    w -> x = nx;
				}
				break;
			    case '\b': {
				    register struct icon   *c = &w -> CurrentFont -> chars['_'];
				    if (c -> OffsetToGeneric)
					w -> x -= ((struct IconGenericPart *) (((int) c) + c -> OffsetToGeneric)) -> Spacing.x;
				}
				break;
			    case '\r': 
				w -> x = 0;
				break;
			    case 'l' & 037: 
				w -> x = 0;
				w -> y = w -> CurrentFont -> NWtoOrigin.y;
				HW_SelectColor (f_white);
				HW_RasterSmash (0, 0, ScreenWidth, ScreenHeight);
				break;
			}
	    p++;
	    n--;
	    s = p;
	}
    }
}
