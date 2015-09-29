/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/* imagen globals */

/* Accounting: */
#ifndef ACCOUNT_FILE
/*
#define ACCOUNT_FILE "/usr/adm/imagen_acct"/* if defined, the name of the
					      imagen page accounting file */
#endif	ACCOUNT_FILE

#define MaxImFamily	128	/* hardware limitation on font family index */
#define DefaultDPI	300	/* 300 for Imagen 8/300, 240 for Imprint-10 */

#define DefaultMaxDrift	3	/* default value for MaxDrift */

/* Default margins, in dots */
/* CRUFT ALERT: depending on DPI variable */
#define DefaultTopMargin   (DPI)	/* 1" margins */
#define DefaultLeftMargin  (DPI)
