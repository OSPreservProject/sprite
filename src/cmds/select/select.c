/* 
 * select.c --
 *
 *	This is a simple program that outputs the Sx selection on
 *	its standard output.
 *
 * Copyright 1987 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/select/RCS/select.c,v 1.2 92/06/04 17:11:03 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <tk.h>
#include <tcl.h>
#include <X11/Xatom.h>

typedef struct {
    char *string;		/* Contents of selection are
				 * here.  This space is malloc-ed. */
    int bytesAvl;		/* Total number of bytes available
				 * at string. */
    int bytesUsed;		/* Bytes currently in use in string,
				 * not including the terminating
				 * NULL. */
} GetInfo;

static int SelGetProc _ARGS_((ClientData clientData, char *portion));

main()
{
    Tk_Window 	window;
    Tcl_Interp	*interp;
    int		rc;
    GetInfo 	getInfo;

    interp = Tcl_CreateInterp();
    if (interp == NULL) {
	fprintf(stderr, "Couldn't create TCL interpreter\n");
	exit(1);
    }
    window = Tk_CreateMainWindow(interp, NULL, "select");
    if (window == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    Tk_MakeWindowExist(window);
    getInfo.string = (char *) ckalloc(100);
    getInfo.bytesAvl = 100;
    getInfo.bytesUsed = 0;
    rc = Tk_GetSelection(interp, window, XA_STRING, SelGetProc, 
	    (ClientData) &getInfo);
    if (rc != TCL_OK) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    write(1, getInfo.string, getInfo.bytesUsed);
    exit(0);
}

/*
 *--------------------------------------------------------------
 *
 * SelGetProc --
 *
 *	This procedure is invoked to process pieces of the
 *	selection as they arrive during Tk_GetSelection.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Bytes are stored in getInfoPtr->string, which is 
 *	expanded if necessary.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
SelGetProc(clientData, interp, portion)
    ClientData clientData;	/* Information about partially-
				 * assembled result. */
    Tcl_Interp *interp;		/* Interpreter used for error
				 * reporting (not used). */
    char *portion;		/* New information to be appended. */
{
    register GetInfo *getInfoPtr = (GetInfo *) clientData;
    int newLength;

    newLength = strlen(portion) + getInfoPtr->bytesUsed;

    /*
     * Grow the result area if we've run out of space.
     */

    if (newLength >= getInfoPtr->bytesAvl) {
	char *newString;

	getInfoPtr->bytesAvl *= 2;
	if (getInfoPtr->bytesAvl <= newLength) {
	    getInfoPtr->bytesAvl = newLength + 1;
	}
	newString = (char *) ckalloc((unsigned) getInfoPtr->bytesAvl);
	memcpy((char *) newString, (char *) getInfoPtr->string,
		getInfoPtr->bytesUsed);
	ckfree(getInfoPtr->string);
	getInfoPtr->string = newString;
    }

    /*
     * Append the new data to what was already there.
     */

    strcpy(getInfoPtr->string + getInfoPtr->bytesUsed, portion);
    getInfoPtr->bytesUsed = newLength;
    return TCL_OK;
}

