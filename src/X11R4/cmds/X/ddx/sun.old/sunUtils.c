/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef	lint
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif

#include "sun.h"

/*
 * Generates & caches a static value that determines whether we are running 
 * in coexistence with the sunwindows, e.g. under overview.
 */

Bool
sunUseSunWindows()
{
#ifdef SUN_WINDOWS
    static Bool sunUseSunWindowsFirstTime = TRUE;
    static Bool useSunWindows;

    if ( sunUseSunWindowsFirstTime ) {
	sunUseSunWindowsFirstTime = FALSE;
	useSunWindows = ( getenv("WINDOW_PARENT") != NULL );
    }
    return( useSunWindows );
#else
    return( FALSE );
#endif SUN_WINDOWS
}


