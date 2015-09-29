
/*

New Tcl debugger

    tcl proc gets executed by trace routine.

    trace is turned off while tcl proc is being executed.

    result of tcl proc, or via some control mechanism,
    options will include "step in" (set trace depth higher),
    "step", "stop" and "continue".  also it would be nice
    to be able to change an arg, print vars, stuff like that.

can add global to disable tracing so prompt won't be traced.

see if there's a proc line number in the interpreter structure


add a maxlevel where trace returns quickly if a maxlevel is exceeded.
This allows single stepping without step-in, step-in, etc, by playing
with the value.

look at return from the eval in the trace procedure as a means of
determining whether to step or whatever, or maybe control it through
a command or variable.

*/


/*
 * ndebug.c --
 *
 * Tcl debugger.
 *---------------------------------------------------------------------------
 * Copyright 1992 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "tclExtdInt.h"

/*
 * Clientdata structure for trace commands.
 */
#define ARG_TRUNCATE_SIZE 40
#define CMD_TRUNCATE_SIZE 60

struct traceInfo_t {
    Tcl_Interp *interp;
    Tcl_Trace   traceHolder;
    int         depth;
    int         depthFloor;
    };
typedef struct traceInfo_t *traceInfo_pt;

static void
TraceRoutine _ANSI_ARGS_((ClientData    clientData,
                          Tcl_Interp   *interp,
                          int           level,
                          char         *command,
                          int           (*cmdProc)(),
                          ClientData    cmdClientData,
                          int           argc,
                          char         *argv[]));

static void
CleanUpDebug _ANSI_ARGS_((ClientData clientData));

/*
 *----------------------------------------------------------------------
 *
 * TraceRoutine --
 *  Routine called by Tcl_Eval to trace a command.
 *
 *----------------------------------------------------------------------
 */
/* static void */
void
TraceRoutine (clientData, interp, level, command, cmdProc, cmdClientData, 
              argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           level;
    char         *command;
    int           (*cmdProc)();
    ClientData    cmdClientData;
    int           argc;
    char         *argv[];
{
    traceInfo_pt traceInfoPtr = (traceInfo_pt) clientData;
    int          idx, cmdLen, printLen;
    int          result;
    char         depthText[12];
    char        *stepCommand;
    char        *stepArgs[4];

    static int   inTraceRoutine = 0;

    /* Don't try to trace the trace routine.  (We can't delete and recreate
     * the trace, because we're being called from a for-loop that won't
     * see such changes, i.e. trace routines cannot safely delete traces.
     *
     * Also we do our own should-we-trace-at-this-depth processing rather
     * than letting regular tcl handle it, so that we can change the depth
     * we want without having to delete and recreate the trace.
     */
    if (inTraceRoutine || (level > traceInfoPtr->depth))
	return;
    inTraceRoutine = 1;

    if (traceInfoPtr->depthFloor == -1) {
	traceInfoPtr->depthFloor = level;
	traceInfoPtr->depth = level + 1;
    }

    /* build up arguments to the trace routine */
    sprintf (depthText, "%d", level);

    stepArgs[0] = "trace_step";
    stepArgs[1] = depthText;
    stepArgs[2] = command;
    stepArgs[3] = Tcl_Merge (argc, argv);

    stepCommand = Tcl_Merge (4, stepArgs);

    ckfree (stepArgs[3]);

    result = Tcl_Eval (interp, stepCommand, 0, NULL);
    if ((result != TCL_OK) && (result != TCL_RETURN)) {
	printf("error in trace_step: %s\n", interp->result);
    }

    ckfree (stepCommand);

    inTraceRoutine = 0;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceConCmd --
 *     Implements the TCL trace control command:
 *     tracecon depth [level]
 *     tracecon depthfloor [level]
 *
 * Results:
 *  Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
static int
Tcl_TraceConCmd (clientData, interp, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           argc;
    char        **argv;
{
    traceInfo_pt infoPtr = (traceInfo_pt) clientData;
    int          idx;

    if (argc < 2)
        goto argumentError;

    /*
     * Handle `depth' sub-command.
     */
    if (STREQU (argv[1], "depth")) {
	if (argc == 2) {
            sprintf(interp->result, "%d", infoPtr->depth);
            return TCL_OK;
	}
	if (argc == 3) {
            return (Tcl_GetInt (interp, argv[2], &(infoPtr->depth)));
	}
	goto argumentError;
    }

    if (STREQU (argv[1], "depthfloor")) {
	if (argc == 2) {
            sprintf(interp->result, "%d", infoPtr->depthFloor);
            return TCL_OK;
	}
	if (argc == 3) {
            return (Tcl_GetInt (interp, argv[2], &(infoPtr->depthFloor)));
	}
	goto argumentError;
    }

argumentError:
    Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                      " depth [level]", (char *) NULL);
    return TCL_ERROR;

}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceProcCmd --
 *     Implements the TCL traceproc command:
 *     traceproc procname [arg...]
 *
 * Results:
 *  Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
static int
Tcl_TraceProcCmd (clientData, interp, argc, argv)
    ClientData    clientData;
    Tcl_Interp   *interp;
    int           argc;
    char        **argv;
{
    register Interp *iPtr = (Interp *) interp;
    traceInfo_pt infoPtr = (traceInfo_pt) clientData;
    int          idx;
    char        *commandToBeTraced;
    int          result;

    if (argc < 2) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                          " procname [arg...]", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * If a trace is in progress, delete it now.
     */
    if (infoPtr->traceHolder != NULL) {
        Tcl_DeleteTrace(interp, infoPtr->traceHolder);
        infoPtr->traceHolder = NULL;
    }

    infoPtr->depth = MAXINT;
    infoPtr->depthFloor = -1;
      
    infoPtr->traceHolder = 
        Tcl_CreateTrace (interp, MAXINT, TraceRoutine, 
                         (ClientData)infoPtr);

    commandToBeTraced = Tcl_Merge (argc - 1, &argv[1]);
    result = Tcl_Eval (interp, commandToBeTraced, 0, NULL);
    ckfree (commandToBeTraced);

    Tcl_DeleteTrace (infoPtr->interp, infoPtr->traceHolder);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 *  CleanUpDebug --
 *
 *  Release the client data area when the trace command is deleted.
 *
 *----------------------------------------------------------------------
 */
static void
CleanUpDebug (clientData)
    ClientData clientData;
{
    traceInfo_pt infoPtr = (traceInfo_pt) clientData;

    if (infoPtr->traceHolder != NULL)
        Tcl_DeleteTrace (infoPtr->interp, infoPtr->traceHolder);
    ckfree ((char *) infoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 *  Tcl_InitDebug --
 *
 *  Initialize the TCL debugging commands.
 *
 *----------------------------------------------------------------------
 */
void
Tcl_InitnDebug (interp)
    Tcl_Interp *interp;
{
    traceInfo_pt infoPtr;

    infoPtr = (traceInfo_pt)ckalloc (sizeof (struct traceInfo_t));

    infoPtr->interp=interp;  /* Save just so we can delete traces at the end */
    infoPtr->traceHolder = NULL;
    infoPtr->depth = 0;

    Tcl_CreateCommand (interp, "tracecon", Tcl_TraceConCmd, 
                       (ClientData)infoPtr, CleanUpDebug);

    Tcl_CreateCommand (interp, "traceproc", Tcl_TraceProcCmd, 
                       (ClientData)infoPtr, (void (*)())NULL);
}
