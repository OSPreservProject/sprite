/*-
 * notused.c --
 *	Dummy functions not used for SpriteOS support.
 *	Note: some of these are referenced in dix, so I prefer to have
 *	  dummy stubs rather than go and put #ifdef's into dix
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: ";
#endif lint

void FlushAllOutput() {}
void FlushIfCriticalOutputPending() {}
void SetCriticalOutputPending() {}
void ResetWellKnownSockets() {}
