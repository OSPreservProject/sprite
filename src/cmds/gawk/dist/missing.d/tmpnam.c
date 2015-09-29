/*
 * tmpnam - an implementation for systems lacking a library version
 *	    this version does not rely on the P_tmpdir and L_tmpnam constants.
 */

#ifndef NULL
#define NULL	0
#endif

static char template[] = "/tmp/gawkXXXXXX";

char *
tmpnam(tmp)
char *tmp;
{
	static char tmpbuf[sizeof(template)];
	
	if (tmp == NULL) {
		(void) strcpy(tmpbuf, template);
		(void) mktemp(tmpbuf);
		return tmpbuf;
	} else {
		(void) strcpy(tmp, template);
		(void) mktemp(tmp);
		return tmp;
	}
}
