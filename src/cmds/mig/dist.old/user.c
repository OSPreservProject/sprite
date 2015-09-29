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
 * $Log:	user.c,v $
 * Revision 2.8  91/08/28  11:17:34  jsb
 * 	Added MIG_SERVER_DIED.
 * 	[91/08/21            rpd]
 * 	Removed Camelot and TrapRoutine support.
 * 	Changed MsgKind to MsgSeqno.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.7  91/07/31  18:11:31  dbg
 * 	Allow indefinite-length variable arrays.  They may be copied
 * 	either in-line or out-of-line, depending on size.
 * 
 * 	Copy variable-length C Strings with mig_strncpy, to combine
 * 	'strcpy' and 'strlen' operations.
 * 
 * 	New method for advancing request message pointer past
 * 	variable-length arguments.  We no longer have to know the order
 * 	of variable-length arguments and their count arguments.
 * 
 * 	Remove redundant assignments (to msgh_simple, msgh_size) in
 * 	generated code.
 * 	[91/07/17            dbg]
 * 
 * Revision 2.6  91/06/26  14:39:44  rpd
 * 	Removed the dummy user initialization function,
 * 	which was kept for backwards-compatibility.
 * 	[91/06/26            rpd]
 * 
 * Revision 2.5  91/06/25  10:32:22  rpd
 * 	Cast request and reply ports to mach_port_t in KernelUser stubs.
 * 	[91/05/27            rpd]
 * 
 * 	Changed HeaderFileName to UserHeaderFileName.
 * 	Changed WriteVarDecl to WriteUserVarDecl.
 * 	[91/05/23            rpd]
 * 
 * Revision 2.4  91/02/05  17:56:20  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:28  mrt]
 * 
 * Revision 2.3  90/06/19  23:01:20  rpd
 * 	Added UserFilePrefix support.
 * 	[90/06/03            rpd]
 * 
 * Revision 2.2  90/06/02  15:06:03  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:14:40  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 21-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Get name for header file from HeaderFileName, since it can
 *	change.
 *
 *  8-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Added WriteUserIndividual to put each user-side routine in its
 *	own file.
 *
 *  8-Jul-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Declared routines to be mig_external instead of extern,
 *	where mig_external is conditionally defined in <subsystem>.h.
 *	The Avalon folks want to define mig_external to be static
 *	in their compilations because they inlcude the User.c code in
 *	their programs.
 *
 * 23-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed the include of camelot_types.h to cam/camelot_types.h
 *
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added comments for each routine. Called WriteMsgError
 *	for MIG_ARRAY_TOO_LARGE errors.
 *
 * 19-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Change variable-length inline array declarations to use
 *	maximum size specified to Mig.  Make message variable
 *	length if the last item in the message is variable-length
 *	and inline.  Use argMultiplier field to convert between
 *	argument and IPC element counts.
 *
 * 19-Jan-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	In WriteInitRoutine changed reference from reply_port; to reply_port++;
 *	for lint code.
 *
 * 17-Jan-88  David Detlefs (dld) at Carnegie-Mellon University
 *	Modified to produce C++ compatible code via #ifdefs.
 *	All changes have to do with argument declarations.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Handle variable-length inline arrays.
 *
 * 22-Oct-87  Mary Thompson (mrt) at Carnegie-Mellon University
 * 	Added a reference to rep_port in the InitRoutine
 *	with an ifdef lint conditional.
 *
 * 22-Sep-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed check for TransId to be a not equal test
 *	rather than an equal test.
 *
 *  2-Sep-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed WriteCheckIdentity to check TransId instead
 *	of msgh_id for a returned camelot reply
 *
 * 24-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added a  LINTLIBRARY  line to keep lint
 *	from complaining about routines that are not used.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added Flag parameter to WritePackMsgType.
 *
 * 12-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Made various camelot changes: include of camelot_types.h
 *	Check for death_pill before correct msg-id.
 *
 * 10-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Renamed get_reply_port and dealloc_reply_port to
 *	mig_get_reply_port and mig_dealloc_reply_port.
 *	Fixed WriteRequestHead to handle MsgType parameter.
 *
 *  3-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed to generate code that is the same for multi-threaded and
 *	single threaded use. Gets reply port from library routine
 *	get_reply_port and deallocates with routine
 *	dealloc_reply_port. Removed all routines in mig interface code
 *	to keep track of the reply port. The init routine still exists
 *	but does nothing.
 * 
 * 29-Jul_87  Mary Thompson (mrt) at Carnegie-Mellon University
 * 	Fixed call to WriteVarDecl to use correspond to
 *	the changes that were made in that routine.
 *
 * 16-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added write of MsgType to WriteSetMsgTypeRoutine.
 *
 *  8-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed #include of sys/types.h from WriteIncludes.
 *	Changed the KERNEL include from ../h to sys/
 *	Removed extern from WriteUser to make hi-c happy
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <assert.h>

#include <mach/message.h>
#include "write.h"
#include "error.h"
#include "utils.h"
#include "global.h"

/*************************************************************
 *	Writes the standard includes. The subsystem specific
 *	includes  are in <SubsystemName>.h and writen by
 *	header:WriteHeader. Called by WriteProlog.
 *************************************************************/
static void
WriteIncludes(file)
    FILE *file;
{
    if (IsKernelServer)
    {
	/*
	 *	We want to get the user-side definitions of types
	 *	like task_t, ipc_space_t, etc. in mach/mach_types.h.
	 */

	fprintf(file, "#undef\tKERNEL\n");

	if (InternalHeaderFileName != strNULL)
	{
	    register char *cp;

	    /* Strip any leading path from InternalHeaderFileName. */
	    cp = rindex(InternalHeaderFileName, '/');
	    if (cp == 0)
		cp = InternalHeaderFileName;
	    else
		cp++;	/* skip '/' */
	    fprintf(file, "#include \"%s\"\n", cp);
	}
    }

    if (UserHeaderFileName != strNULL)
    {
	register char *cp;

	/* Strip any leading path from UserHeaderFileName. */
	cp = rindex(UserHeaderFileName, '/');
	if (cp == 0)
	    cp = UserHeaderFileName;
	else
	    cp++;	/* skip '/' */
	fprintf(file, "#include \"%s\"\n", cp);
    }

    fprintf(file, "#define EXPORT_BOOLEAN\n");
    fprintf(file, "#include <mach/boolean.h>\n");
    fprintf(file, "#include <mach/kern_return.h>\n");
    fprintf(file, "#include <mach/message.h>\n");
    fprintf(file, "#include <mach/notify.h>\n");
    fprintf(file, "#include <mach/mach_types.h>\n");
    fprintf(file, "#include <mach/mig_errors.h>\n");
    fprintf(file, "#include <mach/msg_type.h>\n");
    if (!IsKernelServer)
    {
	/* note KERNEL is undefined when IsKernelServer */
	fprintf(file, "#ifndef\tKERNEL\n");
	fprintf(file, "#include <strings.h>\n");
	fprintf(file, "#endif\tKERNEL\n");
    }
    fprintf(file, "/* LINTLIBRARY */\n");
    fprintf(file, "\n");
    fprintf(file, "extern mach_port_t mig_get_reply_port();\n");
    fprintf(file, "extern void mig_dealloc_reply_port();\n");
    fprintf(file, "\n");
}

static void
WriteGlobalDecls(file)
    FILE *file;
{
    if (RCSId != strNULL)
	WriteRCSDecl(file, strconcat(SubsystemName, "_user"), RCSId);

    fprintf(file, "#define msgh_request_port\tmsgh_remote_port\n");
    fprintf(file, "#define msgh_reply_port\t\tmsgh_local_port\n");
    fprintf(file, "\n");
}

/*************************************************************
 *	Writes the standard #includes, #defines, and
 *	RCS declaration. Called by WriteUser.
 *************************************************************/
static void
WriteProlog(file)
    FILE *file;
{
    WriteIncludes(file);
    WriteBogusDefines(file);
    WriteGlobalDecls(file);
}

/*ARGSUSED*/
static void
WriteEpilog(file)
    FILE *file;
{
}

static string_t
WriteHeaderPortType(arg)
    argument_t *arg;
{
    if (arg->argType->itInName == MACH_MSG_TYPE_POLYMORPHIC)
	return arg->argPoly->argVarName;
    else
	return arg->argType->itInNameStr;
}

static void
WriteRequestHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    if (rt->rtMaxRequestPos > 0)
	fprintf(file, "\tInP = &Mess.In;\n");

    if (rt->rtSimpleFixedRequest) {
	fprintf(file, "\tInP->Head.msgh_bits =");
	if (!rt->rtSimpleSendRequest)
	    fprintf(file, " MACH_MSGH_BITS_COMPLEX|");
	fprintf(file, "\n");
	fprintf(file, "\t\tMACH_MSGH_BITS(%s, %s);\n",
		WriteHeaderPortType(rt->rtRequestPort),
		WriteHeaderPortType(rt->rtReplyPort));
    } else {
	fprintf(file, "\tInP->Head.msgh_bits = msgh_simple ?\n");
	fprintf(file, "\t\tMACH_MSGH_BITS(%s, %s) :\n",
		WriteHeaderPortType(rt->rtRequestPort),
		WriteHeaderPortType(rt->rtReplyPort));
	fprintf(file, "\t\t(MACH_MSGH_BITS_COMPLEX|\n");
	fprintf(file, "\t\t MACH_MSGH_BITS(%s, %s));\n",
		WriteHeaderPortType(rt->rtRequestPort),
		WriteHeaderPortType(rt->rtReplyPort));
    }

    fprintf(file, "\t/* msgh_size passed as argument */\n");

    /*
     *	KernelUser stubs need to cast the request and reply ports
     *	from ipc_port_t to mach_port_t.
     */

    if (IsKernelUser)
	fprintf(file, "\tInP->%s = (mach_port_t) %s;\n",
		rt->rtRequestPort->argMsgField,
		rt->rtRequestPort->argVarName);
    else
	fprintf(file, "\tInP->%s = %s;\n",
		rt->rtRequestPort->argMsgField,
		rt->rtRequestPort->argVarName);

    if (akCheck(rt->rtReplyPort->argKind, akbUserArg)) {
	if (IsKernelUser)
	    fprintf(file, "\tInP->%s = (mach_port_t) %s;\n",
		    rt->rtReplyPort->argMsgField,
		    rt->rtReplyPort->argVarName);
	else
	    fprintf(file, "\tInP->%s = %s;\n",
		    rt->rtReplyPort->argMsgField,
		    rt->rtReplyPort->argVarName);
    } else if (rt->rtOneWay || IsKernelUser)
	fprintf(file, "\tInP->%s = MACH_PORT_NULL;\n",
		rt->rtReplyPort->argMsgField);
    else
	fprintf(file, "\tInP->%s = mig_get_reply_port();\n",
		rt->rtReplyPort->argMsgField);

    fprintf(file, "\tInP->Head.msgh_seqno = 0;\n");
    fprintf(file, "\tInP->Head.msgh_id = %d;\n",
	    rt->rtNumber + SubsystemBase);
}

/*************************************************************
 *  Writes declarations for the message types, variables
 *  and return  variable if needed. Called by WriteRoutine.
 *************************************************************/
static void
WriteVarDecls(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tunion {\n");
    fprintf(file, "\t\tRequest In;\n");
    if (!rt->rtOneWay)
	fprintf(file, "\t\tReply Out;\n");
    fprintf(file, "\t} Mess;\n");
    fprintf(file, "\n");

    fprintf(file, "\tregister Request *InP = &Mess.In;\n");
    if (!rt->rtOneWay)
	fprintf(file, "\tregister Reply *OutP = &Mess.Out;\n");
    fprintf(file, "\n");

    if (!rt->rtOneWay || rt->rtProcedure)
	fprintf(file, "\tmach_msg_return_t msg_result;\n");

    if (!rt->rtSimpleFixedRequest)
	fprintf(file, "\tboolean_t msgh_simple = %s;\n",
		strbool(rt->rtSimpleSendRequest));
    else if (!rt->rtOneWay &&
	     !(rt->rtSimpleCheckReply && rt->rtSimpleReceiveReply)) {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tboolean_t msgh_simple;\n");
	fprintf(file, "#endif\tTypeCheck\n");
    }

    if (rt->rtNumRequestVar > 0)
	fprintf(file, "\tunsigned int msgh_size;\n");
    else if (!rt->rtOneWay && !rt->rtNoReplyArgs)
    {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tunsigned int msgh_size;\n");
	fprintf(file, "#endif\tTypeCheck\n");
    }

    /* if either request or reply is variable, we need msgh_size_delta */
    if ((rt->rtMaxRequestPos > 0) ||
	(rt->rtMaxReplyPos > 0))
	fprintf(file, "\tunsigned int msgh_size_delta;\n");

    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to call the user provided error procedure
 *  when a MIG error occurs. Called by WriteMsgSend, 
 *  WriteMsgCheckReceive, WriteMsgSendReceive, WriteCheckIdentity,
 *  WriteRetCodeCheck, WriteTypeCheck, WritePackArgValue.
 *************************************************************/
static void
WriteMsgError(file, rt, error)
    FILE *file;
    routine_t *rt;
    char *error;
{
    if (rt->rtProcedure)
	fprintf(file, "\t\t{ %s(%s); return; }\n", rt->rtErrorName, error);
    else if (rt->rtReturn != rt->rtRetCode)
    {
	fprintf(file, "\t\t{ %s(%s); ", rt->rtErrorName, error);
	if (rt->rtNumReplyVar > 0)
	    fprintf(file, "OutP = &Mess.Out; ");
	fprintf(file, "return OutP->%s; }\n", rt->rtReturn->argMsgField);
    }
    else
	fprintf(file, "\t\treturn %s;\n", error);
}

/*************************************************************
 *   Writes the send call when there is to be no subsequent
 *   receive. Called by WriteRoutine for SimpleProcedures
 *   or SimpleRoutines
 *************************************************************/
static void
WriteMsgSend(file, rt)
    FILE *file;
    routine_t *rt;
{
    char *SendSize = (rt->rtNumRequestVar == 0)
			? "sizeof(Request)"
			: "msgh_size";

    char *MsgResult = (rt->rtProcedure)
			? "msg_result ="
			: "return";

    if (IsKernelUser)
    {
	fprintf(file, "\t%s mach_msg_send_from_kernel(", MsgResult);
	fprintf(file, "&InP->Head, %s);\n", SendSize);
    }
    else
    {
	fprintf(file, "\t%s mach_msg(&InP->Head, MACH_SEND_MSG|%s, %s, 0,",
		MsgResult,
		rt->rtMsgOption->argVarName,
		SendSize);
	fprintf(file,
		" MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);\n"
		);
    }

    if (rt->rtProcedure)
    {
	fprintf(file, "\tif (msg_result != MACH_MSG_SUCCESS)\n");
	WriteMsgError(file, rt, "msg_result");
    }
}

/*************************************************************
 *  Writes to code to check for error returns from receive.
 *  Called by WriteMsgSendReceive and WriteMsgRPC
 *************************************************************/
static void
WriteMsgCheckReceive(file, rt, success)
    FILE *file;
    routine_t *rt;
    char *success;
{
    fprintf(file, "\tif (msg_result != %s) {\n", success);
    if (!akCheck(rt->rtReplyPort->argKind, akbUserArg) && !IsKernelUser)
    {
	/* If we aren't using a user-supplied reply port, then
	   deallocate the reply port when it is invalid or
	   for TIMED_OUT errors. */

	fprintf(file, "\t\tif ((msg_result == MACH_SEND_INVALID_REPLY) ||\n");
	if (rt->rtWaitTime != argNULL)
	    fprintf(file, "\t\t    (msg_result == MACH_RCV_TIMED_OUT) ||\n");
	fprintf(file, "\t\t    (msg_result == MACH_RCV_INVALID_NAME))\n");
	fprintf(file, "\t\t\tmig_dealloc_reply_port();\n");
    }
    WriteMsgError(file, rt, "msg_result");
    fprintf(file, "\t}\n");
}

/*************************************************************
 *  Writes the send and receive calls and code to check
 *  for errors. Normally the rpc code is generated instead
 *  although, the subsytem can be compiled with the -R option
 *  which will cause this code to be generated. Called by
 *  WriteRoutine if UseMsgRPC option is false.
 *************************************************************/
static void
WriteMsgSendReceive(file, rt)
    FILE *file;
    routine_t *rt;
{
    char *SendSize = (rt->rtNumRequestVar == 0)
			? "sizeof(Request)"
			: "msgh_size";

    fprintf(file, "\tmsg_result = mach_msg(&InP->Head, MACH_SEND_MSG|%s, %s, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);\n",
	    rt->rtMsgOption->argVarName,
	    SendSize);

    fprintf(file, "\tif (msg_result != MACH_MSG_SUCCESS)\n");
    WriteMsgError(file, rt, "msg_result");
    fprintf(file, "\n");

    fprintf(file, "\tmsg_result = mach_msg(&OutP->Head, MACH_RCV_MSG|%s%s, 0, sizeof(Reply), InP->Head.msgh_local_port, %s, MACH_PORT_NULL);\n",
	    rt->rtMsgOption->argVarName,
	    rt->rtWaitTime != argNULL ? "|MACH_RCV_TIMEOUT" : "",
	    rt->rtWaitTime != argNULL ? rt->rtWaitTime->argVarName : "MACH_MSG_TIMEOUT_NONE");
    WriteMsgCheckReceive(file, rt, "MACH_MSG_SUCCESS");
    fprintf(file, "\n");
}

static void
WriteMsgReceive(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tmsg_result = mach_msg(&OutP->Head, MACH_RCV_MSG|%s%s, 0, sizeof(Reply), %s, %s, MACH_PORT_NULL);\n",
	    rt->rtMsgOption->argVarName,
	    rt->rtWaitTime != argNULL ? "|MACH_RCV_TIMEOUT" : "",
	    rt->rtReplyPort->argVarName,
	    rt->rtWaitTime != argNULL ? rt->rtWaitTime->argVarName : "MACH_MSG_TIMEOUT_NONE");
    WriteMsgCheckReceive(file, rt, "MACH_MSG_SUCCESS");
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes the rpc call and the code to check for errors.
 *  This is the default code to be generated. Called by WriteRoutine
 *  for all routine types except SimpleProcedure and SimpleRoutine.
 *************************************************************/
static void
WriteMsgRPC(file, rt)
    FILE *file;
    routine_t *rt;
{
    char *SendSize = (rt->rtNumRequestVar == 0)
			? "sizeof(Request)"
			: "msgh_size";

    if (IsKernelUser)
	fprintf(file, "\tmsg_result = mach_msg_rpc_from_kernel(&InP->Head, %s, sizeof(Reply));\n", SendSize);
    else
	fprintf(file, "\tmsg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|%s%s, %s, sizeof(Reply), InP->Head.msgh_reply_port, %s, MACH_PORT_NULL);\n",
	    rt->rtMsgOption->argVarName,
	    rt->rtWaitTime != argNULL ? "|MACH_RCV_TIMEOUT" : "",
	    SendSize,
	    rt->rtWaitTime != argNULL? rt->rtWaitTime->argVarName : "MACH_MSG_TIMEOUT_NONE");
    WriteMsgCheckReceive(file, rt, "MACH_MSG_SUCCESS");
    fprintf(file, "\n");
}

/*************************************************************
 *   Sets the correct value of the dealloc flag and calls
 *   Utils:WritePackMsgType to fill in the ipc msg type word(s)
 *   in the request message. Called by WriteRoutine for each
 *   argument that is to be sent in the request message.
 *************************************************************/
static void
WritePackArgType(file, arg)
    FILE *file;
    argument_t *arg;
{
    WritePackMsgType(file, arg->argType, arg->argDeallocate, arg->argLongForm,
		     TRUE, "InP->%s", "%s", arg->argTTName);
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to copy an argument into the request message.  
 *  Called by WriteRoutine for each argument that is to placed
 *  in the request message.
 *************************************************************/
static void
WritePackArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register char *ref = argByReferenceUser(arg) ? "*" : "";

    if (it->itInLine && it->itVarArray) {

	if (it->itString) {
	    /*
	     *	Copy variable-size C string with mig_strncpy.
	     *	Save the string length (+ 1 for trailing 0)
	     *	in the argument`s count field.
	     */
	    fprintf(file,
		"\tInP->%s = mig_strncpy(InP->%s, %s, %d);\n",
		arg->argCount->argMsgField,
		arg->argMsgField,
		arg->argVarName,
		it->itNumber);
	}
	else {

	    /*
	     *	Copy in variable-size inline array with bcopy,
	     *	after checking that number of elements doesn`t
	     *	exceed declared maximum.
	     */
	    register argument_t *count = arg->argCount;
	    register char *countRef = argByReferenceUser(count) ? "*" : "";
	    register ipc_type_t *btype = it->itElement;

	    /* Note btype->itNumber == count->argMultiplier */

	    fprintf(file, "\tif (%s%s > %d) {\n",
		countRef, count->argVarName,
		it->itNumber/btype->itNumber);
	    if (it->itIndefinite) {
		fprintf(file, "\t\tInP->%s.msgtl_header.msgt_inline = FALSE;\n",
			arg->argTTName);
		fprintf(file, "\t\t*((%s **)InP->%s) = %s%s;\n",
			FetchUserType(btype),
			arg->argMsgField,
			ref, arg->argVarName);
		if (!arg->argRoutine->rtSimpleFixedRequest)
		    fprintf(file, "\t\tmsgh_simple = FALSE;\n");
	    }
	    else
		WriteMsgError(file, arg->argRoutine, "MIG_ARRAY_TOO_LARGE");

	    fprintf(file, "\t}\n\telse {\n");

	    fprintf(file, "\t\tbcopy((char *) %s%s, (char *) InP->%s, ",
		ref, arg->argVarName, arg->argMsgField);
	    if (btype->itTypeSize > 1)
		fprintf(file, "%d * ", btype->itTypeSize);
	    fprintf(file, "%s%s);\n",
		countRef, count->argVarName);
	    fprintf(file, "\t}\n");
	}
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "InP->%s", "/* %s */ %d * %s%s",
		      arg->argMsgField, arg->argMultiplier,
		      ref, arg->argVarName);
    else
	WriteCopyType(file, it, "InP->%s", "/* %s */ %s%s",
		      arg->argMsgField, ref, arg->argVarName);
    fprintf(file, "\n");
}

static void
WriteAdjustMsgSimple(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (!arg->argRoutine->rtSimpleFixedRequest)
    {
	register char *ref = argByReferenceUser(arg) ? "*" : "";

	fprintf(file, "\tif (MACH_MSG_TYPE_PORT_ANY(%s%s))\n",
		ref, arg->argVarName);
	fprintf(file, "\t\tmsgh_simple = FALSE;\n");
	fprintf(file, "\n");
    }
}

/*
 * Calculate the size of a variable-length message field.
 */
static void
WriteArgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argType;
    register int bsize = ptype->itElement->itTypeSize;
    register argument_t *count = arg->argCount;

    if (ptype->itIndefinite) {
	/*
	 *	Check descriptor.  If out-of-line, use standard size.
	 */
	fprintf(file, "(InP->%s.msgtl_header.msgt_inline) ? ",
		arg->argTTName);
    }
    if (bsize > 1)
	fprintf(file, "%d * ", bsize);
    if (ptype->itString)
	/* get count from descriptor in message */
	fprintf(file, "InP->%s", count->argMsgField);
    else
	/* get count from argument */
	fprintf(file, "%s%s",
		argByReferenceUser(count) ? "*" : "",
		count->argVarName);

    /*
     * If the base type size is not a multiple of sizeof(int) [4],
     * we have to round up.
     */
    if (bsize % 4 != 0)
	fprintf(file, " + 3 & ~3");

    if (ptype->itIndefinite) {
	fprintf(file, " : sizeof(%s *)",
		FetchUserType(ptype->itElement));
    }
}

/*
 * Adjust message size and advance request pointer.
 * Called after packing a variable-length argument that
 * has more arguments following.
 */
static void
WriteAdjustMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argType;

    /* There are more In arguments.  We need to adjust msgh_size
       and advance InP, so we save the size of the current field
       in msgh_size_delta. */

    fprintf(file, "\tmsgh_size_delta = ");
    WriteArgSize(file, arg);
    fprintf(file, ";\n");

    if (arg->argRequestPos == 0)
	/* First variable-length argument.  The previous msgh_size value
	   is the minimum request size. */

	fprintf(file, "\tmsgh_size = %d + msgh_size_delta;\n",
		arg->argRoutine->rtRequestSize);
    else
	fprintf(file, "\tmsgh_size += msgh_size_delta;\n");

    fprintf(file,
	"\tInP = (Request *) ((char *) InP + msgh_size_delta - %d);\n",
	ptype->itTypeSize + ptype->itPadSize);
}

/*
 * Calculate the size of the message.  Called after the
 * last argument has been packed.
 */
static void
WriteFinishMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    /* No more In arguments.  If this is the only variable In
       argument, the previous msgh_size value is the minimum
       request size. */

    if (arg->argRequestPos == 0) {
	fprintf(file, "\tmsgh_size = %d + (",
			arg->argRoutine->rtRequestSize);
	WriteArgSize(file, arg);
	fprintf(file, ");\n");
    }
    else {
        fprintf(file, "\tmsgh_size += ");
	WriteArgSize(file, arg);
        fprintf(file, ";\n");
    }
}

/*
 * Called for every argument.  Responsible for packing that
 * argument into the request message.
 */
static void
WritePackArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (akCheck(arg->argKind, akbRequest))
	WritePackArgType(file, arg);

    if ((akIdent(arg->argKind) == akePoly) &&
	akCheck(arg->argKind, akbSendSnd))
	WriteAdjustMsgSimple(file, arg);

    if (akCheckAll(arg->argKind, akbSendSnd|akbSendBody))
	WritePackArgValue(file, arg);
}

/*
 * Generate code to fill in all of the request arguments and their
 * message types.
 */
WriteRequestArgs(file, rt)
    FILE *file;
    register routine_t *rt;
{
    register argument_t *arg;
    register argument_t *lastVarArg;

    lastVarArg = argNULL;
    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext) {

	/*
	 * Adjust message size and advance message pointer if
	 * the last request argument was variable-length and the
	 * request position will change.
	 */
	if (lastVarArg != argNULL &&
	    lastVarArg->argRequestPos < arg->argRequestPos)
	{
	    WriteAdjustMsgSize(file, lastVarArg);
	    lastVarArg = argNULL;
	}

	/*
	 * Copy the argument
	 */
	WritePackArg(file, arg);

	/*
	 * Remember whether this was variable-length.
	 */
	if (akCheckAll(arg->argKind, akbSendSnd|akbSendBody|akbVariable))
	    lastVarArg = arg;
    }

    /*
     * Finish the message size.
     */
    if (lastVarArg != argNULL)
	WriteFinishMsgSize(file, lastVarArg);
}

/*************************************************************
 *  Writes code to check that the return msgh_id is correct and that
 *  the size of the return message is correct. Called by
 *  WriteRoutine.
 *************************************************************/
static void
WriteCheckIdentity(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tif (OutP->Head.msgh_id != %d) {\n",
	    rt->rtNumber + SubsystemBase + 100);
    fprintf(file, "\t\tif (OutP->Head.msgh_id == MACH_NOTIFY_SEND_ONCE)\n");
    WriteMsgError(file, rt, "MIG_SERVER_DIED");
    fprintf(file, "\t\telse\n");
    WriteMsgError(file, rt, "MIG_REPLY_MISMATCH");
    fprintf(file, "\t}\n");
    fprintf(file, "\n");
    fprintf(file, "#if\tTypeCheck\n");

    if (rt->rtSimpleCheckReply && rt->rtSimpleReceiveReply)
    {
	/* Expecting a simple message.  We can factor out the check for
	   a simple message, since the error reply message is also simple.
	   */

	if (!rt->rtNoReplyArgs)
	    fprintf(file, "\tmsgh_size = OutP->Head.msgh_size;\n\n");

	fprintf(file,
	    "\tif ((OutP->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||\n");
	if (rt->rtNoReplyArgs)
	    fprintf(file, "\t    (OutP->Head.msgh_size != %d))\n",
			rt->rtReplySize);
	else {
	    fprintf(file, "\t    ((msgh_size %s %d) &&\n",
		(rt->rtNumReplyVar > 0) ? "<" : "!=",
		rt->rtReplySize);
	    fprintf(file, "\t     ((msgh_size != sizeof(mig_reply_header_t)) ||\n");
	    fprintf(file, "\t      (OutP->RetCode == KERN_SUCCESS))))\n");
	}
    }
    else {
	/* Expecting a complex message, or may vary at run time. */

	fprintf(file, "\tmsgh_size = OutP->Head.msgh_size;\n");
	fprintf(file, "\tmsgh_simple = !(OutP->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX);\n");
	fprintf(file, "\n");

	fprintf(file, "\tif (((msgh_size %s %d)",
		(rt->rtNumReplyVar > 0) ? "<" : "!=",
		rt->rtReplySize);

	if (rt->rtSimpleCheckReply)
	    /* if rtSimpleReceiveReply was true, then we would have
	       executed the code above.  So we know that the message
	       is complex. */
	    fprintf(file, " || msgh_simple");
	fprintf(file, ") &&\n");

	fprintf(file, "\t    ((msgh_size != sizeof(mig_reply_header_t)) ||\n");
	fprintf(file, "\t     !msgh_simple ||\n");
	fprintf(file, "\t     (OutP->RetCode == KERN_SUCCESS)))\n");
    }
    WriteMsgError(file, rt, "MIG_TYPE_ERROR");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

/*************************************************************
 *  Write code to generate error handling code if the RetCode
 *  argument of a Routine is not KERN_SUCCESS.
 *************************************************************/
static void
WriteRetCodeCheck(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tif (OutP->RetCode != KERN_SUCCESS)\n");
    WriteMsgError(file, rt, "OutP->RetCode");
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to check that the type of each of the arguments
 *  in the reply message is what is expected. Called by 
 *  WriteRoutine for each argument in the reply message.
 *************************************************************/
static void
WriteTypeCheck(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register routine_t *rt = arg->argRoutine;

    fprintf(file, "#if\tTypeCheck\n");
    if (akCheck(arg->argKind, akbReplyQC))
    {
	fprintf(file, "#if\tUseStaticMsgType\n");
	fprintf(file, "\tif (* (int *) &OutP->%s != * (int *) &%sCheck)\n",
		arg->argTTName, arg->argVarName);
	fprintf(file, "#else\tUseStaticMsgType\n");
    }
    fprintf(file, "\tif (");
    if (!it->itIndefinite) {
	fprintf(file, "(OutP->%s%s.msgt_inline != %s) ||\n\t    ",
	    arg->argTTName,
	    arg->argLongForm ? ".msgtl_header" : "",
	    strbool(it->itInLine));
    }
    fprintf(file, "(OutP->%s%s.msgt_longform != %s) ||\n",
	    arg->argTTName,
	    arg->argLongForm ? ".msgtl_header" : "",
	    strbool(arg->argLongForm));
    if (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)
    {
	if (!rt->rtSimpleCheckReply)
	    fprintf(file, "\t    (MACH_MSG_TYPE_PORT_ANY(OutP->%s.msgt%s_name) && msgh_simple) ||\n",
		    arg->argTTName,
		    arg->argLongForm ? "l" : "");
    }
    else
	fprintf(file, "\t    (OutP->%s.msgt%s_name != %s) ||\n",
		arg->argTTName,
		arg->argLongForm ? "l" : "",
		it->itOutNameStr);
    if (!it->itVarArray)
	fprintf(file, "\t    (OutP->%s.msgt%s_number != %d) ||\n",
		arg->argTTName,
		arg->argLongForm ? "l" : "",
		it->itNumber);
    fprintf(file, "\t    (OutP->%s.msgt%s_size != %d))\n",
	    arg->argTTName,
	    arg->argLongForm ? "l" : "",
	    it->itSize);
    if (akCheck(arg->argKind, akbReplyQC))
	fprintf(file, "#endif\tUseStaticMsgType\n");
    WriteMsgError(file, rt, "MIG_TYPE_ERROR");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

static void
WriteCheckArgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argType;
    register ipc_type_t *btype = ptype->itElement;
    argument_t *count = arg->argCount;
    int multiplier = btype->itTypeSize / btype->itNumber;

    if (ptype->itIndefinite) {
	/*
	 * Check descriptor.  If out-of-line, use standard size.
	 */
	fprintf(file, "(OutP->%s.msgtl_header.msgt_inline) ? ",
		arg->argTTName);
    }

    if (multiplier > 1)
	fprintf(file, "%d * ", multiplier);

    fprintf(file, "OutP->%s", count->argMsgField);

    /* If the base type size of the data field isn`t a multiple of 4,
       we have to round up. */
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, " + 3 & ~3");

    if (ptype->itIndefinite)
	fprintf(file, " : sizeof(%s *)", FetchUserType(btype));
}

static void
WriteCheckMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;

    /* If there aren't any more Out args after this, then
       we can use the msgh_size_delta value directly in
       the TypeCheck conditional. */

    if (arg->argReplyPos == rt->rtMaxReplyPos)
    {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tif (msgh_size != %d + (",
		rt->rtReplySize);
	WriteCheckArgSize(file, arg);
	fprintf(file, "))\n");

	WriteMsgError(file, rt, "MIG_TYPE_ERROR");
	fprintf(file, "#endif\tTypeCheck\n");
    }
    else
    {
	/* If there aren't any more variable-sized arguments after this,
	   then we must check for exact msg-size and we don't need
	   to update msgh_size. */

	boolean_t LastVarArg = arg->argReplyPos+1 == rt->rtNumReplyVar;

	/* calculate the actual size in bytes of the data field.  note
	   that this quantity must be a multiple of four.  hence, if
	   the base type size isn't a multiple of four, we have to
	   round up.  note also that btype->itNumber must
	   divide btype->itTypeSize (see itCalculateSizeInfo). */

	fprintf(file, "\tmsgh_size_delta = ");
	WriteCheckArgSize(file, arg);
	fprintf(file, ";\n");
	fprintf(file, "#if\tTypeCheck\n");

	/* Don't decrement msgh_size until we've checked that
	   it won't underflow. */

	if (LastVarArg)
	    fprintf(file, "\tif (msgh_size != %d + msgh_size_delta)\n",
		rt->rtReplySize);
	else
	    fprintf(file, "\tif (msgh_size < %d + msgh_size_delta)\n",
		rt->rtReplySize);
	WriteMsgError(file, rt, "MIG_TYPE_ERROR");

	if (!LastVarArg)
	    fprintf(file, "\tmsgh_size -= msgh_size_delta;\n");

	fprintf(file, "#endif\tTypeCheck\n");
    }
    fprintf(file, "\n");
}

/*************************************************************
 *  Write code to copy an argument from the reply message
 *  to the parameter. Called by WriteRoutine for each argument
 *  in the reply message.
 *************************************************************/
static void
WriteExtractArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t	*argType = arg->argType;
    register char *ref = argByReferenceUser(arg) ? "*" : "";

    if (argType->itInLine && argType->itVarArray) {

	if (argType->itString) {
	    /*
	     *	Copy out variable-size C string with mig_strncpy.
	     */
	    fprintf(file, "\t(void) mig_strncpy(%s%s, OutP->%s, %d);\n",
		ref,
		arg->argVarName,
		arg->argMsgField,
		argType->itNumber * argType->itTypeSize);
	}
	else {

	    /*
	     *	Copy out variable-size inline array with bcopy,
	     *	after checking that number of elements doesn`t
	     *	exceed user`s maximum.
	     */
	    register argument_t *count = arg->argCount;
	    register char *countRef = argByReferenceUser(count) ? "*" : "";
	    register ipc_type_t *btype = argType->itElement;

	    /* Note count->argMultiplier == btype->itNumber */

	    fprintf(file, "\tif (OutP->%s", count->argMsgField);
	    if (btype->itNumber > 1)
		fprintf(file, " / %d", btype->itNumber);
	    fprintf(file, " > %s%s) {\n",
		countRef, count->argVarName);

	    if (argType->itIndefinite) {
		/*
		 * If number of elements is too many for user receiving area,
		 * put a pointer to the data in the start of the user receiving
		 * area.  If the data is in-line, allocate memory and copy the
		 * data to it.
		 */
		fprintf(file, "\t\tif (OutP->%s.msgtl_header.msgt_inline) {\n",
			arg->argMsgField);
		fprintf(file, "\t\t\t(void) vm_allocate(mach_task_self(),\n");
		fprintf(file, "\t\t\t\t(vm_offset_t *)%s%s,\n\t\t\t\t",
			ref, arg->argVarName);
		if (btype->itTypeSize != btype->itNumber)
		    fprintf(file, "%d * ", btype->itTypeSize/btype->itNumber);
		fprintf(file, "OutP->%s, TRUE);\n",
			count->argMsgField);
		fprintf(file, "\t\t\tbcopy(*(char **)OutP->%s,*(char **)%s%s, ",
			arg->argMsgField, ref, arg->argVarName);
		if (btype->itTypeSize != btype->itNumber)
		    fprintf(file, "%d * ", btype->itTypeSize/btype->itNumber);
		fprintf(file, "OutP->%s);\n", count->argMsgField);
		fprintf(file, "\t\t} else {\n");
		fprintf(file, "\t\t\t*((%s **)%s%s) = *((%s **)OutP->%s);\n",
			FetchUserType(btype), ref, arg->argVarName,
			FetchUserType(btype), arg->argMsgField);
		fprintf(file, "\t\t}\n");
	    }
	    else {
		/*
		 * If number of elements is too many for user receiving area,
		 * fill user`s area as much as possible.  Return the correct
		 * number of elements.
		 */
		fprintf(file, "\t\tbcopy((char *) OutP->%s, (char *) %s%s, ",
			arg->argMsgField, ref, arg->argVarName);
		if (btype->itTypeSize > 1)
		    fprintf(file, "%d * ", btype->itTypeSize);
		fprintf(file, "%s%s);\n",
			countRef, count->argVarName);

		fprintf(file, "\t\t%s%s = OutP->%s",
		      countRef, count->argVarName, count->argMsgField);
		if (btype->itNumber > 1)
		    fprintf(file, " / %d", btype->itNumber);
		fprintf(file, ";\n");
		WriteMsgError(file,arg->argRoutine, "MIG_ARRAY_TOO_LARGE");
	    }
	    fprintf(file, "\t}\n\telse {\n");

	    fprintf(file, "\t\tbcopy((char *) OutP->%s, (char *) %s%s, ",
		arg->argMsgField, ref, arg->argVarName);
	    if (btype->itTypeSize != btype->itNumber)
		fprintf(file, "%d * ",
			btype->itTypeSize/btype->itNumber);
	    fprintf(file, "OutP->%s);\n",
		count->argMsgField);
	    fprintf(file, "\t}\n");
	}
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, argType,
		      "%s%s", "/* %s%s */ OutP->%s / %d",
		      ref, arg->argVarName, arg->argMsgField,
		      arg->argMultiplier);
    else
	WriteCopyType(file, argType,
		      "%s%s", "/* %s%s */ OutP->%s",
		      ref, arg->argVarName, arg->argMsgField);
    fprintf(file, "\n");
}

static void
WriteExtractArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;

    if (akCheck(arg->argKind, akbReply))
	WriteTypeCheck(file, arg);

    if (akCheckAll(arg->argKind, akbVariable|akbReply))
	WriteCheckMsgSize(file, arg);

    /* Now that the RetCode is type-checked, check its value.
       Must abort immediately if it isn't KERN_SUCCESS, because
       in that case the reply message is truncated. */

    if (arg == rt->rtRetCode)
	WriteRetCodeCheck(file, rt);

    if (akCheckAll(arg->argKind, akbReturnRcv))
	WriteExtractArgValue(file, arg);
}

WriteAdjustReplyMsgPtr(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argType;

    fprintf(file,
	"\tOutP = (Reply *) ((char *) OutP + msgh_size_delta - %d);\n\n",
	ptype->itTypeSize + ptype->itPadSize);
}

void
WriteReplyArgs(file, rt)
    FILE *file;
    register routine_t *rt;
{
    register argument_t *arg;
    register argument_t *lastVarArg;

    lastVarArg = argNULL;
    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext) {

	/*
	 * Advance message pointer if the last reply argument was
	 * variable-length and the reply position will change.
	 */
	if (lastVarArg != argNULL &&
	    lastVarArg->argReplyPos < arg->argReplyPos)
	{
	    WriteAdjustReplyMsgPtr(file, lastVarArg);
	    lastVarArg = argNULL;
	}

	/*
	 * Copy the argument
	 */
	WriteExtractArg(file, arg);

	/*
	 * Remember whether this was variable-length.
	 */
	if (akCheckAll(arg->argKind, akbReturnRcv|akbVariable))
	    lastVarArg = arg;
    }
}

/*************************************************************
 *  Writes code to return the return value. Called by WriteRoutine
 *  for routines and functions.
 *************************************************************/
static void
WriteReturnValue(file, rt)
    FILE *file;
    routine_t *rt;
{
    if (rt->rtReturn == rt->rtRetCode)
	/* If returning RetCode, we have already checked that it is
	   KERN_SUCCESS */
	fprintf(file, "\treturn KERN_SUCCESS;\n");

    else
    {
	if (rt->rtNumReplyVar > 0)
	    fprintf(file, "\tOutP = &Mess.Out;\n");

	fprintf(file, "\treturn OutP->%s;\n", rt->rtReturn->argMsgField);
    }
}

/*************************************************************
 *  Writes the elements of the message type declaration: the
 *  msg_type structure, the argument itself and any padding 
 *  that is required to make the argument a multiple of 4 bytes.
 *  Called by WriteRoutine for all the arguments in the request
 *  message first and then the reply message.
 *************************************************************/
static void
WriteFieldDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    WriteFieldDeclPrim(file, arg, FetchUserType);
}

static void
WriteStubDecl(file, rt)
    FILE *file;
    register routine_t *rt;
{
    fprintf(file, "\n");
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "mig_external %s %s\n", ReturnTypeStr(rt), rt->rtUserName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "(\n");
    WriteList(file, rt->rtArgs, WriteUserVarDecl, akbUserArg, ",\n", "\n");
    fprintf(file, ")\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(");
    WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", ", "");
    fprintf(file, ")\n");
    WriteList(file, rt->rtArgs, WriteUserVarDecl, akbUserArg, ";\n", ";\n");
    fprintf(file, "#endif\n");
    fprintf(file, "{\n");
}

/*************************************************************
 *  Writes all the code comprising a routine body. Called by
 *  WriteUser for each routine.
 *************************************************************/
static void
WriteRoutine(file, rt)
    FILE *file;
    register routine_t *rt;
{
    /* write the stub's declaration */

    WriteStubDecl(file, rt);

    /* typedef of structure for Request and Reply messages */

    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbRequest, "Request");
    if (!rt->rtOneWay)
	WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbReply, "Reply");

    /* declarations for local vars: Union of Request and Reply messages,
       InP, OutP and return value */

    WriteVarDecls(file, rt);

    /* declarations and initializations of the mach_msg_type_t variables
       for each argument */

    WriteList(file, rt->rtArgs, WriteTypeDeclIn, akbRequest, "\n", "\n");
    if (!rt->rtOneWay)
	WriteList(file, rt->rtArgs, WriteCheckDecl, akbReplyQC, "\n", "\n");

    /* fill in all the request message types and then arguments */

    WriteRequestArgs(file, rt);

    /* fill in request message head */

    WriteRequestHead(file, rt);
    fprintf(file, "\n");

    /* Write the send/receive or rpc call */

    if (rt->rtOneWay)
	WriteMsgSend(file, rt);
    else
    {
	if (UseMsgRPC)
	    WriteMsgRPC(file, rt);
	else
	    WriteMsgSendReceive(file, rt);

	/* Check the values that are returned in the reply message */

	WriteCheckIdentity(file, rt);

	/* If the reply message has no Out parameters or return values
	   other than the return code, we can type-check it and
	   return it directly. */

	if (rt->rtNoReplyArgs)
	{
	    WriteTypeCheck(file, rt->rtRetCode);

	    fprintf(file, "\treturn OutP->RetCode;\n");
	}
	else {
	    WriteReplyArgs(file, rt);

	    /* return the return value, if any */

	    if (rt->rtProcedure)
		fprintf(file, "\t/* Procedure - no return needed */\n");
	    else
		WriteReturnValue(file, rt);
	}
    }

    fprintf(file, "}\n");
}

/*************************************************************
 *  Writes out the xxxUser.c file. Called by mig.c
 *************************************************************/
void
WriteUser(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;

    WriteProlog(file);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    WriteRoutine(file, stat->stRoutine);
	    break;
	  case skImport:
	  case skUImport:
	  case skSImport:
	    break;
	  default:
	    fatal("WriteUser(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    WriteEpilog(file);
}

/*************************************************************
 *  Writes out individual .c user files for each routine.  Called by mig.c
 *************************************************************/
void
WriteUserIndividual(stats)
    statement_t *stats;
{
    register statement_t *stat;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    {
		FILE *file;
		register char *filename;

		filename = strconcat(UserFilePrefix,
				     strconcat(stat->stRoutine->rtName, ".c"));
		file = fopen(filename, "w");
		if (file == NULL)
		    fatal("fopen(%s): %s", filename,
			  unix_error_string(errno));
		WriteProlog(file);
		WriteRoutine(file, stat->stRoutine);
		WriteEpilog(file);
		fclose(file);
		strfree(filename);
	    }
	    break;
	  case skImport:
	  case skUImport:
	  case skSImport:
	    break;
	  default:
	    fatal("WriteUserIndividual(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
}
