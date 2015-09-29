/*
 * getlogin.c --
 *
 *	Emulate the UNIX getlogin() function. Since we have no /etc/utmp,
 *	we have to rely on the USER envariable being set correctly...
 *
 * Copyright (c) 1987 by the Regents of the University of Californai
 * All Rights Reserved
 */
#include <sprite.h>
#include <proc.h>

char *
getlogin()
{
    char	*name;
    extern char *getenv();

    name = getenv("USER");
    return (name);
}
