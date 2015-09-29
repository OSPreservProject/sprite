
/*	@(#)token.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

extern char	**token;
extern char	*tokens[128];
extern char	*testname;

#define NULL		((char *) 0)
#define DEFAULT		'.'
#define FOREVER		'*'
#define SEPARATOR	';'

struct menu {
	char	t_char, *t_name;
	int	(*t_call)();
	char	*t_help;
};
