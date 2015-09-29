/*****************************************\
* 					  *
* 	File: menu.c			  *
* 	Copyright (c) 1984 IBM		  *
* 	Date: Tue Mar 20 09:06:33 1984	  *
* 	Author: James Gosling		  *
* 					  *
* Hierarchic pop-up menu package.	  *
* 					  *
* HISTORY				  *
* 					  *
\*****************************************/



#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "font.h"
#include "menu.h"
#include "window.h"
#include "display.h"
#include "sys/ioctl.h"
#include "time.h"
#include "ctype.h"
#include "util.h"

struct VisibleMenu {
    int     MWidth,
            MHeight,
            MX,
            MY,
	    SaveSize,
	    blackline;
    char *MTitle;
    struct menu *m;
    struct raster SaveArea;
};

static struct display *MenuDisplay;
static struct VisibleMenu menus[50];   /* Should allocate this dynamically!!! */
static NMenusVisible;
static DSHRmode;
static long MenuClickInterval;
static TranslateMenus;
static SortMenus;

#define StackXShift (mfont->newline.y/3)
#define StackYShift (mfont->newline.y*5/4)
#define MenuBannerHeight (mfont->newline.y*3/2)

static int MX, MY, MW, MH;

static struct MenuEntryTranslation {
    struct MenuEntryTranslation *next;
    char level;				/* 0: system file, higher user files */
    char *in;				/* input string */
    unsigned int hash;			/* hash of input string */
    char *program;			/* restricted to this program */
    char *out;				/* output string */
    char *region;			/* add menu to this region */
    char *deletekey;			/* only delete if this matches menu item */
} **TranslationHashTable;
static int TranslationHashTableSize = 0;

static char *DeleteKey;

char *savestr(s)
char *s; {
    register char *ret = (char *) malloc (strlen(s)+1);
    strcpy(ret, s);
    return ret;
}

/*char *savestrn(s,n)
char *s; int n; {
    register char *ret = (char *) malloc (n + 1);
    strncpy(ret, s, n);
    return ret;
}*/

static int ReadLine(file,buf,maxsize)
register FILE *file;
char *buf;
register int maxsize;
{
    register char *p = buf;
    register char c;
    register size = 0;
    while ((c = fgetc(file)) != NULL && c != '\n') {
	if (c == '#') {
	    while ((c = fgetc(file)) != NULL && c != '\n')
	    	;
	    break;
	}
	if (++size == maxsize)
	    return 0;
	*p++ = c;
    }
    if (c == NULL)
    	return 0;
    if (++size == maxsize)
        return 0;
    *p = '\0';
    return size;
}

static ReadMenuTranslations(filename)
char *filename;
{
    static  level = 0;
    FILE * file = fopen (filename, "r");
    char    line[512];
    char   *program,
           *in,
           *region,
           *out,
	   *dk;
    int length;
    level++;			/* later files have precedence */
    if (file == NULL)
	return (0);
    while ( (length = ReadLine (file, line, sizeof (line)) ) >0) {
	unsigned register   hash = 0;
	register char  *p = line;
	char   *mline;
	while (*p && isspace (*p))
	    p++;
	mline = (char *) malloc (length);
	strcpy (mline, line);
	program = 0;
	in = p = mline;
	region = 0;
	out = 0;
	while (*p && *p != ':') {
	    hash = hash * 31 + (*p = FoldTRT[*p]);
	    if (*p == '.' && !program) {
		hash = 0;
		*p = '\0';
		program = mline;
		in = p + 1;
	    }
	    p++;
	}
	if (*p != ':')
	    continue;
	*p++ = '\0';
	while (*p && isspace (*p))
	    p++;
	if (program && *program == '\0')
	    program = 0;
	out = p;
	if (p = (char *) index (p, '.')) {
	    *p++ = '\0';
	    region = out;
	    out = p;
	}
	p = out + strlen(out);
	while (isspace(*--p))
	    *p = '\0';
	if (!*out)
	    out = NULL;
        dk = in;
        if (p = (char *) index(in, ',')) {
	    dk = (char *) malloc(p-in+1);
	    strncpy(dk,in,p-in);
	}
	{
	    register struct MenuEntryTranslation
	       **tp = &TranslationHashTable[hash % TranslationHashTableSize],
	       *te = (struct MenuEntryTranslation  *) malloc (sizeof (struct MenuEntryTranslation));
	    te -> next = *tp;
	    te -> level = level;
	    te -> in = in;
	    te -> hash = hash;
	    te -> program = program;
	    te -> out = out;
	    te -> region = region;
	    te -> deletekey = dk;
	    *tp = te;
	}
    }
    fclose(file);
}

InitializeMenuSystem() {
    char names[200];
    register char *s;
    register char *f;
    register char c;
    int size = 0;
    int nnames = 0;
    StackedMenus = getprofileswitch("StackMenus", 0);
    TranslateMenus = getprofileswitch("MapMenus", StackedMenus);
    SortMenus = getprofileswitch("SortMenus", TranslateMenus);

    s = getprofile ("menupath");
    if (TranslateMenus) {
	if (s)
	    strcpy(names, s);
	else
	    sprintf(names, "%s/menus:/usr/local/lib/menus", getenv ("HOME"));
    }
    else
	strcpy(names,"");
    if (*(s = names))
        do {
	    struct stat buf;
	    f = s;
            while ((c = *s) && c != ':')
    	        s++;
    	    *s++ = '\0';
	    if (stat(f, &buf) == 0)
	  	size += buf.st_size;
	    nnames++;
	} while (c);
    TranslationHashTableSize = size/20 + 1;    
    TranslationHashTable = (struct MenuEntryTranslation **)
    	calloc (TranslationHashTableSize,sizeof (struct MenuEntryTranslation *));
    --s;
    while (nnames>1) {
        nnames--;
	while (*--s);
	ReadMenuTranslations(s+1);
    }
    if (nnames == 1) {
        ReadMenuTranslations(names);
    }
}


static
DropMenu (MenuNumber) {
    while (MenuNumber < NMenusVisible) {
	register struct VisibleMenu *vm = &menus[NMenusVisible - 1];
	if (vm -> MWidth) {
	    CursorDown ();
	    HW_SelectColor (f_copy);
	    HW_CopyMemoryToScreen (0, 0, &vm -> SaveArea,
		    vm -> MX, vm -> MY, vm -> MWidth, vm -> MHeight);
	    vm -> MWidth = 0;
	    vm -> blackline = -1;
	}
	NMenusVisible--;
    }
}

static
DrawMenu (m, x, y, vm)
register struct menu   *m;
register struct VisibleMenu *vm; {
    register    i,
                x2,
                y2;
    if (m == 0) {
	printf ("Null menu\n");
	exit (1);
    }
    CursorDown ();
    MenuActive = 1;
    vm -> MX = x;
    vm -> MY = y;
    if (StackedMenus) {
	vm->MWidth = MW;
	vm->MHeight = MH;
    } else {
        vm -> MWidth = m -> width;
        vm -> MHeight = m -> height;
    }
    vm -> blackline = -1;
    HW_SelectColor (f_copy);
    vm -> m = m;
    {
	register    size = HW_SizeofRaster (vm -> MWidth, vm -> MHeight);
	if (size > vm -> SaveSize) {
	    if (vm -> SaveArea.bits)
		free (vm -> SaveArea.bits);
	    vm -> SaveArea.bits = (short *) malloc (vm -> SaveSize = size * 6 / 5);
	}
	vm -> SaveArea.width = vm -> MWidth;
	vm -> SaveArea.height = vm -> MHeight;
    }
    HW_CopyScreenToMemory (vm -> MX, vm -> MY, 0, 0, &vm -> SaveArea,
	    vm -> SaveArea.width, vm -> SaveArea.height);
    HW_SelectColor (f_white);
    HW_RasterSmash (x, y, vm -> MWidth, vm -> MHeight);
    x2 = x + vm -> MWidth - 1;
    y2 = y + vm -> MHeight - 1;
    HW_SelectColor (f_black);
    HW_vector (x, y, x, y2);
    HW_vector (x, y2, x2, y2);
    HW_vector (x2, y, x2, y2);
    HW_vector (x2, y, x, y);
/*  if (StackedMenus)
    	HW_vector (x, y + mfont->newline.y, x2, y+mfont->newline.y);*/
    x2 = x + 3 + mfont -> NWtoOrigin.x;
    y2 = y + 4 + mfont -> NWtoOrigin.y;
    HW_SelectColor (f_black|f_CharacterContext);
    {
	register struct item   *p = m -> item;
	if (StackedMenus/* && *vm->MTitle*/) {
	    int xoffset = (vm->MWidth - StringWidth(mfont, vm->MTitle))/2;
	    HW_DrawString (x2 + xoffset, y2, mfont, vm->MTitle, 0, 0, 0);
	    y2 += MenuBannerHeight;
	}
	for (i = 0; i < m -> nitems; i++) {
	    int Offset;
	    /*Offset = StackedMenus? (vm->MWidth - p->width)/2 : 0;*/
	    HW_DrawString (x2 /*+ Offset*/, y2, mfont, p -> name, 0, 0, 0);
	    y2 += mfont -> newline.y;
	    if (StackedMenus && i + 1 < m->nitems && p->priority/10 != (p+1)->priority/10)
	    	y2 += mfont->newline.y;
	    p++;
	}
    }
}



static
int BlackLine(y, vm)
struct VisibleMenu *vm;
{
    register struct item *item;
    register struct menu *m;
    register itemy,i,nitems;
    register nl;
    y -= 3;
    if (StackedMenus /*&& *vm -> MTitle*/) {
	y -= MenuBannerHeight;
	if (y < 0)
	    return  -1;
    }
    nl = mfont->newline.y;
    nitems = vm->m->nitems;
    for (i = 0, itemy = nl, item = vm->m->item; i<nitems; i++,itemy+=nl,item++) {
	if (y < itemy)
		return i;
	if (StackedMenus && i + 1 < nitems && item->priority/10 != (item+1)->priority/10) {
		if (y <= itemy + (nl>>1))
		    return i;
		itemy += nl;
	}
    }
    return -1;
}

static
FlipButton (n, vm)
register struct VisibleMenu *vm; {
    register int i,y;
    register struct item *item = vm->m->item;
    CursorDown ();
    HW_SelectColor (f_invert);
    y = vm->MY + 3;
    for (i=0; i<n; i++,item++) {
	y+=mfont->newline.y;
	if (StackedMenus && item->priority/10 != (item+1)->priority/10)
		y += mfont->newline.y;
    }
    if (StackedMenus/* && *vm->MTitle*/)
    	y += MenuBannerHeight;
    HW_RasterSmash (vm->MX + 2,
    		    y,
		    vm->MWidth - 4,
		    mfont->newline.y);
}

struct menu *AllocateMenu (title)
char   *title; {
    register struct menu   *m = (struct menu   *) malloc (sizeof (struct menu));
    m -> item = (struct item   *) malloc ((m -> itemspace = 10) * sizeof (struct item));
    m -> title = savestr (title);
    m -> nitems = 0;
    m -> twidth = 0;
    m -> LastPosition = -1;
    m -> height = 5;	/* only for old style menus */
    m -> width = 0;
    return m;
}


DeleteMenuEntry (name, m)
char *name;
register struct menu   *m; {
    register struct item   *p = m -> item;
    register i;
    for (i = 0; i < m -> nitems; i++, p++)
	if (p -> name[0] == name[0] && strcmp (p -> name, name) == 0)
	    break;
    if (i < m -> nitems)
	DeleteIthMenuEntry (i, m);
}

static
DeleteIthMenuEntry (i, m)
register struct menu *m;
register    i; {
    if (i < m -> nitems) {
	register struct item   *p = &m -> item[i];
	register struct menu   *submenu = p -> menu;
        
	if (p -> name) {
	    if (DeleteKey != p->deletekey || (DeleteKey && strcmp(DeleteKey,p->deletekey)))
		return 0;
	    if (StringWidth (mfont, p -> name) + 6 >= m -> width) {
		register struct item *i;
		m -> width = 0;
		for (i = &m -> item[m -> nitems]; --i >= m -> item; )
		if (i != p && i -> name) {
		    register width = StringWidth (mfont, i -> name) + 6;
		    if (width > m -> width) m -> width = width;
		}
	    }
	    free (p -> name);
	}
	if (p -> string)
	    free (p -> string);
	if (submenu && submenu != LManagersMenu) {
	    register    j;
	    int DeleteSubmenu = 1;
	    for (j = 0; j < submenu -> nitems; j++)
		DeleteSubmenu &= DeleteIthMenuEntry (j, submenu);
	    if (DeleteSubmenu)
	        free (submenu);
	}
	while (i+1 < m->nitems) {
		m->item[i] = m->item[i+1];
		i++;
	}
	m->nitems--;
	m -> LastPosition = -1;
        m -> height -= mfont -> newline.y;
    }
    return 0;
}



struct menu *AddMenuEntry (mp, name, string, function, menu, priority, flags)
struct menu **mp;	/* add to this menu */
register char *name;	/* display this string in the pop-up menu */
char   *string;		/* send this string back to the client (optional) */
int (*function)();	/* window manager function to call (optional) */
struct menu *menu;	/* submenu to associate with this entry (optional) */
short priority;		/* sort priority */

{
    register    i,k;
    register struct item   *p;
    register struct menu   *m = *mp;
    register char *cp;

    if (m == 0)
	m = AllocateMenu ("*BOGUS*");
    p = m -> item;
    for (k = i = 0; i < m -> nitems; i++, p++) {
	int comp;
	if ((comp = strcmp (p -> name, name)) == 0)
	    break;
	if (p->priority < priority || (p->priority == priority && (comp < 0 || !SortMenus)))
	    k = i+1;
    }
    if (i >= m -> nitems) {
	register    width;
	if (mfont == 0) {
	    char   *s;
	    DSHRmode = getprofileswitch ("dshrmode", 0);
	    s = getprofile ("menufont");
	    MenuClickInterval = getprofileint("MenuClickInterval",50) * 10000;
	    if (s == 0 || (mfont = getpfont (s)) == 0)
		mfont = getpfont ("helvetica12b");
	}
	if (m -> nitems >= m -> itemspace)
	    m -> item = (struct item   *) realloc (m -> item,
	                                sizeof (struct item) * (m -> itemspace += 10));
	for (i = m->nitems; i>k; i--)
	    m->item[i] = m->item[i-1];
	m -> nitems++;
	p = &m -> item[k];
	p -> name = savestr (name);
	p -> menu = 0;
	p -> string = 0;
	p -> function = 0;
	p -> priority = priority;
	p -> flags = flags;
        p -> deletekey = DeleteKey;
	m -> height += mfont -> newline.y;
	width = StringWidth (mfont, name) + 6;
	if (m -> width < width)
	    m -> width = width;
	p -> width = width;
    }
    *mp = m;
    if (string)
	p -> string = string;
    if (function)
	p -> function = function;
    if (((int) menu) == -1) {
	if (p -> menu == 0)
	    p -> menu = AllocateMenu ("*SUB*");
	return p -> menu;
    }
    if (menu)
	p -> menu = menu;
    return 0;
}

struct menu *AddMenu(RootMenu, s, function)
struct menu *RootMenu;
char   *s;
int (*function)();
{
    char    buf[100];
    char    copy[100];
    register char  *p,
                   *d;
    struct menu **m = &RootMenu;
    struct menu *ThisMenu = RootMenu;
    int     priority,
            flags;

    p = s;
    if (StackedMenus) {
	register    comma = 0;
	p = copy;
	*p++ = ',';
	strcpy (p, s);
	while (*p && *p != ':') {
	    if (*p == ',' && ++comma > 1)
		*p = '-';
	    p++;
	}
	p = copy;
	if (comma)
	    p++;
    }
    flags = 0;
    priority = 0;
    d = buf;
    while (1) {
	while (*p && *p != ':' && *p != ',' && *p != '~')
	    *d++ = *p++;
	*d++ = 0;
	switch (*p) {
		register char   c;
	    case '~': 
		while ((c = *p) && c != ':' && c != ',') {
		    if (isdigit (c))
			priority = priority * 10 + c - '0';
		    else
			switch (c) {
			    case 'g': 
			    case 'G': 
				flags |= mf_Greyable;
				break;
			    case 'd': 
			    case 'D': 
			    /* delete */
				return RootMenu;
			    case '~':	/* This can actually happen... */
			    	priority = 0;
				flags = 0;
				break;
			}
		    p++;
		}
		break;
	    case ':': 
		if (*++p)
		    AddMenuEntry (m, buf, savestr (p), 0, 0, priority, flags);
		return RootMenu;
	    case ',': 
		if (!buf[0])
		    priority = -1;
		ThisMenu = AddMenuEntry (m, buf, 0, 0, -1, priority, flags);
		m = &ThisMenu;
		if (StackedMenus && RootMenu && RootMenu -> nitems && RootMenu -> item[0].menu)
		    DeleteMenuEntry (buf, RootMenu -> item[0].menu);
				/* Bogus! */
		p++;
		flags = 0;
		priority = 0;
		d = buf;
		break;
	    default: 
		if (function)
		    AddMenuEntry (m, buf, 0, function, 0, priority, flags);
		else {
		    DeleteMenuEntry (buf, *m);
		    if (StackedMenus && RootMenu && RootMenu -> nitems
			    && RootMenu -> item[0].menu == *m)
			DeleteMenuEntry (buf, RootMenu);/* Bogus! */
		}
	        return RootMenu;
	}
    }
}


AddMenuTranslated(window, menustring, function)
struct Window *window;
register char *menustring;
int (*function)();
{
    char    buf[512];
    struct {
	unsigned int    hash;
	char   *eos;
    }       substrings[20], *ss = &substrings[0];
    int     levelused = 0;
    register char  *s = menustring,
                   *d = buf;
    char *returnstring;
    register char c;
    unsigned int    hash = 0;
    struct Window Widow; /*Gawd!  this is really awful*/
    if (!window) {
	window = &Widow;
	strcpy(Widow.ProgramName,"wm");
	Widow.MaxRegion = -1;
	Widow.CurrentRegion = -1;
	Widow.menu = LManagersMenu;
    }
    while ((c = FoldTRT[*s]) && c != ':') {
	if (c == ',') {
	    ss -> hash = hash;
	    ss -> eos = d;
	    ss++;
	}
	*d++ = *s++;
	hash = hash * 31 + c;
    }
    returnstring = s;
    *d = '\0';
    ss -> hash = hash;
    ss -> eos = d;
    for (; ss >= &substrings[0]; ss--) {
	register struct MenuEntryTranslation   *te =
	    TranslationHashTable[ss -> hash % TranslationHashTableSize];
	register char savec = *(ss -> eos);
	*(ss -> eos) = '\0';
	while (te && (!levelused || levelused == te -> level)) {
	    if (ss -> hash == te -> hash && FOLDEDEQ (te -> in, buf)
		    && (!te -> program || FOLDEDEQ (te -> program, window -> ProgramName))) {
		levelused = te -> level;
		if (te -> out) {
		    char    menustring[512];
		    register char *p,*q;
		    struct menu **m = 0;
	            d = menustring;		    
		    for (p = te->out; (*d = *p) && *p++ != '`'; d++);
		    *(ss->eos) = savec;
		    for (q = ss->eos; *d = *q++; d++);
		    for (; *d = *p++; d++);
		    s = returnstring;
		    while (*d++ = *s++);
		    if (te->region && window->MaxRegion >= 0) {
			register struct WindowRegion *r;
			for (r = &window->regions[window->MaxRegion]; r >= window->regions; r--)
			    if (r->name && FOLDEDEQ(te->region, r->name)) {
				m = &r->menu;
				break;
			    }
		    }
		    if (!m) {
			m = window->CurrentRegion < 0 ?
			    &window->menu :
			    &window->regions[window->CurrentRegion].menu;
		    }
		    DeleteKey = te->deletekey;
		    *m = AddMenu (*m, menustring, function);
		    DeleteKey = 0;
		}
	    }
	    te = te -> next;
	}
	if (levelused)
	    break;
	*(ss->eos) = savec;
    }
 /* this and previous AddMenu need better calculation of menu to add to */
    if (!levelused) {
	struct menu **m = window->CurrentRegion < 0 ?
			    &window->menu :
			    &window->regions[window->CurrentRegion].menu;
        *m = AddMenu (*m, menustring, function);
    }
}


CreateMenuStack(m)
struct menu *m;
{
    int W, H, X, Y;
    int n;
    int yshift;
    struct VisibleMenu *vm;
    /* Create a stack-of-cards menu */
    
    /* Find maximum width, height of all the menus */
    MW = m->width; /* Gives Maximum title width */
    MH = 0;
    for (n = 0; n < m->nitems; n++) {
	struct menu *tm = m->item[n].menu;
	if (tm && tm->width > MW)
		MW = tm->width;
	if (tm) {
	    register struct item *im = tm->item;
	    register length = tm->nitems;
	    register struct item *em = &im[length - 1];
	    while (im < em) {
	    	if (im->priority/10 != (im+1)->priority/10)
		    length++;
		im++;
	    }
	    if (length>MH)
	    	MH = length;
	}
    }
    MW;				/* All menu items will be this width */
    MH *= mfont->newline.y;
    MH += MenuBannerHeight + 5;	/* Actual height includes provision for menu banner */
    W = MW + StackXShift*(m->nitems-1);  /* Total width of menu stack */
    H = MH + StackYShift*(m->nitems-1);  /* Total height of menu stack */
    /* Try to position upper left corner of containing rectangle so original cursor
       position (before popping menu) will be at the center top of the topmost menu
       (this is where the user is looking--he wants to read the text, not stare at
       the cursor) */
    X = MouseX - (MW/2);
    Y = MouseY - (StackYShift/2) - (H - MH);
    /* Correct if off screen */
    if (X < 0)
    	X = 0;
    if (Y < 0)
        Y = 0;
    if (X + W >= ScreenWidth)
       X = ScreenWidth - W;
    if (Y + H >= ScreenHeight)
       Y = ScreenHeight - H;
    /* Position Cursor at middle of topmost menu */
    MouseX = X + MW/2;
    MouseY = Y + (StackYShift/2) + (H - MH);
    /* Get X,Y for top left corner of first (deepest) menu */
    X = X + W - MW;  /* Y is OK */
    /* Draw all the menus, starting from the deepest in the pile */
    vm = &menus[0];
    NMenusVisible = 0;
    for (n = m->nitems-1; n>=0; n--) {
        struct item *item = &m->item[n];
	struct menu *menu = item->menu;
	if (!menu)
	    continue;
        vm->MTitle = item->name;
	DrawMenu(menu, X, Y, vm);
	X -= StackXShift;
	Y += StackYShift;
	NMenusVisible++;
	vm++;
    }
}   

LisaStyleMenu (x, y, Action, Button) {
    static struct menu *m = 0;
    static struct Window   *InitialWindow;
    register struct VisibleMenu *vm;
    static  OldMouseX,
            OldMouseY;
    int     WasInsideAMenu = 0;
    static struct timeval   dtp;
    static struct timezone  tzp;
    static struct timeval   utp;

    if (Action == DownTransition && !MenuActive) {
	register struct Window *w;
	struct VisibleMenu *vmt;
	int     mx,
	        my;

	if (MenuClickInterval)
	    gettimeofday (&dtp, &tzp);

	CurrentViewPort = &CursorDisplay -> screen;
	if ((w = CursorWindow) == 0 || w -> t.type == SplitWindowType || (y + display.screen.top) < w -> SubViewPort.top)
	    m = LManagersMenu;
	else {
	    register struct WindowRegion   *r;
	    register    area = 999999;
	    m = w -> menu;
	    if (w -> MaxRegion >= 0)
		for (r = &w -> regions[w -> MaxRegion];
			r >= w -> regions; r--) {
		    struct menu *menu;

		if (x >= r -> region.left
			    && y >= r -> region.top
			    && area > r -> area
			    && (menu = w -> regions[r -> linked].menu)
			    && x < r -> region.left + r -> region.width
			    && y < r -> region.top + r -> region.height) {
			m = menu;
			area = r -> area;
		    }
		}
	}
	if (m == 0)
	    return;
    /* MenuActive = 1; */
	InitialWindow = w;
	MenuDisplay = CursorDisplay;
	if (MenuDisplay != CurrentDisplay) {
	    CurrentDisplay = MenuDisplay;
	    display = *CurrentDisplay;
	}
	if (StackedMenus) {
	    OldMouseX = MouseX;
	    OldMouseY = MouseY;
	    CreateMenuStack (m);
	    x = MouseX + CurrentViewPort -> left;
	    y = MouseY + CurrentViewPort -> top;
				/* Arghh!! (RNS) - Hear! Hear! (DSHR) */
	    vm = &menus[NMenusVisible - 1];
	}
	else {
	    vm = &menus[0];
	    vm -> m = m;
	    if (m -> LastPosition >= 0) {
		register struct menu   *t;
		while (m -> LastPosition >= 0 && (t = m -> item[m -> LastPosition].menu)) {
		    m = t;
		    vm++;
		    vm -> m = m;
		}
	    }
	    vmt = vm;
	    mx = MouseX;
	    my = MouseY;
	    while (vm >= &menus[0]) {
		m = vm -> m;
		if (m -> LastPosition >= 0) {
		    vm -> MX = mx - 1;
		    vm -> MY = my - ((((m -> LastPosition << 1) + 1) * mfont -> newline.y) >> 1);
		}
		else {
		    vm -> MX = mx + 1;
		    vm -> MY = my - m -> height / 2;
		}
		if (vm -> MX + m -> width >= ScreenWidth)
		    vm -> MX = ScreenWidth - m -> width;
		if (vm -> MY < 0)
		    vm -> MY = 0;
		if (vm -> MX < 0)
		    vm -> MX = 0;
		if (vm -> MY + m -> height >= ScreenHeight)
		    vm -> MY = ScreenHeight - m -> height;
		mx = vm -> MX - 35;
		vm--;
	    }
	    NMenusVisible = 0;
	    for (vm = &menus[0]; vm <= vmt; vm++) {
		m = vm -> m;
		DrawMenu (m, vm -> MX, vm -> MY, vm);
		if (vm < vmt && m -> LastPosition >= 0) {
		    vm -> blackline = m -> LastPosition;
		    FlipButton (vm -> blackline, vm);
		}
		NMenusVisible++;
	    }
	}
    }
    if (m && NMenusVisible > 0) {
	register    l = -1;
	register    i,
	            menu = -1;
	if (MenuDisplay != CurrentDisplay) {
	    CurrentDisplay = MenuDisplay;
	    display = *CurrentDisplay;
	}
	CurrentViewPort = &display.screen;
/*
	x -= display.screen.left;
	y -= display.screen.top;
*/
	for (i = NMenusVisible; --i >= 0;) {
	    vm = &menus[i];
	    if (x > vm -> MX && x < vm -> MX + vm -> MWidth
		    && y < vm -> MY + vm -> MHeight - 4
		    && y > vm -> MY) {
		l = BlackLine (y - vm -> MY, vm);
		menu = i;
		WasInsideAMenu = 1;
		break;
	    }
	}
	if (StackedMenus) {
	    if (menu == -1) {
		vm = &menus[NMenusVisible - 1];
		if (vm -> blackline != -1) {
		    FlipButton (vm -> blackline, vm);
		    vm -> blackline = -1;
		}
	    }
	    else {
		DropMenu (menu + 1);
		vm = &menus[NMenusVisible - 1];
		if (l != vm -> blackline) {
		    if (vm -> blackline >= 0) {
			FlipButton (vm -> blackline, vm);
			vm -> blackline = -1;
		    }
		    if (l >= 0 && l < vm -> m -> nitems)
			FlipButton (vm -> blackline = l, vm);
		}
	    }
	}
	else {
	    if (l < 0)
		vm = &menus[NMenusVisible - 1];
	    if (l != vm -> blackline) {
		if (l >= 0)
		    DropMenu (i + 1);
		if (vm -> blackline >= 0)
		    FlipButton (vm -> blackline, vm);
		if (l >= 0) {
		    register struct menu   *m;
		    m = vm -> m;
		    if (m -> item[l].menu) {
			register    mx,
			            my;
			m = m -> item[l].menu;
			if (m -> nitems > 0) {
			    FlipButton (l, vm);
			    mx = vm -> MX + (vm -> MWidth < 36 ? vm -> MWidth : 36);
			    if (mx + m -> width >= display.screen.width) {
				register    i,
				            t;
				for (i = 0; i < NMenusVisible; i++)
				    if ((t = menus[i].MX - m -> width) < mx)
					mx = t;
			    }
			    if (m -> LastPosition >= 0)
				my = MouseY - ((((m -> LastPosition << 1) + 1) * mfont -> newline.y) >> 1);
			    else
				my = vm -> MY + 1 + (l + 1) * mfont -> newline.y;
			    if (my < 0)
				my = 0;
			    if (my + m -> height >= display.screen.height)
				my = display.screen.height - m -> height;
			    NMenusVisible++;
			    DrawMenu (m, mx, my, &menus[NMenusVisible - 1]);
			}
			else
			    l = -1;
		    }
		    else
			FlipButton (l, vm);
		}
		vm -> blackline = l;
		if (vm -> m && DSHRmode)
		    vm -> m -> LastPosition = l;
	    }
	}
    }
    if (Action == UpTransition) {
	int     seconds;

	if (MenuClickInterval) {
	    gettimeofday (&utp, &tzp);
	    if (((seconds = (utp.tv_sec - dtp.tv_sec)) < 3600) &&
		    ((seconds * 1000000 + utp.tv_usec - dtp.tv_usec) < MenuClickInterval))
		return;
	}

	if (m && NMenusVisible > 0) {
	    register struct menu   *HitMenu;
	    register    blackline = vm -> blackline;
	    DropMenu (0);
	    if (blackline >= 0 && (HitMenu = vm -> m)) {
		register struct item   *p = &HitMenu -> item[blackline];
		register struct Window *w = InitialWindow;
		CurrentViewPort = &w -> SubViewPort;
		if (p -> function)
		    (p -> function) (p -> name);
		if (p -> string && w) {
		    if (w -> MenuPrefix[0])
			write (w -> SubProcess, w -> MenuPrefix + 1, w -> MenuPrefix[0]);
		    write (w -> SubProcess, p -> string, strlen (p -> string));
		}
	    }
	}
	if (StackedMenus && WasInsideAMenu) {
	    MouseX = OldMouseX;
	    MouseY = OldMouseY;
	}
	MenuActive = 0;
	RestoreToCurrentCursor ();
	NMenusVisible = 0;
	m = 0;
    }
}

static InRawMode;
static struct sgttyb sgold;

CleanupScreen () {
    if (InRawMode) {
	static int  OFF = 0;
	ioctl (0, FIONBIO, &OFF);
	ioctl (0, TIOCSETP, &sgold);
    }
}

RawMode () {
    if (!InRawMode) {
	struct sgttyb    sgnew;
	ioctl (0, TIOCGETP, &sgold);
	sgnew = sgold;
	sgnew.sg_flags |= /* CBREAK */ RAW;
	sgnew.sg_flags &= ~ECHO;
	ioctl (0, TIOCSETP, &sgnew);
	InRawMode = 1;
    }
}

char *GetStrAt (x, y, width, height, prompt, init)
char   *prompt; {
    int     ClearFrom = x;
    int     stx = x + 1;
    static char buf[200];
    int     len = 0;
    int     dot = 0;
    int     running = 1;
    HW_SelectColor (f_white);
    HW_RasterSmash (x, y, width, height);
    HW_SelectColor (f_CharacterContext|f_black);
    if (prompt)
	stx = HW_DrawString (stx, y, mfont, prompt, 0, 0, 0);
    if (init) {
	strcpy (buf, init);
	dot = len = strlen (buf);
    }
    buf[len] = 0;
    RawMode ();
    while (running) {
	register char c;
	int     endx;
	c = buf[dot];
	buf[dot] = 0;
	HW_SelectColor (f_CharacterContext|f_black);
	endx = HW_DrawString (stx, y, mfont, buf, 0, 0, 0);
	buf[dot] = c;
	HW_SelectColor (f_black);
	HW_vector (endx, y, endx, y+mfont->newline.y-1);
	HW_SelectColor (f_CharacterContext|f_black);
	endx = HW_DrawString (endx+1, y, mfont, buf+dot, 0, 0, 0);
	if (endx < ClearFrom) {
	    HW_SelectColor (f_white);
	    HW_RasterSmash (endx, y, ClearFrom - endx, height);
	}
	ClearFrom = endx;
	switch (c = getchar () & 0177) {
	    case '\r': 
	    case '\n': 
	    case 0: 
	    case 033: 
		running = 0;
		break;
	    case 0177: 
	    case '\b': 
		if (dot) {
		    dot--;
		    len--;
		    strcpy (buf + dot, buf + dot + 1);
		}
		break;
	    case 'b' & 037: 
		if (dot)
		    dot--;
		break;
	    case 'f' & 037: 
		if (dot < len)
		    dot++;
		break;
	    case 'a' & 037: 
		dot = 0;
		break;
	    case 'e' & 037: 
		dot = len;
		break;
	    case 'c' & 037: 
	    case 'g' & 037: 
		return 0;
	    case 't' & 037: 
		if (dot >= 2) {
		    register char   t = buf[dot - 1];
		    buf[dot - 1] = buf[dot - 2];
		    buf[dot - 2] = t;
		} break;
	    default: 
		if (c < ' ')
		    c = ' ';
		{
		    register    i = len;
		    while (i >= dot) {
			buf[i + 1] = buf[i];
			i--;
		    }
		}
		buf[dot] = c;
		dot++;
		len++;
		break;
	}
    }
    return buf;
}

char *GetStr (prompt, init)
char   *prompt;
char   *init; {
    return GetStrAt (0, CurrentViewPort->height - mfont->newline.y,
	    CurrentViewPort->width, mfont->newline.y,
	    prompt, init);
}
