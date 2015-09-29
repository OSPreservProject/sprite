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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	server.c,v $
 * Revision 2.6  91/08/28  11:17:21  jsb
 * 	Replaced ServerProcName with ServerDemux.
 * 	[91/08/13            rpd]
 * 
 * 	Removed Camelot and TrapRoutine support.
 * 	Changed MsgKind to MsgSeqno.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.5  91/07/31  18:10:51  dbg
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
 * Revision 2.4  91/06/25  10:31:51  rpd
 * 	Cast request and reply ports to ipc_port_t in KernelServer stubs.
 * 	[91/05/27            rpd]
 * 
 * Revision 2.3  91/02/05  17:55:37  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:55:30  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:29  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:13:12  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 18-Oct-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Set the local port in the server reply message to
 *	MACH_PORT_NULL for greater efficiency and to make Camelot
 *	happy.
 *
 * 18-Apr-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed call to WriteLocalVarDecl in WriteMsgVarDecl
 *	to write out the parameters for the C++ code to a call
 *	a new routine WriteServerVarDecl which includes the *
 *	for reference variable, but uses the transType if it
 *	exists.
 *
 * 27-Feb-88  Richard Draves (rpd) at Carnegie-Mellon University
 *	Changed reply message initialization for camelot interfaces.
 *	Now we assume camelot interfaces are all camelotroutines and
 *	always initialize the dummy field & tid field.  This fixes
 *	the wrapper-server-call bug in distributed transactions.
 *
 * 23-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed the include of camelot_types.h to cam/camelot_types.h
 *
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed WriteDestroyArg to not call the destructor
 *	function on any in/out args.
 *
 *  4-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed dld's code to write out parameter list to
 *	use WriteLocalVarDecl to get transType or ServType if
 *	they exist.
 *
 * 19-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Change variable-length inline array declarations to use
 *	maximum size specified to Mig.  Make message variable
 *	length if the last item in the message is variable-length
 *	and inline.  Use argMultipler field to convert between
 *	argument and IPC element counts.
 *
 * 18-Jan-88  David Detlefs (dld) at Carnegie-Mellon University
 *	Modified to produce C++ compatible code via #ifdefs.
 *	All changes have to do with argument declarations.
 *
 *  2-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Added destructor function for IN arguments to server.
 *
 * 18-Nov-87  Jeffrey Eppinger (jle) at Carnegie-Mellon University
 *	Changed to typedef "novalue" as "void" if we're using hc.
 *
 * 17-Sep-87  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Added _<system>SymTab{Base|End} for use with security
 *	dispatch routine.  It is neccessary for the authorization
 *	system to know the operations by symbolic names.
 *	It is harmless to user code as it only means an extra
 *	array if it is accidentally turned on.
 *
 * 24-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Corrected the setting of retcode for CamelotRoutines.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added deallocflag to call to WritePackArgType.
 *
 * 14-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Moved type declarations and assignments for DummyType 
 *	and tidType to server demux routine. Automatically
 *	include camelot_types.h and msg_types.h for interfaces 
 *	containing camelotRoutines.
 *
 *  8-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed #include of sys/types.h and strings.h from WriteIncludes.
 *	Changed the KERNEL include from ../h to sys/
 *	Removed extern from WriteServer to make hi-c happy
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <assert.h>

#include <mach/message.h>
#include "write.h"
#include "utils.h"
#include "global.h"

static void
WriteIncludes(file)
    FILE *file;
{
    fprintf(file, "#define EXPORT_BOOLEAN\n");
    fprintf(file, "#include <mach/boolean.h>\n");
    fprintf(file, "#include <mach/kern_return.h>\n");
    fprintf(file, "#include <mach/message.h>\n");
    fprintf(file, "#include <mach/mig_errors.h>\n");
    if (IsKernelServer)
	fprintf(file, "#include <ipc/ipc_port.h>\n");
    fprintf(file, "\n");
}

static void
WriteGlobalDecls(file)
    FILE *file;
{
    fprintf(file, "/* Due to pcc compiler bug, cannot use void */\n");
    fprintf(file, "#if\t%s || defined(hc)\n", NewCDecl);
    fprintf(file, "#define novalue void\n");
    fprintf(file, "#else\n");
    fprintf(file, "#define novalue int\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    if (RCSId != strNULL)
	WriteRCSDecl(file, strconcat(SubsystemName, "_server"), RCSId);

    /* Used for locations in the request message, *not* reply message.
       Reply message locations aren't dependent on IsKernelServer. */

    if (IsKernelServer)
    {
	fprintf(file, "#define msgh_request_port\tmsgh_remote_port\n");
	fprintf(file, "#define MACH_MSGH_BITS_REQUEST(bits)");
	fprintf(file, "\tMACH_MSGH_BITS_REMOTE(bits)\n");
	fprintf(file, "#define msgh_reply_port\t\tmsgh_local_port\n");
	fprintf(file, "#define MACH_MSGH_BITS_REPLY(bits)");
	fprintf(file, "\tMACH_MSGH_BITS_LOCAL(bits)\n");
    }
    else
    {
	fprintf(file, "#define msgh_request_port\tmsgh_local_port\n");
	fprintf(file, "#define MACH_MSGH_BITS_REQUEST(bits)");
	fprintf(file, "\tMACH_MSGH_BITS_LOCAL(bits)\n");
	fprintf(file, "#define msgh_reply_port\t\tmsgh_remote_port\n");
	fprintf(file, "#define MACH_MSGH_BITS_REPLY(bits)");
	fprintf(file, "\tMACH_MSGH_BITS_REMOTE(bits)\n");
    }
    fprintf(file, "\n");
}

static void
WriteProlog(file)
    FILE *file;
{
    fprintf(file, "/* Module %s */\n", SubsystemName);
    fprintf(file, "\n");
    
    WriteIncludes(file);
    WriteBogusDefines(file);
    WriteGlobalDecls(file);
}


static void
WriteSymTabEntries(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;
    register u_int current = 0;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine) {
	    register	num = stat->stRoutine->rtNumber;
	    char	*name = stat->stRoutine->rtName;
	    while (++current <= num)
		fprintf(file,"\t\t\t{ \"\", 0, 0 },\n");
	    fprintf(file, "\t{ \"%s\", %d, _X%s },\n",
	    	name,
		SubsystemBase + current - 1,
		name);
	}
    while (++current <= rtNumber)
	fprintf(file,"\t{ \"\", 0, 0 },\n");
}

static void
WriteArrayEntries(file, stats)
    FILE *file;
    statement_t *stats;
{
    register u_int current = 0;
    register statement_t *stat;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine)
	{
	    register routine_t *rt = stat->stRoutine;

	    while (current++ < rt->rtNumber)
		fprintf(file, "\t\t0,\n");
	    fprintf(file, "\t\t_X%s,\n", rt->rtName);
	}
    while (current++ < rtNumber)
	fprintf(file, "\t\t\t0,\n");
}

static void
WriteEpilog(file, stats)
    FILE *file;
    statement_t *stats;
{
    fprintf(file, "\n");

    fprintf(file, "mig_external boolean_t %s\n", ServerDemux);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(InHeadP, OutHeadP)\n");
    fprintf(file, "\tmach_msg_header_t *InHeadP, *OutHeadP;\n");
    fprintf(file, "#endif\n");

    fprintf(file, "{\n");
    fprintf(file, "\tregister mach_msg_header_t *InP =  InHeadP;\n");

    fprintf(file, "\tregister mig_reply_header_t *OutP = (mig_reply_header_t *) OutHeadP;\n");

    fprintf(file, "\n");

    WriteStaticDecl(file, itRetCodeType,
		    itRetCodeType->itDeallocate, itRetCodeType->itLongForm,
		    !IsKernelServer, "RetCodeType");
    fprintf(file, "\n");

    fprintf(file, "\ttypedef novalue (*SERVER_STUB_PROC)\n");
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t\t(mach_msg_header_t *, mach_msg_header_t *);\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t\t();\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\tstatic SERVER_STUB_PROC routines[] = {\n");

    WriteArrayEntries(file, stats);

    fprintf(file, "\t};\n");
    fprintf(file, "\n");
    fprintf(file, "\tregister SERVER_STUB_PROC routine;\n");
    fprintf(file, "\n");

    fprintf(file, "\tOutP->Head.msgh_bits = ");
    fprintf(file, "MACH_MSGH_BITS(MACH_MSGH_BITS_REPLY(InP->msgh_bits), 0);\n");
    fprintf(file, "\tOutP->Head.msgh_size = sizeof *OutP;\n");
    fprintf(file, "\tOutP->Head.msgh_remote_port = InP->msgh_reply_port;\n");
    fprintf(file, "\tOutP->Head.msgh_local_port = MACH_PORT_NULL;\n");
    fprintf(file, "\tOutP->Head.msgh_seqno = 0;\n");
    fprintf(file, "\tOutP->Head.msgh_id = InP->msgh_id + 100;\n");
    fprintf(file, "\n");
    WritePackMsgType(file, itRetCodeType,
		     itRetCodeType->itDeallocate, itRetCodeType->itLongForm,
		     !IsKernelServer, "OutP->RetCodeType", "RetCodeType");
    fprintf(file, "\n");

    fprintf(file, "\tif ((InP->msgh_id > %d) || (InP->msgh_id < %d) ||\n",
	    SubsystemBase + rtNumber - 1, SubsystemBase);
    fprintf(file, "\t    ((routine = routines[InP->msgh_id - %d]) == 0)) {\n",
	    SubsystemBase);
    fprintf(file, "\t\tOutP->RetCode = MIG_BAD_ID;\n");
    fprintf(file, "\t\treturn FALSE;\n");
    fprintf(file, "\t}\n");

    /* Call appropriate routine */
    fprintf(file, "\t(*routine) (InP, &OutP->Head);\n");
    fprintf(file, "\treturn TRUE;\n");
    fprintf(file, "}\n");

    /* symtab */

    if (GenSymTab) {
	fprintf(file,"\nmig_symtab_t _%sSymTab[] = {\n",SubsystemName);
	WriteSymTabEntries(file,stats);
	fprintf(file,"};\n");
	fprintf(file,"int _%sSymTabBase = %d;\n",SubsystemName,SubsystemBase);
	fprintf(file,"int _%sSymTabEnd = %d;\n",SubsystemName,SubsystemBase+rtNumber);
    }
}

/*
 *  Returns the return type of the server-side work function.
 *  Suitable for "extern %s serverfunc()".
 */
static char *
ServerSideType(rt)
    routine_t *rt;
{
    if (rt->rtServerReturn == argNULL)
	return "void";
    else
	return rt->rtServerReturn->argType->itTransType;
}

static void
WriteLocalVarDecl(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (it->itInLine && it->itVarArray)
    {
	register ipc_type_t *btype = it->itElement;

	fprintf(file, "\t%s %s[%d]", btype->itTransType,
		arg->argVarName, it->itNumber/btype->itNumber);
    }
    else
	fprintf(file, "\t%s %s", it->itTransType, arg->argVarName);
}

static void
WriteServerArgDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    fprintf(file, "%s %s%s",
	    arg->argType->itTransType,
	    argByReferenceServer(arg) ? "*" : "",
	    arg->argVarName);
}

/*
 *  Writes the local variable declarations which are always
 *  present:  InP, OutP, the server-side work function.
 */
static void
WriteVarDecls(file, rt)
    FILE *file;
    routine_t *rt;
{
    int i;
    boolean_t NeedMsghSize = FALSE;
    boolean_t NeedMsghSizeDelta = FALSE;

    fprintf(file, "\tregister Request *In0P = (Request *) InHeadP;\n");
    for (i = 1; i <= rt->rtMaxRequestPos; i++)
	fprintf(file, "\tregister Request *In%dP;\n", i);
    fprintf(file, "\tregister Reply *OutP = (Reply *) OutHeadP;\n");

    fprintf(file, "\tmig_external %s %s\n",
	    ServerSideType(rt), rt->rtServerName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t\t(");
    WriteList(file, rt->rtArgs, WriteServerArgDecl, akbServerArg, ", ", "");
    fprintf(file, ");\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t\t();\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    if (!rt->rtSimpleFixedReply)
	fprintf(file, "\tboolean_t msgh_simple;\n");
    else if (!rt->rtSimpleCheckRequest)
    {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tboolean_t msgh_simple;\n");
	fprintf(file, "#endif\tTypeCheck\n");
	fprintf(file, "\n");
    }

    /* if either request or reply is variable, we may need
       msgh_size_delta and msgh_size */

    if (rt->rtNumRequestVar > 0)
	NeedMsghSize = TRUE;
    if (rt->rtMaxRequestPos > 0)
	NeedMsghSizeDelta = TRUE;

    if (rt->rtNumReplyVar > 1)
	NeedMsghSize = TRUE;
    if (rt->rtMaxReplyPos > 0)
	NeedMsghSizeDelta = TRUE;

    if (NeedMsghSize)
	fprintf(file, "\tunsigned int msgh_size;\n");
    if (NeedMsghSizeDelta)
	fprintf(file, "\tunsigned int msgh_size_delta;\n");

    if (NeedMsghSize || NeedMsghSizeDelta)
	fprintf(file, "\n");
}

static void
WriteMsgError(file, error)
    FILE *file;
    char *error;
{
    fprintf(file, "\t\t{ OutP->RetCode = %s; return; }\n", error);
}

static void
WriteReplyInit(file, rt)
    FILE *file;
    routine_t *rt;
{
    boolean_t printed_nl = FALSE;

    if (rt->rtSimpleFixedReply)
    {
	if (!rt->rtSimpleSendReply) /* complex reply message */
	{
	    printed_nl = TRUE;
	    fprintf(file, "\n");
	    fprintf(file,
		"\tOutP->Head.msgh_bits |= MACH_MSGH_BITS_COMPLEX;\n");
	}
    }
    else
    {
	printed_nl = TRUE;
	fprintf(file, "\n");
	fprintf(file, "\tmsgh_simple = %s;\n", 
			  strbool(rt->rtSimpleSendReply));
    }

    if (rt->rtNumReplyVar == 0)
    {
	if (!printed_nl)
	    fprintf(file, "\n");
	fprintf(file, "\tOutP->Head.msgh_size = %d;\n", rt->rtReplySize);
    }
}

static void
WriteReplyHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    if ((!rt->rtSimpleFixedReply) ||
	(rt->rtNumReplyVar > 1))
    {
	fprintf(file, "\n");
	if (rt->rtMaxReplyPos > 0)
	    fprintf(file, "\tOutP = (Reply *) OutHeadP;\n");
    }

    if (!rt->rtSimpleFixedReply)
    {
	fprintf(file, "\tif (!msgh_simple)\n");
	fprintf(file,
		"\t\tOutP->Head.msgh_bits |= MACH_MSGH_BITS_COMPLEX;\n");
    }
    if (rt->rtNumReplyVar > 1)
	fprintf(file, "\tOutP->Head.msgh_size = msgh_size;\n");
}

static void
WriteCheckHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "#if\tTypeCheck\n");
    if (rt->rtNumRequestVar > 0)
	fprintf(file, "\tmsgh_size = In0P->Head.msgh_size;\n");
    if (!rt->rtSimpleCheckRequest)
	fprintf(file, "\tmsgh_simple = !(In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX);\n");

    if (rt->rtNumRequestVar > 0)
	fprintf(file, "\tif ((msgh_size < %d)",
		rt->rtRequestSize);
    else
	fprintf(file, "\tif ((In0P->Head.msgh_size != %d)",
		rt->rtRequestSize);

    if (rt->rtSimpleCheckRequest)
	fprintf(file, " ||\n\t    %s(In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)",
		rt->rtSimpleReceiveRequest ? "" : "!");
    fprintf(file, ")\n");
    WriteMsgError(file, "MIG_BAD_ARGUMENTS");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

static void
WriteTypeCheck(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register routine_t *rt = arg->argRoutine;

    fprintf(file, "#if\tTypeCheck\n");
    if (akCheck(arg->argKind, akbRequestQC))
    {
	fprintf(file, "#if\tUseStaticMsgType\n");
	fprintf(file, "\tif (* (int *) &In%dP->%s != * (int *) &%sCheck)\n",
		arg->argRequestPos, arg->argTTName, arg->argVarName);
	fprintf(file, "#else\tUseStaticMsgType\n");
    }
    fprintf(file, "\tif (");
    if (!it->itIndefinite) {
	fprintf(file, "(In%dP->%s%s.msgt_inline != %s) ||\n\t    ",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? ".msgtl_header" : "",
	    strbool(it->itInLine));
    }
    fprintf(file, "(In%dP->%s%s.msgt_longform != %s) ||\n",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? ".msgtl_header" : "",
	    strbool(arg->argLongForm));
    if (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)
    {
	if (!rt->rtSimpleCheckRequest)
	    fprintf(file, "\t    (MACH_MSG_TYPE_PORT_ANY(In%dP->%s.msgt%s_name) && msgh_simple) ||\n",
		    arg->argRequestPos, arg->argTTName,
		    arg->argLongForm ? "l" : "");
    }
    else
	fprintf(file, "\t    (In%dP->%s.msgt%s_name != %s) ||\n",
		arg->argRequestPos, arg->argTTName,
		arg->argLongForm ? "l" : "",
		it->itOutNameStr);
    if (!it->itVarArray)
	fprintf(file, "\t    (In%dP->%s.msgt%s_number != %d) ||\n",
		arg->argRequestPos, arg->argTTName,
		arg->argLongForm ? "l" : "",
		it->itNumber);
    fprintf(file, "\t    (In%dP->%s.msgt%s_size != %d))\n",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? "l" : "",
	    it->itSize);
    if (akCheck(arg->argKind, akbRequestQC))
	fprintf(file, "#endif\tUseStaticMsgType\n");
    WriteMsgError(file, "MIG_BAD_ARGUMENTS");
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
	fprintf(file, "(In%dP->%s.msgtl_header.msgt_inline) ? ",
		arg->argRequestPos,
		arg->argTTName);
    }

    if (multiplier > 1)
	fprintf(file, "%d * ", multiplier);

    fprintf(file, "In%dP->%s", arg->argRequestPos, count->argMsgField);

    /* If the base type size of the data field isn`t a multiple of 4,
       we have to round up. */
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, " + 3 & ~3");

    if (ptype->itIndefinite) {
	fprintf(file, " : sizeof(%s *)", FetchServerType(btype));
    }
}

static void
WriteCheckMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;

    /* If there aren't any more In args after this, then
       we can use the msgh_size_delta value directly in
       the TypeCheck conditional. */

    if (arg->argRequestPos == rt->rtMaxRequestPos)
    {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tif (msgh_size != %d + (",
		rt->rtRequestSize);
	WriteCheckArgSize(file, arg);
	fprintf(file, "))\n");

	WriteMsgError(file, "MIG_BAD_ARGUMENTS");
	fprintf(file, "#endif\tTypeCheck\n");
    }
    else
    {
	/* If there aren't any more variable-sized arguments after this,
	   then we must check for exact msg-size and we don't need to
	   update msgh_size. */

	boolean_t LastVarArg = arg->argRequestPos+1 == rt->rtNumRequestVar;

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
		rt->rtRequestSize);
	else
	    fprintf(file, "\tif (msgh_size < %d + msgh_size_delta)\n",
		rt->rtRequestSize);
	WriteMsgError(file, "MIG_BAD_ARGUMENTS");

	if (!LastVarArg)
	    fprintf(file, "\tmsgh_size -= msgh_size_delta;\n");

	fprintf(file, "#endif\tTypeCheck\n");
    }
    fprintf(file, "\n");
}

static char *
InArgMsgField(arg)
    register argument_t *arg;
{
    static char buffer[100];

    /*
     *	Inside the kernel, the request and reply port fields
     *	really hold ipc_port_t values, not mach_port_t values.
     *	Hence we must cast the values.
     */

    if (IsKernelServer &&
	((akIdent(arg->argKind) == akeRequestPort) ||
	 (akIdent(arg->argKind) == akeReplyPort)))
	sprintf(buffer, "(ipc_port_t) In%dP->%s",
		arg->argRequestPos, arg->argMsgField);
    else
	sprintf(buffer, "In%dP->%s",
		arg->argRequestPos, arg->argMsgField);

    return buffer;
}

static void
WriteExtractArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "%s", "/* %s */ %s / %d",
		      arg->argVarName, InArgMsgField(arg), arg->argMultiplier);
    else if (it->itInTrans != strNULL)
	WriteCopyType(file, it, "%s", "/* %s */ %s(%s)",
		      arg->argVarName, it->itInTrans, InArgMsgField(arg));
    else
	WriteCopyType(file, it, "%s", "/* %s */ %s",
		      arg->argVarName, InArgMsgField(arg));
    fprintf(file, "\n");
}

static void
WriteInitializeCount(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argParent->argType;
    register ipc_type_t *btype = ptype->itElement;

    /*
     *	Initialize 'count' argument for variable-length inline OUT parameter
     *	with maximum allowed number of elements.
     */

    fprintf(file, "\t%s = %d;\n", arg->argVarName,
	    ptype->itNumber/btype->itNumber);
    fprintf(file, "\n");
}

static void
WriteTypeCheckArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (akCheck(arg->argKind, akbRequest)) {
	WriteTypeCheck(file, arg);

	if (akCheck(arg->argKind, akbVariable))
	    WriteCheckMsgSize(file, arg);
    }
}

WriteAdjustRequestMsgPtr(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argType;

    fprintf(file,
	"\tIn%dP = (Request *) ((char *) In%dP + msgh_size_delta - %d);\n\n",
	arg->argRequestPos+1, arg->argRequestPos,
	ptype->itTypeSize + ptype->itPadSize);
}

WriteTypeCheckRequestArgs(file, rt)
    FILE *file;
    register routine_t *rt;
{
    register argument_t *arg;
    register argument_t *lastVarArg;

    lastVarArg = argNULL;
    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext) {

	/*
	 * Advance message pointer if the last request argument was
	 * variable-length and the request position will change.
	 */
	if (lastVarArg != argNULL &&
	    lastVarArg->argRequestPos < arg->argRequestPos)
	{
	    WriteAdjustRequestMsgPtr(file, lastVarArg);
	    lastVarArg = argNULL;
	}

	/*
	 * Type-check the argument.
	 */
	WriteTypeCheckArg(file, arg);

	/*
	 * Remember whether this was variable-length.
	 */
	if (akCheckAll(arg->argKind, akbVariable|akbRequest))
	    lastVarArg = arg;
    }
}

static void
WriteExtractArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (akCheckAll(arg->argKind, akbSendRcv|akbVarNeeded))
	WriteExtractArgValue(file, arg);

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbReturnSnd))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	    WriteInitializeCount(file, arg);
    }
}

static void
WriteServerCallArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    ipc_type_t *it = arg->argType;
    boolean_t NeedClose = FALSE;

    if (argByReferenceServer(arg))
	fprintf(file, "&");

    if ((it->itInTrans != strNULL) &&
	akCheck(arg->argKind, akbSendRcv) &&
	!akCheck(arg->argKind, akbVarNeeded))
    {
	fprintf(file, "%s(", it->itInTrans);
	NeedClose = TRUE;
    }

    if (akCheck(arg->argKind, akbVarNeeded))
	fprintf(file, "%s", arg->argVarName);
    else if (akCheck(arg->argKind, akbSendRcv)) {
	if (akCheck(arg->argKind, akbIndefinite)) {
	    fprintf(file, "(In%dP->%s.msgtl_header.msgt_inline) ",
			arg->argRequestPos,
			arg->argTTName);
	    fprintf(file, "? %s ", InArgMsgField(arg));
	    fprintf(file, ": *((%s **)%s)",
			FetchServerType(arg->argType->itElement),
			InArgMsgField(arg));
	}
	else
	    fprintf(file, "%s", InArgMsgField(arg));
    }
    else
	fprintf(file, "OutP->%s", arg->argMsgField);

    if (NeedClose)
	fprintf(file, ")");

    if (!argByReferenceServer(arg) && (arg->argMultiplier > 1))
	fprintf(file, " / %d", arg->argMultiplier);
}

static void
WriteDestroyArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (akCheck(arg->argKind, akbIndefinite)) {
	/*
	 * Deallocate only if out-of-line.
	 */
	argument_t *count = arg->argCount;
	ipc_type_t *btype = it->itElement;
	int	multiplier = btype->itTypeSize / btype->itNumber;

	fprintf(file, "\tif (!In%dP->%s.msgtl_header.msgt_inline)\n",
		arg->argRequestPos, arg->argTTName);
	fprintf(file, "\t\tmig_deallocate(*(char **)%s, ",
		InArgMsgField(arg));
	if (multiplier > 1)
	    fprintf(file, "%d * ", multiplier);
	fprintf(file, " %s);\n",
		InArgMsgField(count));
    }
    else {
	if (akCheck(arg->argKind, akbVarNeeded))
	    fprintf(file, "\t%s(%s);\n", it->itDestructor, arg->argVarName);
	else
	    fprintf(file, "\t%s(%s);\n", it->itDestructor,
		InArgMsgField(arg));
    }
}

static void
WriteDestroyPortArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    /*
     *	If a translated port argument occurs in the body of a request
     *	message, and the message is successfully processed, then the
     *	port right should be deallocated.  However, the called function
     *	didn't see the port right; it saw the translation.  So we have
     *	to release the port right for it.
     */

    if ((it->itInTrans != strNULL) &&
	(it->itOutName == MACH_MSG_TYPE_PORT_SEND))
    {
	fprintf(file, "\n");
	fprintf(file, "\tif (IP_VALID(%s))\n", InArgMsgField(arg));
	fprintf(file, "\t\tipc_port_release_send(%s);\n", InArgMsgField(arg));
    }
}

/*
 * Check whether WriteDestroyPortArg would generate any code for arg.
 */
boolean_t
CheckDestroyPortArg(arg)
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if ((it->itInTrans != strNULL) &&
	(it->itOutName == MACH_MSG_TYPE_PORT_SEND))
    {
	return TRUE;
    }
    return FALSE;
}

static void
WriteServerCall(file, rt)
    FILE *file;
    routine_t *rt;
{
    boolean_t NeedClose = FALSE;

    fprintf(file, "\t");
    if (rt->rtServerReturn != argNULL)
    {
	argument_t *arg = rt->rtServerReturn;
	ipc_type_t *it = arg->argType;

	fprintf(file, "OutP->%s = ", arg->argMsgField);
	if (it->itOutTrans != strNULL)
	{
	    fprintf(file, "%s(", it->itOutTrans);
	    NeedClose = TRUE;
	}
    }
    fprintf(file, "%s(", rt->rtServerName);
    WriteList(file, rt->rtArgs, WriteServerCallArg, akbServerArg, ", ", "");
    if (NeedClose)
	fprintf(file, ")");
    fprintf(file, ");\n");
}

static void
WriteGetReturnValue(file, rt)
    FILE *file;
    register routine_t *rt;
{
    if (rt->rtServerReturn != rt->rtRetCode)
	fprintf(file, "\tOutP->%s = KERN_SUCCESS;\n",
		rt->rtRetCode->argMsgField);
}

static void
WriteCheckReturnValue(file, rt)
    FILE *file;
    register routine_t *rt;
{
    if (rt->rtServerReturn == rt->rtRetCode)
    {
	fprintf(file, "\tif (OutP->%s != KERN_SUCCESS)\n",
		rt->rtRetCode->argMsgField);
	fprintf(file, "\t\treturn;\n");
    }
}

static void
WritePackArgType(file, arg)
    FILE *file;
    register argument_t *arg;
{
    fprintf(file, "\n");

    WritePackMsgType(file, arg->argType, arg->argDeallocate, arg->argLongForm,
		     !IsKernelServer, "OutP->%s", "%s", arg->argTTName);
}

static void
WritePackArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    fprintf(file, "\n");

    if (it->itInLine && it->itVarArray) {

	if (it->itString) {
	    /*
	     *	Copy variable-size C string with mig_strncpy.
	     *	Save the string length (+ 1 for trailing 0)
	     *	in the argument`s count field.
	     */
	    fprintf(file,
		"\tOutP->%s = mig_strncpy(OutP->%s, %s, %d);\n",
		arg->argCount->argMsgField,
		arg->argMsgField,
		arg->argVarName,
		it->itNumber);
	}
	else {
	    register argument_t *count = arg->argCount;
	    register ipc_type_t *btype = it->itElement;

	    /* Note btype->itNumber == count->argMultiplier */

	    if (it->itIndefinite) {
		fprintf(file, "\tif (%s > %d) {\n",
			count->argVarName,
			it->itNumber/btype->itNumber);
		fprintf(file, "\t\tOutP->%s.msgtl_header.msgt_inline = FALSE;\n",
			arg->argTTName);
		fprintf(file, "\t\t*((%s *)OutP->%s) = %s;\n",
			FetchServerType(btype),
			arg->argMsgField,
			arg->argVarName);
		if (!arg->argRoutine->rtSimpleFixedReply)
		    fprintf(file, "\t\tmsgh_simple = FALSE;\n");
		fprintf(file, "\t}\n\telse {\n\t");
	    }
	    fprintf(file, "\tbcopy((char *) %s, (char *) OutP->%s, ",
		arg->argVarName, arg->argMsgField);
	    if (btype->itTypeSize > 1)
		fprintf(file, "%d * ",
			btype->itTypeSize);
	    fprintf(file, "%s);\n",
		count->argVarName);
	    if (it->itIndefinite)
		fprintf(file, "\t}\n");
	}
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "OutP->%s", "/* %s */ %d * %s",
		      arg->argMsgField,
		      arg->argMultiplier,
		      arg->argVarName);
    else if (it->itOutTrans != strNULL)
	WriteCopyType(file, it, "OutP->%s", "/* %s */ %s(%s)",
		      arg->argMsgField, it->itOutTrans, arg->argVarName);
    else
	WriteCopyType(file, it, "OutP->%s", "/* %s */ %s",
		      arg->argMsgField, arg->argVarName);
}

static void
WriteCopyArgValue(file, arg)
    FILE *file;
    argument_t *arg;
{
    fprintf(file, "\n");
    WriteCopyType(file, arg->argType, "/* %d */ OutP->%s", "In%dP->%s",
		  arg->argRequestPos, arg->argMsgField);
}

static void
WriteAdjustMsgSimple(file, arg)
    FILE *file;
    register argument_t *arg;
{
    /* akbVarNeeded must be on */

    if (!arg->argRoutine->rtSimpleFixedReply)
    {
	fprintf(file, "\n");
	fprintf(file, "\tif (MACH_MSG_TYPE_PORT_ANY(%s))\n", arg->argVarName);
	fprintf(file, "\t\tmsgh_simple = FALSE;\n");
    }
}

static void
WriteAdjustMsgCircular(file, arg)
    FILE *file;
    register argument_t *arg;
{
    fprintf(file, "\n");

    if (arg->argType->itOutName == MACH_MSG_TYPE_POLYMORPHIC)
	fprintf(file, "\tif (%s == MACH_MSG_TYPE_PORT_RECEIVE)\n",
		arg->argPoly->argVarName);

    /*
     *	The carried port right can be accessed in OutP->XXXX.  Normally
     *	the server function stuffs it directly there.  If it is InOut,
     *	then it has already been copied into the reply message.
     *	If the server function deposited it into a variable (perhaps
     *	because the reply message is variable-sized) then it has already
     *	been copied into the reply message.  Note we must use InHeadP
     *	(or In0P->Head) and OutHeadP to access the message headers,
     *	because of the variable-sized messages.
     */

    fprintf(file, "\tif (IP_VALID((ipc_port_t) InHeadP->msgh_reply_port) &&\n");
    fprintf(file, "\t    IP_VALID((ipc_port_t) OutP->%s) &&\n", arg->argMsgField);
    fprintf(file, "\t    ipc_port_check_circularity((ipc_port_t) OutP->%s, (ipc_port_t) InHeadP->msgh_reply_port))\n", arg->argMsgField);
    fprintf(file, "\t\tOutHeadP->msgh_bits |= MACH_MSGH_BITS_CIRCULAR;\n");
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
	 * Check descriptor.  If out-of-line, use standard size.
	 */
	fprintf(file, "(OutP->%s.msgtl_header.msgt_inline) ? ",
		arg->argTTName);
    }

    if (bsize > 1)
	fprintf(file, "%d * ", bsize);
    if (ptype->itString)
	/* get count from descriptor in message */
	fprintf(file, "OutP->%s", count->argMsgField);
    else
	/* get count from argument */
	fprintf(file, "%s", count->argVarName);

    /*
     * If the base type size is not a multiple of sizeof(int) [4],
     * we have to round up.
     */
    if (bsize % 4 != 0)
	fprintf(file, " + 3 & ~3");

    if (ptype->itIndefinite) {
	fprintf(file, " : sizeof(%s *)",
		FetchServerType(ptype->itElement));
    }
}

/*
 * Adjust message size and advance reply pointer.
 * Called after packing a variable-length argument that
 * has more arguments following.
 */
static void
WriteAdjustMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;
    register ipc_type_t *ptype = arg->argType;

    /* There are more Out arguments.  We need to adjust msgh_size
       and advance OutP, so we save the size of the current field
       in msgh_size_delta. */

    fprintf(file, "\tmsgh_size_delta = ");
    WriteArgSize(file, arg);
    fprintf(file, ";\n");

    if (rt->rtNumReplyVar == 1)
	/* We can still address the message header directly.  Fill
	   in the size field. */

	fprintf(file, "\tOutP->Head.msgh_size = %d + msgh_size_delta;\n",
			rt->rtReplySize);
    else
    if (arg->argReplyPos == 0)
	/* First variable-length argument.  The previous msgh_size value
	   is the minimum reply size. */

	fprintf(file, "\tmsgh_size = %d + msgh_size_delta;\n",
		rt->rtReplySize);
    else
	fprintf(file, "\tmsgh_size += msgh_size_delta;\n");

    fprintf(file,
	"\tOutP = (Reply *) ((char *) OutP + msgh_size_delta - %d);\n",
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
    /* No more Out arguments.  If this is the only variable Out
       argument, we can assign to msgh_size directly. */

    if (arg->argReplyPos == 0) {
	fprintf(file, "\tOutP->Head.msgh_size = %d + (",
			arg->argRoutine->rtReplySize);
	WriteArgSize(file, arg);
	fprintf(file, ");\n");
    }
    else {
	fprintf(file, "\tmsgh_size += ");
	WriteArgSize(file, arg);
	fprintf(file, ";\n");
    }
}

static void
WritePackArg(file, arg)
    FILE *file;
    argument_t *arg;
{
    if (akCheck(arg->argKind, akbReplyInit))
	WritePackArgType(file, arg);

    if ((akIdent(arg->argKind) == akePoly) &&
	akCheck(arg->argKind, akbReturnSnd))
	WriteAdjustMsgSimple(file, arg);

    if (akCheckAll(arg->argKind, akbReturnSnd|akbVarNeeded))
	WritePackArgValue(file, arg);
    else if (akCheckAll(arg->argKind, akbReturnSnd|akbVariable))
    {
	register ipc_type_t *it = arg->argType;
	register argument_t *count = arg->argCount;

	if (it->itString)
	{
	    /* Need to call strlen to calculate the size of the argument. */
	    fprintf(file, "\tOutP->%s = strlen(OutP->%s) + 1;\n",
			count->argMsgField,
			arg->argMsgField);
	}
	else if (it->itIndefinite)
	{
	    fprintf(file, "\tif (%s > %d) {\n",
			count->argVarName,
			it->itNumber/it->itElement->itNumber);
	    fprintf(file, "\t\tOutP->%s.msgtl_header.msgt_inline = FALSE;\n",
			arg->argTTName);
	    if (!arg->argRoutine->rtSimpleFixedReply)
		fprintf(file, "\t\tmsgh_simple = FALSE;\n");
	    fprintf(file, "\t}\n");
	}
    }

    if (akCheck(arg->argKind, akbReplyCopy))
	WriteCopyArgValue(file, arg);

    /*
     *	If this is a KernelServer, and the reply message contains
     *	a receive right, we must check for the possibility of a
     *	port/message circularity.  If queueing the reply message
     *	would cause a circularity, we mark the reply message
     *	with the circular bit.
     */

    if (IsKernelServer &&
	akCheck(arg->argKind, akbReturnSnd) &&
	((arg->argType->itOutName == MACH_MSG_TYPE_PORT_RECEIVE) ||
	 (arg->argType->itOutName == MACH_MSG_TYPE_POLYMORPHIC)))
	WriteAdjustMsgCircular(file, arg);

}

/*
 * Handle reply arguments - fill in message types and copy arguments
 * that need to be copied.
 */
WritePackReplyArgs(file, rt)
    FILE *file;
    register routine_t *rt;
{
    register argument_t *arg;
    register argument_t *lastVarArg;

    lastVarArg = argNULL;
    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext) {

	/*
	 * Adjust message size and advance message pointer if
	 * the last reply argument was variable-length and the
	 * request position will change.
	 */
	if (lastVarArg != argNULL &&
	    lastVarArg->argReplyPos < arg->argReplyPos)
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
	if (akCheckAll(arg->argKind, akbReturnSnd|akbVariable))
	    lastVarArg = arg;
    }

    /*
     * Finish the message size.
     */
    if (lastVarArg != argNULL)
	WriteFinishMsgSize(file, lastVarArg);
}

static void
WriteFieldDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    WriteFieldDeclPrim(file, arg, FetchServerType);
}

static void
WriteRoutine(file, rt)
    FILE *file;
    register routine_t *rt;
{
    fprintf(file, "\n");

    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "mig_internal novalue _X%s\n", rt->rtName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(InHeadP, OutHeadP)\n");
    fprintf(file, "\tmach_msg_header_t *InHeadP, *OutHeadP;\n");
    fprintf(file, "#endif\n");

    fprintf(file, "{\n");
    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbRequest, "Request");
    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbReply, "Reply");

    WriteVarDecls(file, rt);

    WriteList(file, rt->rtArgs, WriteCheckDecl, akbRequestQC, "\n", "\n");
    WriteList(file, rt->rtArgs,
	      IsKernelServer ? WriteTypeDeclOut : WriteTypeDeclIn,
	      akbReplyInit, "\n", "\n");

    WriteList(file, rt->rtArgs, WriteLocalVarDecl,
	      akbVarNeeded, ";\n", ";\n\n");

    WriteCheckHead(file, rt);

    WriteTypeCheckRequestArgs(file, rt);
    WriteList(file, rt->rtArgs, WriteExtractArg, akbNone, "", "");

    WriteServerCall(file, rt);
    WriteGetReturnValue(file, rt);

    WriteReverseList(file, rt->rtArgs, WriteDestroyArg, akbDestroy, "", "");

    /*
     * For one-way routines, it doesn`t make sense to check the return
     * code, because we return immediately afterwards.  However,
     * kernel servers may want to deallocate port arguments - and the
     * deallocation must not be done if the return code is not KERN_SUCCESS.
     */
    if (rt->rtOneWay || rt->rtNoReplyArgs)
    {
	if (IsKernelServer)
	{
	    if (rtCheckMaskFunction(rt->rtArgs, akbSendBody|akbSendRcv,
				CheckDestroyPortArg))
	    {
		WriteCheckReturnValue(file, rt);
	    }
	    WriteReverseList(file, rt->rtArgs, WriteDestroyPortArg,
			 akbSendBody|akbSendRcv, "", "");
	}
    }
    else
    {
	WriteCheckReturnValue(file, rt);

	if (IsKernelServer)
	    WriteReverseList(file, rt->rtArgs, WriteDestroyPortArg,
			 akbSendBody|akbSendRcv, "", "");

	WriteReplyInit(file, rt);
	WritePackReplyArgs(file, rt);
	WriteReplyHead(file, rt);
    }

    fprintf(file, "}\n");
}

static void
WriteStubArgDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    char *ref = argByReferenceServer(arg) ? "*" : "";

    fprintf(file, "\t%s %s%s", arg->argType->itServerType,
	    ref, arg->argVarName);
}

static void
WriteStubDecl(file, rt)
    FILE *file;
    register routine_t *rt;
{
    fprintf(file, "\n");
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "%s %s\n", ServerSideType(rt), rt->rtServerName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "(\n");
    WriteList(file, rt->rtArgs, WriteStubArgDecl,
	      akbServerArg, ",\n", "\n");
    fprintf(file, ")\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(");
    WriteList(file, rt->rtArgs, WriteNameDecl, akbServerArg, ", ", "");
    fprintf(file, ")\n");
    WriteList(file, rt->rtArgs, WriteStubArgDecl,
	      akbServerArg, ";\n", ";\n");
    fprintf(file, "#endif\n");
    fprintf(file, "{\n");
}

void
WriteServer(file, stats)
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
	  case skSImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skUImport:
	    break;
	  default:
	    fatal("WriteServer(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    WriteEpilog(file, stats);
}
