/*	@(#)pr_dblbuf.h 1.4 88/02/08 SMI	*/

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#ifndef pr_dblbuf_DEFINED
#define pr_dblbuf_DEFINED

/* Attributes
   Unless otherwise indicated, attributes can be used for gets and sets.
*/


#define PR_DBL_AVAIL	1	/* AVAIL is get only. */
#define PR_DBL_DISPLAY	2
#define PR_DBL_WRITE	3
#define PR_DBL_READ	4
#define PR_DBL_DISPLAY_DONTBLOCK	5
    /* Similar to PR_DBL_DISPLAY, but DONTBLOCK is for set only. */


/* Attribute values:
*/

#define PR_DBL_EXISTS	1	/* Value for AVAIL only */
#define PR_DBL_A	2
#define PR_DBL_B	3
#define PR_DBL_BOTH	4	/* for PR_DBL_WRITE only. */
#define PR_DBL_NONE	5
    /* can only be returned by pr_dbl_get with PR_DBL_WRITE attribute. */

#endif pr_dblbuf_DEFINED
