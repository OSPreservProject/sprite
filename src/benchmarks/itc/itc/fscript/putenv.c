/* Assign a new value to an environment variable.
   James Gosling, 2/9/84 */

putenv (var, val)
char   *var,
       *val; {
    int     VarLen = strlen (var);
    int     ValLen = strlen (val);
    char    buf[500];
    extern char **environ;
    register char **p;
    for (p = environ; *p; p++)
	if ((*p)[VarLen] == '=' && strncmp (*p, var, VarLen) == 0)
	    break;
    if (*p == 0) {
	char  **ne = (char **) malloc (sizeof (char *) * ((p - environ) + 2));
	register char **np;
	for (p = ne, np = environ; *np;)
	    *p++ = *np++;
	p[1] = 0;
	environ = ne;
    }
    sprintf (buf, "%s=%s", var, val);
    *p = (char *) malloc (VarLen + ValLen + 2);
    strcpy (*p, buf);
}
