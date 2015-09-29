/*
 * tcl.h --
 *
 *	This header file describes the externally-visible facilities
 *	of the Tcl interpreter.
 *
 * Copyright 1987-1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/tcl/RCS/tcl.h,v 1.46 91/05/29 11:54:48 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _TCL
#define _TCL

/*
 * Definitions that allow this header file to be used either with or
 * without ANSI C features like function prototypes.
 */

#undef _ANSI_ARGS_
#undef const
#if defined(USE_ANSI) && defined(__STDC__)
#   define _ANSI_ARGS_(x)	x
#else
#   define _ANSI_ARGS_(x)	()
#   define const
#endif

/*
 * Miscellaneous declarations (to allow Tcl to be used stand-alone,
 * without the rest of Sprite).
 */

#ifndef NULL
#define NULL 0
#endif

#ifndef _CLIENTDATA
typedef int *ClientData;
#define _CLIENTDATA
#endif

/*
 * Data structures defined opaquely in this module.  The definitions
 * below just provide dummy types.  A few fields are made visible in
 * Tcl_Interp structures, namely those for returning string values.
 * Note:  any change to the Tcl_Interp definition below must be mirrored
 * in the "real" definition in tclInt.h.
 */

typedef struct {
    char *result;		/* Points to result string returned by last
				 * command. */
    int dynamic;		/* Non-zero means result is dynamically-
				 * allocated and must be freed by Tcl_Eval
				 * before executing the next command. */
    int errorLine;		/* When TCL_ERROR is returned, this gives
				 * the line number within the command where
				 * the error occurred (1 means first line). */
} Tcl_Interp;

typedef int *Tcl_Trace;
typedef int *Tcl_CmdBuf;

/*
 * When a TCL command returns, the string pointer interp->result points to
 * a string containing return information from the command.  In addition,
 * the command procedure returns an integer value, which is one of the
 * following:
 *
 * TCL_OK		Command completed normally;  interp->result contains
 *			the command's result.
 * TCL_ERROR		The command couldn't be completed successfully;
 *			interp->result describes what went wrong.
 * TCL_RETURN		The command requests that the current procedure
 *			return;  interp->result contains the procedure's
 *			return value.
 * TCL_BREAK		The command requests that the innermost loop
 *			be exited;  interp->result is meaningless.
 * TCL_CONTINUE		Go on to the next iteration of the current loop;
 *			interp->result is meaninless.
 */

#define TCL_OK		0
#define TCL_ERROR	1
#define TCL_RETURN	2
#define TCL_BREAK	3
#define TCL_CONTINUE	4

#define TCL_RESULT_SIZE 199

/*
 * Flag values passed to Tcl_Eval (see the man page for details;  also
 * see tclInt.h for additional flags that are only used internally by
 * Tcl):
 */

#define TCL_BRACKET_TERM	1

/*
 * Flag value passed to Tcl_RecordAndEval to request no evaluation
 * (record only).
 */

#define TCL_NO_EVAL		-1

/*
 * Flag values passed to Tcl_Return (see the man page for details):
 */

#define TCL_STATIC	0
#define TCL_DYNAMIC	1
#define TCL_VOLATILE	2

/*
 * Operation values passed to Tcl_TraceVar, and also passed back to
 * watchers.  Don't change the values below without checking for overlap
 * with values defined for variable flags in tclInt.h!
 */

#define TCL_TRACE_READS		1
#define TCL_TRACE_WRITES	2
#define TCL_TRACE_DELETES	4

/*
 * Additional flag passed back to variable watchers.  This flag must
 * not overlap any of the TCL_TRACE_* flags defined above or the
 * VAR_* flags defined in tclInt.h.
 */

#define TCL_VARIABLE_UNDEFINED	8

/*
 * Exported Tcl procedures:
 */

typedef int (*Tcl_CmdProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[]));
typedef void (*Tcl_CmdDeleteProc) _ANSI_ARGS_((ClientData clientData));
typedef void (*Tcl_CmdTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int level, char *command, Tcl_CmdProc proc,
	ClientData cmdClientData, int argc, char *argv[]));
typedef char *(*Tcl_VarTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *varName, int global, int flags,
	char *oldValue, char *newValue));

extern void		Tcl_AppendResult();
extern char *		Tcl_AssembleCmd _ANSI_ARGS_((Tcl_CmdBuf buffer,
			    char *string));
extern void		Tcl_AddErrorInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    char *message));
extern char		Tcl_Backslash _ANSI_ARGS_((char *src,
			    int *readPtr));
extern char *		Tcl_Concat _ANSI_ARGS_((int argc, char **argv));
extern Tcl_CmdBuf	Tcl_CreateCmdBuf _ANSI_ARGS_((void));
extern void		Tcl_CreateCommand _ANSI_ARGS_((Tcl_Interp *interp,
			    char *cmdName, Tcl_CmdProc proc,
			    ClientData clientData,
			    Tcl_CmdDeleteProc deleteProc));
extern Tcl_Interp *	Tcl_CreateInterp _ANSI_ARGS_((void));
extern Tcl_Trace	Tcl_CreateTrace _ANSI_ARGS_((Tcl_Interp *interp,
			    int level, void (*proc)(ClientData clientData,
				Tcl_Interp *interp, int level,
				char *command, Tcl_CmdProc proc,
				ClientData cmdClientData, int argc,
				char *argv[]),
			    ClientData clientData));
extern void		Tcl_DeleteCmdBuf _ANSI_ARGS_((Tcl_CmdBuf buffer));
extern void		Tcl_DeleteCommand _ANSI_ARGS_((Tcl_Interp *interp,
			    char *cmdName));
extern void		Tcl_DeleteInterp _ANSI_ARGS_((Tcl_Interp *interp));
extern void		Tcl_DeleteTrace _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Trace trace));
extern int		Tcl_Eval _ANSI_ARGS_((Tcl_Interp *interp, char *cmd,
			    int flags, char **termPtr));
extern int		Tcl_ExprBool _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int *ptr));
extern int		Tcl_ExprDouble _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, double *ptr));
extern int		Tcl_ExprInt _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int *ptr));
extern int		Tcl_ExprString _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string));
extern int		Tcl_GetBoolean _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int *boolPtr));
extern int		Tcl_GetDouble _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, double *doublePtr));
extern int		Tcl_GetInt _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int *intPtr));
extern char *		Tcl_GetVar _ANSI_ARGS_((Tcl_Interp *interp,
			    char *varName, int global));
extern char *		Tcl_Merge _ANSI_ARGS_((int argc, char **argv));
extern char *		Tcl_ParseVar _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, char **termPtr));
extern int		Tcl_RecordAndEval _ANSI_ARGS_((Tcl_Interp *interp,
			    char *cmd, int flags));
extern void		Tcl_Return _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int status));
extern void		Tcl_SetVar _ANSI_ARGS_((Tcl_Interp *interp,
			    char *varName, char *newValue, int global));
extern int		Tcl_SplitList _ANSI_ARGS_((Tcl_Interp *interp,
			    char *list, int *argcPtr, char ***argvPtr));
extern int		Tcl_StringMatch _ANSI_ARGS_((char *string,
			    char *pattern));
extern char *		Tcl_TildeSubst _ANSI_ARGS_((Tcl_Interp *interp,
			    char *name));
extern int		Tcl_TraceVar _ANSI_ARGS_((Tcl_Interp *interp,
			    char *varName, int global, int flags,
			    Tcl_VarTraceProc proc, ClientData clientData));
extern void		Tcl_UnTraceVar _ANSI_ARGS_((Tcl_Interp *interp,
			    char *varName, int global));
extern int		Tcl_VarTraceInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    char *varName, int global,
			    Tcl_VarTraceProc *procPtr,
			    ClientData *clientDataPtr));

/*
 * Built-in Tcl command procedures:
 */

extern int		Tcl_BreakCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_CaseCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_CatchCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ConcatCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ContinueCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ErrorCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_EvalCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ExecCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ExprCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_FileCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ForCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ForeachCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_FormatCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_GlobCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_GlobalCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_HistoryCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_IfCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_InfoCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_IndexCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_LengthCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ListCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_PrintCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ProcCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_RangeCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_RenameCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ReturnCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_ScanCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_SetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_SourceCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_StringCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_TimeCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_TraceCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_UplevelCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
extern int		Tcl_VarEval();

#endif /* _TCL */
