
/*	@(#)dpy.h 1.6 88/02/08 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef sun4
#if defined(SUN2) || defined(sun2)
#include "../sun2/dpy.h"
#endif

#if defined(SUN3) || defined(sun3)
#include "../sun3/dpy.h"
#endif
#endif
