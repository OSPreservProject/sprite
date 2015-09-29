#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "ctype.h"
#include "util.h"

char ProgramName[];
static char *progname, *varname, *value;
static char inbuf[300];
static char *profile;
char **environ;

char FoldTRT[256] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, '!', '"', '#', '$', '%',
'&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4',
'5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'a', 'b', 'c',
'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^', '_', '`', 'a',
'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127,
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, '!', '"', '#', '$', '%',
'&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4',
'5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'a', 'b', 'c',
'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^', '_', '`', 'a',
'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127, };

FoldedEQ(s1, s2)
register unsigned char	*s1,
			*s2; {
    while (*s1)
	if (FoldTRT[*s1++] != FoldTRT[*s2++])
	    return 0;
    return * s2 == 0;
}

static
char *parseprofile (p, copyvalue)
register char  *p; {
    register char  *d;
    register char   c;
    if (p == 0)
	return 0;
    while (1) {
	if (*p == 0)
	    return 0;
	if (*p == '\n')
	    p++;
	else
	    if (*p == '#')
		while (*p && *p++ != '\n');
	    else
		break;
    };
    d = inbuf;
    progname = 0;
    varname = d;
    while ((c = *p) && c != '\n' && c != '=' && c != '.' && c != ':')
	if (isspace (c))
	    p++;
	else
	    *d++ = *p++;
    p++;
    *d++ = 0;
    if (c == '.') {
	progname = varname;
	varname = d;
	while ((c = *p) && c != ':' && c != '\n' && c != '=')
	    if (isspace (c))
		p++;
	    else
		*d++ = *p++;
	p++;
	*d++ = 0;
    }
    if (c == 0)
	value = 0;
    else {
	while (isspace (*p))
	    p++;
	if (copyvalue) {
	    value = d;
	    while ((c = *p++) && c != '\n')
		*d++ = c;
	    *d++ = 0;
	}
	else
	    value = p;
    }
    return p;
}

static
char *openprofile () {
    if (profile == 0) {
	char    fn[100];
	int     fd;
	struct stat st;
	static  hasprofile = 1;
	if (!hasprofile)
	    return 0;
	sprintf (fn, "%s/preferences", getenv ("HOME"));
	if ((fd = open (fn, 0)) <= 0) {
	    hasprofile = 0;
	    return 0;
	}
	fstat (fd, &st);
	profile = (char *) malloc (st.st_size + 1);
	if (read (fd, profile, st.st_size) != st.st_size) {
	    hasprofile = 0;
	    close (fd);
	    return 0;
	}
	close (fd);
    }
    return profile;
}

char *getprofile (var)
char *var; {
    char   *profp = openprofile ();
    int     seendesired = 1;
    register char **e = environ;
    while (*e)
	if (parseprofile (*e++, 0))
	    if ((!progname || FOLDEDEQ (progname, ProgramName))
		    && FOLDEDEQ (varname, var))
		return value;
    while (profp = parseprofile (profp, 1)) {
	if (progname)
	    seendesired = FOLDEDEQ (progname, ProgramName)
		|| FOLDEDEQ (progname, "*");
	if (seendesired && FOLDEDEQ (var, varname))
	    return value;
    }
    return 0;
}

getprofileswitch (var, DefaultValue)
char   *var; {
    char   *val;
    register    len;
    static struct keys {
	char   *name;
	int     value;
    }                   keys[] = {
	                    "true", 1,
	                    "false", 0,
	                    "on", 1,
	                    "off", 0,
	                    "yes", 1,
	                    "no", 0,
	                    "1", 1,
	                    "0", 0,
	                    0, 0
    };
    register struct keys   *p;
    if (var && (val = getprofile (var))) {
	len = strlen (val);
	for (p = keys; p -> name; p++)
	    if (p -> name[0] == val[0] && strncmp (p -> name, val, len) == 0)
		return p -> value;
    }
    return DefaultValue;
}

getprofileint (var, DefaultValue)
char   *var; {
    register char  *val;
    register    n = 0;
    register    neg = 0;
    if (var == 0 || (val = getprofile (var)) == 0)
	return DefaultValue;
    while (*val) {
	if (*val == '-')
	    neg = ~neg;
	else
	    if (*val != ' ' && *val != '\t')
		if ('0' <= *val && *val <= '9')
		    n = n * 10 + *val - '0';
		else
		    return DefaultValue;
	val++;
    }
    return neg ? -n : n;
}
