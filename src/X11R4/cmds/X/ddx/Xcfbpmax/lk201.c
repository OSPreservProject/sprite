/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/
/* $XConsortium: lk201.c,v 1.33 89/10/09 14:52:37 keith Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "keynames.h"
#include "keysym.h"
#include "DECkeysym.h"

#ifndef KEY_ESC
#define KEY_ESC KEY_F11
#endif

/* This file is device dependent, but is common to several devices */

#include <sys/types.h>
#include "inputstr.h"

#define KEYDOWN_ERROR	0x3d
#define POWERUP_ERROR 	0x3e
#define BASEKEY		0x41
#define MINSPECIAL	0xb3
#define ALLUPS		0xb3
#define METRONOME	0xb4
#define OUTPUT_ERROR	0xb5
#define INPUT_ERROR 	0xb6
#define MAXSPECIAL	0xba

static u_char lastkey;

#define NUMDIVS 14
static u_char divbeg[NUMDIVS] = {0xbf, 0x91, 0xbc, 0xbd, 0xb0, 0xad, 0xa6,
				 0xa9, 0x88, 0x56, 0x63, 0x6f, 0x7b, 0x7e};
static u_char divend[NUMDIVS] = {0xff, 0xa5, 0xbc, 0xbe, 0xb2, 0xaf, 0xa8,
				 0xac, 0x90, 0x62, 0x6e, 0x7a, 0x7d, 0x87};
/* initially set for keyboard defaults */
static unsigned int keymodes[8] = {0, 0, 0, 0, 0, 0x0001c000, 0, 0};
	/* down/up keys */
static unsigned int keys[8]; /* down/up keys that are currently down */

/* Handle keyboard input from LK201 */

/* Whatever key is associated with the Lock modifier is treated as a toggle key.
 * The code assumes that such keys are always in up/down mode.
 */

void
ProcessLK201Input (e, pdev)
	register xEvent *e;
	DevicePtr pdev;
{
    register unsigned int   key, bits;
    register	      int   idx;
    DeviceIntPtr	    dev = (DeviceIntPtr) pdev;

    key = e->u.u.detail;
    if (key > MAXSPECIAL || (key >= BASEKEY && key < MINSPECIAL))
    {
	lastkey = key;
	idx = key >> 5;
	key &= 0x1f;
	key = 1 << key;
	if (keymodes[idx] & key)	/* an up down type */
	{
	    if ((keys[idx] ^= key) & key)
		e->u.u.type = KeyPress;
	    else
		e->u.u.type = KeyRelease;
	    if (dev->key->modifierMap[lastkey] & LockMask)
	    {
		if (e->u.u.type == KeyRelease)
		    return;
		if (BitIsOn (dev->key->down, lastkey))
		{
		    e->u.u.type = KeyRelease;
		    SetLockLED (FALSE);
		}
		else
		    SetLockLED (TRUE);
	    }
	    (*pdev->processInputProc)(e, dev, 1);
	}
	else
	{
	    e->u.u.type = KeyPress;
	    (*pdev->processInputProc)(e, dev, 1);
	    e->u.u.type = KeyRelease;
	    (*pdev->processInputProc)(e, dev, 1);
	}
    }
    else
    {
	switch (key)
	{
	    case METRONOME: 
		e->u.u.type = KeyPress;
		e->u.u.detail = lastkey;
		(*pdev->processInputProc)(e, dev, 1);
		e->u.u.type = KeyRelease;
		(*pdev->processInputProc)(e, dev, 1);
		break;
	    case ALLUPS: 
		idx = 7;
		e->u.u.type = KeyRelease;
		do
		{
		    if (bits = keys[idx])
		    {
			keys[idx] = 0;
			key = 0;
			do
			{
			    if (bits & 1)
			    {
				e->u.u.detail = (idx << 5) | key;
				if (!(dev->key->modifierMap[e->u.u.detail] & LockMask))
				    (*pdev->processInputProc)(e, dev, 1);
			    }
			    key++;
			} while (bits >>= 1);
		    }
		} while (--idx >= 0);
		break;
	    case POWERUP_ERROR: 
	    case KEYDOWN_ERROR: 
	    case OUTPUT_ERROR: 
	    case INPUT_ERROR: 
	    /* 	    Warning ("keyboard error");    XXX */
		break;
	}
    }
}

/* Put keyboard in autorepeat mode and return control command string.
 * autorepeat/down: main keyboard, numeric keypad, delete, cursors
 * up/down: all others
 */

static void
ResetLKModes (modes)
	register int modes;
{
	register int i = 0;
	register int key, last;

	bzero ((caddr_t) keymodes, sizeof (keymodes));
	do {
	    if (modes & 1) {
		for (key = divbeg[i], last = divend[i]; key <= last; key++)
		    keymodes[key >> 5] |= 1 << (key & 0x1f);
	    }
	    modes >>= 1;
	} while (++i < NUMDIVS);
}

char *
AutoRepeatLKMode ()
{
	ResetLKModes (0x3f38);
	return ("\212\222\232\246\256\266\272\302\316\326\336\346\356\366");
}

/* Put all of keyboard in down/up mode and return control command string */

char *
UpDownLKMode ()
{
	ResetLKModes (0x3fff);
	return ("\216\226\236\246\256\266\276\306\316\326\336\346\356\366");
}

Bool
LegalModifier(key)
    BYTE key;
{
    if ((key == KEY_LOCK)
     || (key == KEY_SHIFT)
     || (key == KEY_COMPOSE)
     || (key == KEY_CTRL))
        return TRUE;
    return FALSE;
}

Bool
GetLK201Mappings(pKeySyms, pModMap)
    KeySymsPtr pKeySyms;
    CARD8 *pModMap;
{
#define INDEX(in) ((in - MIN_LK201_KEY) * LK201_GLYPHS_PER_KEY)
    int i;
    KeySym *map;

    map = (KeySym *)xalloc(sizeof(KeySym) * 
				    (MAP_LENGTH * LK201_GLYPHS_PER_KEY));
    if (!map)
	return FALSE;

    for (i = 0; i < MAP_LENGTH; i++)
	pModMap[i] = NoSymbol;	/* make sure it is restored */
    pModMap[ KEY_LOCK ] = LockMask;
    pModMap[ KEY_SHIFT ] = ShiftMask;
    pModMap[ KEY_CTRL ] = ControlMask;
    pModMap[ KEY_COMPOSE ] = Mod1Mask;

    pKeySyms->minKeyCode = MIN_LK201_KEY;
    pKeySyms->maxKeyCode = MAX_LK201_KEY;
    pKeySyms->mapWidth = LK201_GLYPHS_PER_KEY;
    pKeySyms->map = map;

    for (i = 0; i < (MAP_LENGTH * LK201_GLYPHS_PER_KEY); i++)
	map[i] = NoSymbol;	/* make sure it is restored */

    map[INDEX(KEY_F1)] = XK_F1;
    map[INDEX(KEY_F2)] = XK_F2;
    map[INDEX(KEY_F3)] = XK_F3;
    map[INDEX(KEY_F4)] = XK_F4;
    map[INDEX(KEY_F5)] = XK_F5;
    map[INDEX(KEY_F6)] = XK_F6;
    map[INDEX(KEY_F7)] = XK_F7;
    map[INDEX(KEY_F8)] = XK_F8;
    map[INDEX(KEY_F9)] = XK_F9;
    map[INDEX(KEY_F10)] = XK_F10;
    map[INDEX(KEY_F11)] = XK_F11;
    map[INDEX(KEY_F12)] = XK_F12;
    map[INDEX(KEY_F13)] = XK_F13;
    map[INDEX(KEY_F14)] = XK_F14;

    map[INDEX(KEY_HELP)] = XK_Help;
    map[INDEX(KEY_MENU)] = XK_Menu;

    map[INDEX(KEY_F17)] = XK_F17;
    map[INDEX(KEY_F18)] = XK_F18;
    map[INDEX(KEY_F19)] = XK_F19;
    map[INDEX(KEY_F20)] = XK_F20;

    map[INDEX(KEY_FIND)] = XK_Find;
    map[INDEX(KEY_INSERT_HERE)] = XK_Insert;
    map[INDEX(KEY_REMOVE)] = DXK_Remove;
    map[INDEX(KEY_SELECT)] = XK_Select;
    map[INDEX(KEY_PREV_SCREEN)] = XK_Prior;
    map[INDEX(KEY_NEXT_SCREEN)] = XK_Next;

    map[INDEX(KEY_KP_0)] = XK_KP_0;
    map[INDEX(KEY_KP_PERIOD)] = XK_KP_Decimal;
    map[INDEX(KEY_KP_ENTER)] = XK_KP_Enter;
    map[INDEX(KEY_KP_1)] = XK_KP_1;
    map[INDEX(KEY_KP_2)] = XK_KP_2;
    map[INDEX(KEY_KP_3)] = XK_KP_3;
    map[INDEX(KEY_KP_4)] = XK_KP_4;
    map[INDEX(KEY_KP_5)] = XK_KP_5;
    map[INDEX(KEY_KP_6)] = XK_KP_6;
    map[INDEX(KEY_KP_COMMA)] = XK_KP_Separator;
    map[INDEX(KEY_KP_7)] = XK_KP_7;
    map[INDEX(KEY_KP_8)] = XK_KP_8;
    map[INDEX(KEY_KP_9)] = XK_KP_9;
    map[INDEX(KEY_KP_HYPHEN)] = XK_KP_Subtract;
    map[INDEX(KEY_KP_PF1)] = XK_KP_F1;
    map[INDEX(KEY_KP_PF2)] = XK_KP_F2;
    map[INDEX(KEY_KP_PF3)] = XK_KP_F3;
    map[INDEX(KEY_KP_PF4)] = XK_KP_F4;

    map[INDEX(KEY_LEFT)] = XK_Left;
    map[INDEX(KEY_RIGHT)] = XK_Right;
    map[INDEX(KEY_DOWN)] = XK_Down;
    map[INDEX(KEY_UP)] = XK_Up;

    map[INDEX(KEY_SHIFT)] = XK_Shift_L;
    map[INDEX(KEY_CTRL)] = XK_Control_L;
    map[INDEX(KEY_LOCK)] = XK_Caps_Lock;
    map[INDEX(KEY_COMPOSE)] = XK_Multi_key;
    map[INDEX(KEY_COMPOSE)+1] = XK_Meta_L;
    map[INDEX(KEY_DELETE)] = XK_Delete;
    map[INDEX(KEY_RETURN)] = XK_Return;
    map[INDEX(KEY_TAB)] = XK_Tab;

    map[INDEX(KEY_TILDE)] = XK_quoteleft;
    map[INDEX(KEY_TILDE)+1] = XK_asciitilde;

    map[INDEX(KEY_TR_1)] = XK_1;                 
    map[INDEX(KEY_TR_1)+1] = XK_exclam;
    map[INDEX(KEY_Q)] = XK_Q;
    map[INDEX(KEY_A)] = XK_A;
    map[INDEX(KEY_Z)] = XK_Z;

    map[INDEX(KEY_TR_2)] = XK_2;
    map[INDEX(KEY_TR_2)+1] = XK_at;

    map[INDEX(KEY_W)] = XK_W;
    map[INDEX(KEY_S)] = XK_S;
    map[INDEX(KEY_X)] = XK_X;

    map[INDEX(KEY_LANGLE_RANGLE)] = XK_less;
    map[INDEX(KEY_LANGLE_RANGLE)+1] = XK_greater;

    map[INDEX(KEY_TR_3)] = XK_3;
    map[INDEX(KEY_TR_3)+1] = XK_numbersign;

    map[INDEX(KEY_E)] = XK_E;
    map[INDEX(KEY_D)] = XK_D;
    map[INDEX(KEY_C)] = XK_C;

    map[INDEX(KEY_TR_4)] = XK_4;
    map[INDEX(KEY_TR_4)+1] = XK_dollar;

    map[INDEX(KEY_R)] = XK_R;
    map[INDEX(KEY_F)] = XK_F;
    map[INDEX(KEY_V)] = XK_V;
    map[INDEX(KEY_SPACE)] = XK_space;

    map[INDEX(KEY_TR_5)] = XK_5;
    map[INDEX(KEY_TR_5)+1] = XK_percent;

    map[INDEX(KEY_T)] = XK_T;
    map[INDEX(KEY_G)] = XK_G;
    map[INDEX(KEY_B)] = XK_B;

    map[INDEX(KEY_TR_6)] = XK_6;
    map[INDEX(KEY_TR_6)+1] = XK_asciicircum;

    map[INDEX(KEY_Y)] = XK_Y;
    map[INDEX(KEY_H)] = XK_H;
    map[INDEX(KEY_N)] = XK_N;

    map[INDEX(KEY_TR_7)] = XK_7;
    map[INDEX(KEY_TR_7)+1] = XK_ampersand;

    map[INDEX(KEY_U)] = XK_U;
    map[INDEX(KEY_J)] = XK_J;
    map[INDEX(KEY_M)] = XK_M;

    map[INDEX(KEY_TR_8)] = XK_8;
    map[INDEX(KEY_TR_8)+1] = XK_asterisk;

    map[INDEX(KEY_I)] = XK_I;
    map[INDEX(KEY_K)] = XK_K;

    map[INDEX(KEY_COMMA)] = XK_comma;
    map[INDEX(KEY_COMMA)+1] = XK_less;

    map[INDEX(KEY_TR_9)] = XK_9;
    map[INDEX(KEY_TR_9)+1] = XK_parenleft;

    map[INDEX(KEY_O)] = XK_O;
    map[INDEX(KEY_L)] = XK_L;

    map[INDEX(KEY_PERIOD)] = XK_period;
    map[INDEX(KEY_PERIOD)+1] = XK_greater;

    map[INDEX(KEY_TR_0)] = XK_0;
    map[INDEX(KEY_TR_0)+1] = XK_parenright;

    map[INDEX(KEY_P)] = XK_P;

    map[INDEX(KEY_SEMICOLON)] = XK_semicolon;
    map[INDEX(KEY_SEMICOLON)+1] = XK_colon;

    map[INDEX(KEY_QMARK)] = XK_slash;   
    map[INDEX(KEY_QMARK)+1] = XK_question;

    map[INDEX(KEY_PLUS)] = XK_equal;
    map[INDEX(KEY_PLUS)+1] = XK_plus;

    map[INDEX(KEY_RBRACE)] = XK_bracketright;
    map[INDEX(KEY_RBRACE)+1] = XK_braceright;

    map[INDEX(KEY_VBAR)] = XK_backslash;
    map[INDEX(KEY_VBAR)+1] = XK_bar;

    map[INDEX(KEY_UBAR)] = XK_minus;
    map[INDEX(KEY_UBAR)+1] = XK_underscore;

    map[INDEX(KEY_LBRACE)] = XK_bracketleft;
    map[INDEX(KEY_LBRACE)+1] = XK_braceleft;

    map[INDEX(KEY_QUOTE)] = XK_quoteright;
    map[INDEX(KEY_QUOTE)+1] = XK_quotedbl;

    map[INDEX(KEY_ESC)] = XK_Escape;

    return TRUE;
#undef INDEX
}
