/*
 * getkey.c --
 *
 * C code for Unix System V to read single keys from the keyboard
 *
 *---------------------------------------------------------------------------
 * 
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <termio.h>

#include <tcl.h>


/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetKeyCmd --
 *      Implements the TCL getkey command:
 *        getkey [timeout]
 *
 * If no timeout value is specified, waits indefinitely.
 *
 * Results:
 *      Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_GetKeyCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double ftimeout;
    struct termio orig_term_settings;
    struct termio single_char_term_settings;
    unsigned char timeout;
    unsigned char vmin = 0;
    char c;
    int returnval;

    if (argc > 2) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                          " [timeout]", (char *) NULL);
        return TCL_ERROR;
    }

    if (argc == 1) {
	timeout = 0;
	vmin = 1;
    } else {	
        if (Tcl_GetDouble (interp, argv[1], &ftimeout) != TCL_OK)
            return TCL_ERROR;

	if ((ftimeout < 0.0) || (ftimeout > 25.5)) {
            Tcl_AppendResult (interp, "bad timeout value: ", argv [0], 
                              " timeout must be between 0.0 and 25.5",
			      (char *) NULL);
	    return TCL_ERROR;
	}
	timeout = 10.0 * ftimeout;
    }

    /* get their terminal settings, copy them and do a version for input
     * without canonical input processing, so we can timeout input, get
     * stuff without a newline, etc, etc. */
    if (ioctl(0,TCGETA,&orig_term_settings) < 0) goto unixError;

    single_char_term_settings = orig_term_settings; /* structure copy */
    single_char_term_settings.c_iflag &= ~(INLCR|IGNCR|ICRNL|IUCLC);
    single_char_term_settings.c_lflag &= ~(ICANON|ECHO|ISIG);
    single_char_term_settings.c_cc[VMIN] = vmin;
    single_char_term_settings.c_cc[VTIME] = timeout;
    single_char_term_settings.c_cc[VINTR] = 0377;

    if (ioctl(0,TCSETA,&single_char_term_settings) < 0) goto unixError;

    returnval = read(0,&c,1);
    if (returnval > 0) sprintf(interp->result, "%c", c);

    if (ioctl(0,TCSETA,&orig_term_settings)) goto unixError;
    return TCL_OK;

unixError:
    Tcl_AppendResult (interp, argv[0], ": ", 
                      Tcl_UnixError (interp), (char *) NULL);
    return TCL_ERROR;
}

