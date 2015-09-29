/* 
 * insque.c --
 *
 *	Source code for the "insque" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/insque.c,v 1.2 92/11/21 18:25:47 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

struct qelem {
    struct qelem *q_forw;
    struct qelem *q_back;
    char q_data[4];
};


/*
 *----------------------------------------------------------------------
 *
 * insque --
 *
 *	Insert a new element into a queue after a given predecessor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Elem is linked in after pred.
 *
 *----------------------------------------------------------------------
 */

insque(elem, pred)
    register struct qelem *elem;
    register struct qelem *pred;
{
    elem->q_forw = pred->q_forw;
    elem->q_back = pred;
    pred->q_forw = elem;
    elem->q_forw->q_back = elem;
}
