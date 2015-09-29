
/*
 * @(#)getkey.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * getkey.c
 *
 * Keyboard decoding routine for Sun Monitor
 *
 * This module decodes keyboard up/down codes put in a buffer by
 * "keypress", turns them into ASCII, and passes them on to the rest
 * of the world, one at a time.
 *
 * There are currently two options.  You can request up/down codes,
 * in which case we just leave the driving to you; or you can
 * request ASCII codes, in which case we deal with remembering
 * case shifts, Repeat keys, Ascii translations, etc, and you just
 * see the kind of bytes that might come in from a terminal.
 *
 * We default to providing ASCII.
 */


/*
 * #define KEYBARF invokes some consistency checks on the data received,
 * doing printf's if anything is wrong.
 */

#include "../h/keyboard.h"
#include "../h/asyncbuf.h"
#include "../h/globram.h"

extern struct keyboard	*keytables[];
extern char *keystringtab[];

/* 
 * Getkey Initialization
 *
 */
initgetkey()
{
#ifdef KEYBARF
	unsigned char *keyptr = &gp->keyswitch[0];

	while (keyptr < &gp->keyswitch[128]) 
		*keyptr++ = RELEASED;
#endif KEYBARF

	gp->g_shiftmask = 0;
	gp->g_translation = TR_ASCII;
	gp->g_keyrtick = 1000/13;	/* 13 cps repeat rate */
	gp->g_keyrinit = 500;		/* Wait .5 sec before repeating */
	gp->g_keyrkey = IDLEKEY;	/* Nothing happening now */
}


/*
 * This routine fixes up our tables after an Abort, so we won't
 * think we detected spurious ups or downs.  The refresh routine
 * has seen these two keys go down but has not told us about them.
 * We, however, will see them go up, and this lets us know that
 * their upgoings are OK.
 *
 * This routine is called from the refresh routine or the start
 * of the monitor code for an Abort.  If it fails, Aborts won't work.
 */
abortfix()
{

#ifdef KEYBARF
	gp->keyswitch[ABORTKEY1] = PRESSED;
	gp->keyswitch[ABORTKEY2] = PRESSED;
#endif KEYBARF
	/* Tell mainline that keyboard is idle, (even tho it isn't)
	   to avoid its thinking that an autorepeat should occur */
	bput (gp->g_keybuf, IDLEKEY);
}

/*
 * getkey()
 *
 * Returns a key code (if up/down codes being returned),
 * 	a byte of ASCII (if that's requested)
 * 	NOKEY (if no key has been hit).
 */
int getkey ()
{
	register unsigned char keycode, key, entry;
	unsigned char notrepeating;

    while (1) {
	/* Loop til a value-returning key is pressed or buffer is empty */

	if (!bgetp(gp->g_keybuf)) {
		if (gp->g_keyrkey != IDLEKEY) {
			if (gp->g_nmiclock >= gp->g_keyrtime) {
				gp->g_keyrtime = gp->g_keyrtick + gp->g_nmiclock;
				keycode = gp->g_keyrkey;
				notrepeating = 0;  /* We are repeating */
				goto interpretkeycode;
			}
		}
		return (NOKEY); /* Either no autorepeat or no tick yet. */
	}

	bget(gp->g_keybuf, keycode);
	notrepeating = 1;	/* We are not repeating */

interpretkeycode:

	key = KEYOF(keycode);

	if (gp->g_translation == TR_NONE) 
		return (keycode);
	/* The only other value currently supported is TR_ASCII. */

	/* Translate key number and current shifts to an action byte */
	if (gp->g_shiftmask & SHIFTMASK)
		entry = keytables[0]->k_shifted->keymap[key];
	else
		entry = keytables[0]->k_normal ->keymap[key];

	if (notrepeating) {
		register unsigned char enF0;

		enF0 = entry & 0xF0;
		if ((STATEOF(keycode) == PRESSED) &&
/* We might need to add the RESET/IDLE/ERROR entries here.  FIXME? */
		    (NOSCROLL  != entry) &&
		    (SHIFTKEYS != enF0) &&
		    (BUCKYBITS != enF0) ) {
			gp->g_keyrkey = keycode;
			gp->g_keyrtime = gp->g_nmiclock + gp->g_keyrinit;
		} else {
			if (key == KEYOF(gp->g_keyrkey))
				gp->g_keyrkey = IDLEKEY;
		}
	}

	/* If key is going up and is not a reset or shift key, ignore it. */
	if (STATEOF(keycode) != PRESSED) {
		if (keycode == RESETKEY)	/* Reset has key-up bit on */
			entry = RESET;
		else
		     if ((entry&0xF0) != SHIFTKEYS)
			entry = NOP;
	}

	switch (entry >> 4) {

	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		/* Map normal ascii depending on ctrl, capslock */
		if (gp->g_shiftmask & CTRLMASK) entry &= 0x1F;
		if ((gp->g_shiftmask & CAPSMASK) &&
			(entry >= 'a' && entry <= 'z'))
				entry += 'A' - 'a';
		return entry;

	case SHIFTKEYS >> 4:
		gp->g_shiftmask ^= 1 << (entry & 0x0F);
		break;

	case FUNNY >> 4:
		switch (entry) {
		case NOP:
			break;

		case NOSCROLL:
			if (gp->g_shiftmask & CTLSMASK)	goto sendcq;
			else				goto sendcs;

		case CTRLS:
		sendcs:
			gp->g_shiftmask |= CTLSMASK;
			return (('S'-0x40) /* | gp->g_buckybits */);

		case CTRLQ:
		sendcq:
			gp->g_shiftmask &= ~CTLSMASK;
			return (('Q'-0x40) /* | gp->g_buckybits */);

		case IDLE:
		case RESET:
		gotreset:
			gp->g_shiftmask &= keytables[0]->k_idleshifts;
			gp->g_keyrkey = IDLEKEY;	/* Don't repeat */
			break;

		case ERROR:
			printf("Keyboard error detected\n");
			goto gotreset;

/* Remember when adding new entries that, if they should NOT auto-repeat,
they should be put into the IF statement just above this switch block.
*/
		default:
			goto badentry;
		}
		break;

	/*
	 * Remember when adding new entries that, if they should NOT
	 * auto-repeat, they should be put into the IF statement just above
	 * this switch block.
	 */
	default:
	badentry:
		break;
	}
    }
}
