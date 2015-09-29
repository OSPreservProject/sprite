#include "stdio.h"
#include "font.h"
#include "usergraphics.h"

wm_DrawString (x, y, op, string)
register x, y;
register char *string; {
    register FILE *o = winout;
    register struct font *f = CurFont;
    if (op&(wm_AtTop|wm_BetweenTopAndBottom|wm_AtBottom)) {
	x += f -> NWtoOrigin.x - f -> Wbase.x;
	y += f -> NWtoOrigin.y - f -> Wbase.y;
    }
    if (op&wm_BetweenTopAndBaseline) {
	x += (f -> NWtoOrigin.x - f -> Wbase.x)>>1;
	y += (f -> NWtoOrigin.y - f -> Wbase.y)>>1;
    }
    if (op&wm_AtBottom) {
	x -= f -> NtoS.x;
	y -= f -> NtoS.y;
    }
    if (op&wm_BetweenTopAndBottom) {
	x -= f -> NtoS.x>>1;
	y -= f -> NtoS.y>>1;
    }
    if (op&(wm_AtRight|wm_BetweenLeftAndRight)) {
	int LastXWidth, LastYWidth;
	wm_StringWidth (string, &LastXWidth, &LastYWidth);
	if (op&wm_AtRight) {
	    x -= LastXWidth;
	    y -= LastYWidth;
	} else {
	    x -= LastXWidth>>1;
	    y -= LastYWidth>>1;
	}
    }
    wm_MoveTo (x, y);
    while (*string) putc (*string++, o);
}

wm_printf (x, y, op, fmt, args)
register x, y;
char   *fmt; {
    register struct font   *f = CurFont;
    if (op&(wm_AtTop|wm_BetweenTopAndBottom|wm_AtBottom)) {
	x += f -> NWtoOrigin.x - f -> Wbase.x;
	y += f -> NWtoOrigin.y - f -> Wbase.y;
    }
    if (op&wm_BetweenTopAndBaseline) {
	x += (f -> NWtoOrigin.x - f -> Wbase.x)>>1;
	y += (f -> NWtoOrigin.y - f -> Wbase.y)>>1;
    }
    if (op & wm_AtBottom) {
	x -= f -> NtoS.x;
	y -= f -> NtoS.y;
    }
    if (op & wm_BetweenTopAndBottom) {
	x -= f -> NtoS.x >> 1;
	y -= f -> NtoS.y >> 1;
    }
    if (op & (wm_AtRight | wm_BetweenLeftAndRight)) {
	char    buf[300];
	struct _iobuf  strbuf;
	int     LastXWidth,
	        LastYWidth;
	register char  *p;
	register    FILE * f = winout;
	strbuf._flag = _IOSTRG;
	strbuf._ptr = buf;
	strbuf._cnt = sizeof buf;
	_doprnt (fmt, &args, &strbuf);
	putc ('\0', &strbuf);
	buf[sizeof buf - 1] = 0;
	wm_StringWidth (buf, &LastXWidth, &LastYWidth);
	if (op & wm_AtRight) {
	    x -= LastXWidth;
	    y -= LastYWidth;
	}
	else {
	    x -= LastXWidth >> 1;
	    y -= LastYWidth >> 1;
	}
	wm_MoveTo (x, y);
	p = buf;
	while (*p)
	    putc (*p++, f);
    }
    else {
	wm_MoveTo (x, y);
	_doprnt (fmt, &args, winout);
    }
}


wm_StringWidth (s, fx, fy)
int *fx, *fy;
register unsigned char  *s; {
    register struct font   *font = CurFont;
    register    x = 0,
                y = 0;
    while (*s) {
	register struct icon   *c = &font -> chars[*s++];
	if (c -> OffsetToGeneric) {
	    register struct IconGenericPart *g =
		(struct IconGenericPart *) (((int) c) + c -> OffsetToGeneric);
	    x += g -> Spacing.x;
	    y += g -> Spacing.y;
	}
    }
    if (fx) *fx = x;
    if (fy) *fy = y;
    return x;
}
