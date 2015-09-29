/* User graphics package.  Simple communicates with the window manager */

/*********************************\
* 				  *
* 	James Gosling, 1983	  *
* 	Copyright (c) 1983 IBM	  *
* 				  *
\*********************************/

#ifndef FONTMAGIC
#include "font.h"
#endif

enum GraphicsOperations {
	GR_MOVE,
	GR_LINE,
	GR_GETDIMS,
	GR_GETDIMSR,
	GR_SETFUNC,
	GR_SETTTL,
	GR_SETDIMS,
	GR_MOUSECHANGE,
	GR_SETRAWIN,
	GR_DISABLEINPUT,
	GR_ENABLEINPUT,
	GR_SETMOUSEINTEREST,
	GR_RASTEROP,
	GR_RASTERSMASH,
	GR_DELETEWINDOW,
	GR_DEFINEFONT,
	GR_DEFINEFONTR,
	GR_SELECTFONT,
	GR_SETMOUSEPREFIX,
	GR_MYPGRPIS,
	GR_ADDMENU,
	GR_DISABLENEWLINES,
	GR_SHOWBITS,
	GR_SENDFONT,
	GR_HEREISFONT,
	GR_SETCURSOR,
	GR_SETSPACESHIM,
	GR_SETCHARSHIM,
	GR_DEFINECOLOR,
	GR_HEREISCOLOR,
	GR_SELECTCOLOR,
	GR_HIDEME,
	GR_EXPOSEME,
	GR_SETPROGRAMNAME,
	GR_SETMOUSEGRID,
	GR_DEFINEREGION,
	GR_SELECTREGION,
	GR_ZAPREGIONS,
	GR_SETCLIPRECTANGLE,
	GR_ACQUIREINPUTFOCUS,
	GR_GIVEUPINPUTFOCUS,
	GR_IHANDLEACQUISITION,
	GR_WRITETOCUTBUFFER,
	GR_READFROMCUTBUFFER,
	GR_HEREISCUTBUFFER,
	GR_ROTATECUTRING,
	GR_SAVEREGION,
	GR_RESTOREREGION,
	GR_FORGETREGION,
	GR_HEREISREGION,
	GR_FILLTRAPEZOID,
	GR_ZOOMFROM,
	GR_SETMENUPREFIX,
	GR_LINKREGION,
	GR_NAMEREGION,
};

#define wm_MoveTo(x,y) GR_SEND(GR_MOVE, x, y)
#define wm_DrawTo(x,y) GR_SEND(GR_LINE, x, y)

#define wm_SetFunction(f)  {	register T = (f); \
				if (T != wm_CurrentFunction) \
				    GR_SEND(GR_SETFUNC, (wm_CurrentFunction=T));	}

/*
 * The following permit functions to be expressed as Boolean combinations
 * of the three primitive functions 'source', 'mask', and 'dest'.
 */

# define	f_source		0xCCCC
# define	f_mask			0xF0F0
# define	f_dest			0xAAAA

#define		f_black		0
#define		f_white		1
#define		f_invert	513
#define		f_copy		514
#define		f_BlackOnWhite	515
#define		f_WhiteOnBlack	516

int wm_CurrentFunction;

#define wm_SetTitle(s)   GR_SEND(GR_SETTTL, s)
#define wm_SetProgramName(s)   GR_SEND(GR_SETPROGRAMNAME, s)
#define wm_GetDimensions(width,height)   (GR_SEND(GR_GETDIMS), \
				 GR_WAITFOR (GR_GETDIMSR, 1), \
				 *(width)=uarg[0],*(height)=uarg[1])
#define wm_SetDimensions(minx, maxx, miny, maxy) GR_SEND(GR_SETDIMS, minx, maxx, miny, maxy)
#define wm_MouseInputToken (0200|GR_MOUSECHANGE)
#define wm_SawMouse(action,x,y) (GR_WAITFOR(GR_MOUSECHANGE,0), \
			      (*action)=uarg[0], \
			      (*x)=uarg[1], \
			      (*y)=uarg[2])
#define wm_SetRawInput() GR_SEND(GR_SETRAWIN)
#define wm_DisableInput() GR_SEND(GR_DISABLEINPUT)
#define wm_EnableInput() GR_SEND(GR_ENABLEINPUT)
#define wm_RasterOp(sx,sy,dx,dy,w,h) GR_SEND(GR_RASTEROP,sx,sy,dx,dy,w,h)
#define wm_RasterSmash(dx,dy,w,h) GR_SEND(GR_RASTERSMASH,dx,dy,w,h)
#define wm_DeleteWindow() (GR_SEND(GR_DELETEWINDOW), fclose(winout))
#define wm_ClearWindow() putc('\014', winout)
#define wm_SetMouseInterest(mask) GR_SEND(GR_SETMOUSEINTEREST, mask)
#define wm_AddMenu(s) GR_SEND(GR_ADDMENU, s)
#define wm_StdioWindow() wm_SelectWindow (0)
#define wm_DefineFont(f) (GR_SEND(GR_DEFINEFONT,f), \
			  GR_WAITFOR(GR_DEFINEFONTR,1), \
			  wm_FontStruct(uarg[0]))
#define wm_SelectFont(n) GR_SEND(GR_SELECTFONT,(CurFont = n)->magic)
#define wm_SetMousePrefix(p) GR_SEND(GR_SETMOUSEPREFIX,p)
#define wm_DisableNewlines() GR_SEND(GR_DISABLENEWLINES)
#define wm_ShowBits(x,y,w,h,b) GR_SEND(GR_SHOWBITS,x,y,w,h,b)
#define wm_SetCursor(f,c) GR_SEND(GR_SETCURSOR,(f)->magic,c)
#define wm_SetStandardCursor(c) GR_SEND(GR_SETCURSOR,-1,c)
#define wm_SetSpaceShim(s) GR_SEND(GR_SETSPACESHIM,s)
#define wm_SetCharShim(s) GR_SEND(GR_SETCHARSHIM,s)
#define wm_DefineColor(s,c) wm_DoDefineColor(s,c)
#define wm_SelectColor(c) GR_SEND(GR_SELECTCOLOR,c->index)
#define wm_SelectColorElement(c,e) GR_SEND(GR_SELECTCOLOR,c->index+((e)<<8))
#define wm_HideMe() GR_SEND(GR_HIDEME)
#define wm_ExposeMe() GR_SEND(GR_EXPOSEME)
#define wm_SetMouseGrid(n) GR_SEND(GR_SETMOUSEGRID,n)
#define wm_DefineRegion(id,x,y,w,h) GR_SEND(GR_DEFINEREGION,id,x,y,w,h)
#define wm_SelectRegion(id) GR_SEND(GR_SELECTREGION,id)
#define wm_ZapRegions() GR_SEND(GR_ZAPREGIONS)
#define wm_SetClipRectangle(x,y,w,h) GR_SEND(GR_SETCLIPRECTANGLE,x,y,w,h)
#define wm_AcquireInputFocus() GR_SEND(GR_ACQUIREINPUTFOCUS)
#define wm_GiveupInputFocus() GR_SEND(GR_GIVEUPINPUTFOCUS)
#define wm_IHandleAcquisition() GR_SEND(GR_IHANDLEACQUISITION)
#define wm_WriteToCutBuffer() GR_SEND(GR_WRITETOCUTBUFFER)
#define wm_ReadFromCutBuffer(n) GR_SEND(GR_READFROMCUTBUFFER, n)
#define wm_RotateCutRing(n) GR_SEND(GR_ROTATECUTRING,n)
#define wm_SaveRegion(id,x,y,w,h) GR_SEND(GR_SAVEREGION,id,x,y,w,h)
#define wm_RestoreRegion(id,x,y) GR_SEND(GR_RESTOREREGION,id,x,y)
#define wm_ForgetRegion(id) GR_SEND(GR_FORGETREGION,id)
#define wm_HereIsRegion(id,w,h) GR_SEND(GR_HEREISREGION,id,w,h)
#define wm_FillTrapezoid(x1,y1,w1,x2,y2,w2,f,c) GR_SEND(GR_FILLTRAPEZOID,x1,y1,w1,x2,y2,w2,f,c)
#define wm_ZoomFrom(x,y,w,h) GR_SEND(GR_ZOOMFROM,x,y,w,h)
#define wm_SetMenuPrefix(s) GR_SEND(GR_SETMENUPREFIX,s)
#define wm_LinkRegion(newid,oldid) GR_SEND(GR_LINKREGION,newid,oldid)
#define wm_NameRegion(id,name) GR_SEND(GR_NAMEREGION,id,name)

short uarg[6];

struct wm_window_aux {
    struct font *CurFont;
    short   nFonts;
    short   nSlots;
    struct font **font;
};

struct wm_window {
    struct _iobuf   outs,
                    ins;
    char    inb[200],
            outb[2048];
    struct wm_window_aux a;
};

struct font *CurFont;
struct wm_window *wm_NewWindow(/* host */);
struct wm_window *CurrentUserWindow;
struct wm_window_aux *CurrentUserWindowParameters;
#define outfile(w) (&(w)->outs)
#define infile(w) (&(w)->ins)
FILE *winout;
FILE *winin;
#define WMPORT 2000

/* Mouse button definitions */
#define LeftButton	2
#define MiddleButton	1
#define RightButton	0
#define UpMovement	0
#define DownTransition	1
#define UpTransition	2
#define DownMovement	3

#define MouseMask(button, action) (1<<((button)*4+(action)))

struct font *wm_FontStruct ();

#define wm_AtLeft		    1
#define wm_AtRight		    2
#define wm_BetweenLeftAndRight	    4
#define wm_AtTop		  010
#define wm_AtBottom		  020
#define wm_AtBaseline		  040
#define wm_BetweenTopAndBottom	 0100
#define wm_BetweenTopAndBaseline 0200


char *getprofile(/* char *var */);

#define wm_GunsightCursor		'g'
#define wm_CrossCursor			'x'
#define wm_HourglassCursor		'H'
#define wm_RightFingerCursor		'f'
#define wm_HorizontalBarsCursor		'h'
#define wm_LowerRightCornerCursor	'l'
#define wm_PaintbrushCursor		'p'
#define wm_UpperLeftCornerCursor	'u'
#define wm_VerticalBarsCursor		'v'
#define wm_DangerousBendCursor		'w'
#define wm_CaretCursor			'|'

#define wm_FlagRedraw	'R'
#define wm_FlagExpose	'E'
#define wm_FlagKill	'K'
#define wm_FlagHide	'H'

struct color {
    short index;
    short setsize;
};
struct color *wm_DoDefineColor();


#define program(n) char ProgramName[] = "n";
