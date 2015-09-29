/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)strings.h	5.3 (Berkeley) 11/18/87
 */

#ifndef _STRINGS
#define _STRINGS

/*
 * External function definitions
 * for routines described in string(3).
 */
char	*strcat();
char	*strncat();
int	strcmp();
int	strncmp();
int	strcasecmp();
int	strncasecmp();
char	*strcpy();
char	*strncpy();
int	strlen();
char	*index();
char	*rindex();

/* S5 compatibility */
char	*memccpy();
char	*memchr();
int	memcmp();
char	*memcpy();
char	*memset();
char	*strchr();
char	*strdup();
char	*strrchr();
char	*strpbrk();
char	*strsep();
int	strspn();
int	strcspn();
char	*strtok();

/* Routines from ANSI X3J11 */
char	*strerror();

#endif /* _STRINGS */
