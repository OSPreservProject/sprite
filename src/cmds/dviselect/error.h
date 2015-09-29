/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

char	*HelpMsg;		/* the current help message, if any */
int	UseErrHelp;		/* true iff HelpMsg makes sense */
int	ErrHistory;		/* the error history */
int	Interaction;		/* the interaction level */
int	ErrCount;		/* total errors since last paragraph */
int	DeletionsAllowed;	/* true iff user is allowed to delete tokens
				   in the error recovery code */

/* values for ErrHistory, in increasing order of severity */
#define EH_Spotless	0	/* no errors */
#define EH_Warning	1	/* gave a warning (diagnostic) */
#define EH_ErrMessage	2	/* gave an error message */
#define EH_FatalErr	3	/* died on a fatal error */

/* values for Interaction */
#define IA_ErrStop	0	/* stop on error */
#define IA_ErrScroll	1	/* do not stop on error */
#define IA_NonStop	2	/* do not ever stop */
#define IA_Batch	3	/* neither stop nor produce tty output */

/* pick an output descriptor; there is no log file right now */
#define NoLogErrFD()	(Interaction == IA_Batch ? OutIgnore : OutTerm)
