
/*
 * @(#)keypress.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * keypress.c
 *
 * This routine is called each time a keypress is received.  It should
 * be as fast and safe as possible, since it runs at high interrupt levels.
 * It should just stash the character somewhere and check for aborts.
 * All other processing is done in "getkey".
 */

#include "../sun3/sunmon.h"
#include "../h/globram.h"
#include "../h/keyboard.h"
#include "../h/asyncbuf.h"

#define keybuf		gp->g_keybuf
#define keystate	gp->g_keystate
#define keybid		gp->g_keybid


/*
 * A keypress was received (from a parallel or serial keyboard).
 * Process it, and return zero for normalcy or nonzero to abort.
 */
int
keypress(key)
	register unsigned char key;
{
	switch (keystate) {

	normalstate:
		keystate = NORMAL;

	case NORMAL:
		if (key == ABORTKEY1) {
			keystate = ABORT1; 
			break;
		}
		bput (keybuf, key);
		break;

	case ABORT1:
		if (key == ABORTKEY2) {
			keystate = NORMAL;
			bputclr(keybuf);  /* Clear typeahead */
			abortfix(); /* Let our other half know that
				     these keys really did go down */
			gp->g_insource = INKEYB;  /* Take keyb inp */
			gp->g_outsink = OUTSCREEN;
			return 1;	/* Break out to the monitor */
		} else {
			bput (keybuf, ABORTKEY1);
			goto normalstate;
		}

	case STARTUP:
		if (key == RESETKEY)
			keystate = STARTUP2;
		break;

	case STARTUP2:
		keybid = key;
		bput (keybuf, RESETKEY);
		keystate = NORMAL;
		break;
	}
	return 0;		/* After normalcy, return 0. */
}

