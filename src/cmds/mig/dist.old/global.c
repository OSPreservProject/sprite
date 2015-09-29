/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	global.c,v $
 * Revision 2.7  91/08/28  11:16:53  jsb
 * 	Replaced ServerProcName with ServerDemux.
 * 	[91/08/13            rpd]
 * 
 * 	Removed Camelot and TrapRoutine support.
 * 	Changed MsgKind to MsgSeqno.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.6  91/06/26  14:39:24  rpd
 * 	Removed InitRoutineName.
 * 	[91/06/26            rpd]
 * 
 * Revision 2.5  91/06/25  10:30:59  rpd
 * 	Added ServerHeaderFileName.
 * 	[91/05/22            rpd]
 * 
 * Revision 2.4  91/02/05  17:54:23  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:54:07  mrt]
 * 
 * Revision 2.3  90/06/19  23:00:47  rpd
 * 	Added UserFilePrefix.
 * 	[90/06/03            rpd]
 * 
 * Revision 2.2  90/06/02  15:04:38  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:10:43  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 *  8-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Use default values for output file names only if they have not
 *	yet been set.
 *
 * 17-Sep-87  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Added GenSymTab
 *
 * 25-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed CamelotPrefix to op_
 *
 * 12-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Initialized CamelotPrefix
 *
 * 10-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed UseRPC to TRUE
 *
 *  3-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed reference to UseThreads.
 *
 * 16-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added MsgType.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include "string.h"
#include "global.h"

boolean_t BeQuiet = FALSE;
boolean_t BeVerbose = FALSE;
boolean_t UseMsgRPC = TRUE;
boolean_t GenSymTab = FALSE;

boolean_t IsKernelUser = FALSE;
boolean_t IsKernelServer = FALSE;

string_t RCSId = strNULL;

string_t SubsystemName = strNULL;
u_int SubsystemBase = 0;

string_t MsgOption = strNULL;
string_t WaitTime = strNULL;
string_t ErrorProc = "MsgError";
string_t ServerPrefix = "";
string_t UserPrefix = "";
string_t ServerDemux = strNULL;

string_t yyinname;

char NewCDecl[] = "(defined(__STDC__) || defined(c_plusplus))";
char LintLib[] = "defined(LINTLIBRARY)";

void
init_global()
{
    yyinname = strmake("<no name yet>");
}

string_t UserFilePrefix = strNULL;
string_t UserHeaderFileName = strNULL;
string_t ServerHeaderFileName = strNULL;
string_t InternalHeaderFileName = strNULL;
string_t UserFileName = strNULL;
string_t ServerFileName = strNULL;

void
more_global()
{
    if (SubsystemName == strNULL)
	fatal("no SubSystem declaration");

    if (UserHeaderFileName == strNULL)
	UserHeaderFileName = strconcat(SubsystemName, ".h");
    else if (streql(UserHeaderFileName, "/dev/null"))
	UserHeaderFileName = strNULL;

    if (UserFileName == strNULL)
	UserFileName = strconcat(SubsystemName, "User.c");
    else if (streql(UserFileName, "/dev/null"))
	UserFileName = strNULL;

    if (ServerFileName == strNULL)
	ServerFileName = strconcat(SubsystemName, "Server.c");
    else if (streql(ServerFileName, "/dev/null"))
	ServerFileName = strNULL;

    if (ServerDemux == strNULL)
	ServerDemux = strconcat(SubsystemName, "_server");
}
