/***********************************************************************\
* 								        *
* 	File: menu.h						        *
* 	Copyright (c) 1984 IBM					        *
* 	Date: Thu May 17 10:03:38 1984				        *
* 	Author: James Gosling					        *
* 								        *
* Definitions for the window managers internal manipulations of menus.  *
* 								        *
\***********************************************************************/


struct font *mfont;
struct menu *LManagersMenu;
int StackedMenus;

struct menu {
    int     width,
            height;
    char   *title;
    int     twidth;
    int     nitems;
    int     itemspace;
    int     LastPosition;
    struct item *item;
};

struct item {
    char        *name;
    int		width;	/* width of name */
    char        *string;
    int        (*function) ();
    struct menu *menu;
    short	priority;
    short	flags;
    char        *deletekey;
};

/* Menu flags */
#define mf_Dangerous	1
#define mf_UnUsable	2
#define mf_Greyable	3		/* Don't delete: grey instead */

struct menu *AddMenu();

struct menu *defmenua (/* "name", "ItemName", ItemFunc, N,F,...  ,0 */);
int PostMenu (/* Menu */);
int getmenu ();

int MenuActive;

/* Mouse button definitions */
#define LeftButton	2
#define MiddleButton	1
#define RightButton	0
#define UpMovement	0
#define DownTransition	1
#define UpTransition	2
#define DownMovement	3

struct Window *Reshapee;
int Reshapee_x, Reshapee_y;
