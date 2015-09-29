/* 
 * remque.c --
 *
 *	Source code for the "remque" library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: proto.c,v 1.2 88/03/11 08:39:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

struct qelem {
    struct qelem *q_forw;
    struct qelem *q_back;
    char q_data[4];
}


/*
 *----------------------------------------------------------------------
 *
 * remque --
 *
 *	Remove an element from its queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Elem is removed from the queue it used to be part of.
 *
 *----------------------------------------------------------------------
 */

remque(elem)
    register struct qelem *elem;
{
    elem->q_forw->q_back = elem->q_back;
    elem->q_back->q_forw = elem->q_forw;
}
