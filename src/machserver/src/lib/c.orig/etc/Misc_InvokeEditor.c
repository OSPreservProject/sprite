/* 
 * InvokeEditor.c --
 *
 *	Source for the Misc_InvokeEditor library function.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/InvokeEditor.c,v 1.2 91/06/03 22:16:16 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <errno.h>
#include <libc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/*
 *----------------------------------------------------------------------
 *
 * Misc_InvokeEditor --
 *
 *	Invoke the user's editor on the given file.
 *
 * Results:
 *	Returns the exit status from the editor.
 *
 * Side effects:
 *	Typically the user edits the named file.  Of course, once the 
 *	user is in the editor, she can do whatever the editor will let 
 *	her get away with.
 *
 *----------------------------------------------------------------------
 */

int
Misc_InvokeEditor(file)
    char *file;			/* name of file to edit */
{
    int pid, w;
    union wait status;
    char *name;			/* simple name of the editor */
    char *editorPath;		/* path to the editor */
    
    editorPath = getenv("EDITOR");
    if (editorPath != NULL ) {
	name = rindex(editorPath, '/');
	if (name != NULL) {
	    ++name;
	} else {
	    name = editorPath;
	}
    } else {
	name = editorPath = "vi";
    }

    pid = vfork();
    if (pid < 0) {
	perror("Can't fork editor");
	return 1;
    } else if (pid == 0) {
	/* 
	 * Make sure we don't give the user an editor with a protected 
	 * user ID.
	 */
	if (getuid() != geteuid()) {
	    (void)seteuid(getuid());
	}
	if (getgid() != getegid()) {
	    (void)setegid(getgid());
	}
	if (strcmp(name, "mx") == 0) {
	    execlp(editorPath, name, "-D", file, NULL);
	} else {
	    execlp(editorPath, name, file, NULL);
	}
	(void)fprintf(stderr, "Can't start %s: %s\n", editorPath,
		      strerror(errno));
	_exit(127);
    }
    while ((w = wait(&status)) != pid && w != -1) {
	;
    }
    return(w == -1 || status.w_retcode);
}
