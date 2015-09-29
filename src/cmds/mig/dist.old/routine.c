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
 * $Log:	routine.c,v $
 * Revision 2.6  91/08/28  11:17:12  jsb
 * 	Removed Camelot and TrapRoutine support.
 * 	Changed MsgKind to MsgSeqno.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.5  91/07/31  18:10:15  dbg
 * 	Support indefinite-length (inline or out-of-line) variable
 * 	arrays.
 * 	[91/04/10            dbg]
 * 
 * 	Change argDeallocate to an enumerated type, to allow for
 * 	user-specified deallocate flag.
 * 
 * 	Special handling for varying C_Strings in rtAddCountArg.
 * 
 * 	Add rtCheckMaskFunction.  Add rtNoReplyArgs to routine
 * 	structure.
 * 	[91/04/03            dbg]
 * 
 * Revision 2.4  91/02/05  17:55:20  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:55:07  mrt]
 * 
 * Revision 2.3  90/09/28  16:57:29  jsb
 * 	Fixed bug setting akbRequestQC in rtAugmentArgKind.
 * 	Kernel servers can't quick-check the deallocate bit
 * 	on port arguments.
 * 	[90/09/19            rpd]
 * 
 * Revision 2.2  90/06/02  15:05:17  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:12:43  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 17-Oct-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added to code to rtAugmentArgKind to reject any inline
 *	variable arguments that are both In and Out.
 *
 * 27-Feb-88  Richard Draves (rpd) at Carnegie-Mellon University
 *	Added warning messages.  CamelotRoutines should only be
 *	used in camelot subsystems, which should consist entirely
 *	of CamelotRoutines.
 *
 * 18-Feb-88  Richard Draves (rpd) at Carnegie-Mellon University
 *	Fix to rtCheckRoutineArgs, so we don't seg-fault on bad input.
 *	We want to do some checking for malformed args after an error,
 *	but nothing that has to use their type (which is NULL).
 *
 * 20-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Fill in pointers to last request and reply arguments.  Implement
 *	partial variable-length messages - only the last inline argument
 *	in a message can vary in size.  Added argMultiplier field for
 *	count arguments.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Don't add akbVarNeeded attribute here - server.c can
 *	better determine whether it is needed.
 *
 * 25-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed CamelotPrefix from a UserPrefix to a ServerPrefix.
 *
 * 18-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added code to add requestPort, Tid and WaitTime 
 *	arguments for CamelotRoutines.
 *
 * 10-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added code to handle MsgType arguments
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed rtAlloc and argAlloc to correctly initialize string
 *	pointers in allocated structures.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

/*
 *  ABSTRACT:
 *   Provides the routine used by parser.c to generate
 *   routine structures for each routine statement.
 *   The parser generates a threaded list of statements
 *   of which the most interesting are the various kinds
 *   routine statments. The routine structure is defined
 *   in routine.h which includes it name, kind of routine
 *   and other information,
 *   a pointer to an argument list which contains the name
 *   and type information for each argument, and a list
 *   of distinguished arguments, eg.  Request and Reply
 *   ports, waittime, retcode etc.
 */

#include <mach/message.h>
#include "error.h"
#include "alloc.h"
#include "global.h"
#include "routine.h"

u_int rtNumber = 0;

routine_t *
rtAlloc()
{
    register routine_t *new;

    new = (routine_t *) calloc(1, sizeof *new);
    if (new == rtNULL)
	fatal("rtAlloc(): %s", unix_error_string(errno));
    new->rtNumber = rtNumber++;
    new->rtName = strNULL;
    new->rtErrorName = strNULL;
    new->rtUserName = strNULL;
    new->rtServerName = strNULL;

    return new;
}

void
rtSkip()
{
    rtNumber++;
}

argument_t *
argAlloc()
{
    static argument_t prototype =
    {
	strNULL,		/* identifier_t argName */
	argNULL,		/* argument_t *argNext */
	akNone,			/* arg_kind_t argKind */
	itNULL,			/* ipc_type_t *argType */
	strNULL,		/* string_t argVarName */
	strNULL,		/* string_t argMsgField */
	strNULL,		/* string_t argTTName */
	strNULL,		/* string_t argPadName */
	flNone,			/* ipc_flags_t argFlags */
	d_NO,			/* dealloc_t argDeallocate */
	FALSE,			/* boolean_t argLongForm */
	FALSE,			/* boolean_t argServerCopy */
	rtNULL,			/* routine_t *argRoutine */
	argNULL,		/* argument_t *argCount */
	argNULL,		/* argument_t *argPoly */
	argNULL,		/* argument_t *argDealloc */
	argNULL,		/* argument_t *argSCopy */
	argNULL,		/* argument_t *argParent */
	1,			/* int argMultiplier */
	0,			/* int argRequestPos */
	0			/* int argReplyPos */
    };
    register argument_t *new;

    new = (argument_t *) malloc(sizeof *new);
    if (new == argNULL)
	fatal("argAlloc(): %s", unix_error_string(errno));
    *new = prototype;
    return new;
}

routine_t *
rtMakeRoutine(name, args)
    identifier_t name;
    argument_t *args;
{
    register routine_t *rt = rtAlloc();

    rt->rtName = name;
    rt->rtKind = rkRoutine;
    rt->rtArgs = args;

    return rt;
}

routine_t *
rtMakeSimpleRoutine(name, args)
    identifier_t name;
    argument_t *args;
{
    register routine_t *rt = rtAlloc();

    rt->rtName = name;
    rt->rtKind = rkSimpleRoutine;
    rt->rtArgs = args;

    return rt;
}

routine_t *
rtMakeProcedure(name, args)
    identifier_t name;
    argument_t *args;
{
    register routine_t *rt = rtAlloc();

    rt->rtName = name;
    rt->rtKind = rkProcedure;
    rt->rtArgs = args;

    warn("Procedure %s: obsolete routine kind", name);

    return rt;
}

routine_t *
rtMakeSimpleProcedure(name, args)
    identifier_t name;
    argument_t *args;
{
    register routine_t *rt = rtAlloc();

    rt->rtName = name;
    rt->rtKind = rkSimpleProcedure;
    rt->rtArgs = args;

    warn("SimpleProcedure %s: obsolete routine kind", name);

    return rt;
}

routine_t *
rtMakeFunction(name, args, type)
    identifier_t name;
    argument_t *args;
    ipc_type_t *type;
{
    register routine_t *rt = rtAlloc();
    register argument_t *ret = argAlloc();

    ret->argName = name;
    ret->argKind = akReturn;
    ret->argType = type;
    ret->argNext = args;

    rt->rtName = name;
    rt->rtKind = rkFunction;
    rt->rtArgs = ret;

    warn("Function %s: obsolete routine kind", name);

    return rt;
}

char *
rtRoutineKindToStr(rk)
    routine_kind_t rk;
{
    switch (rk)
    {
      case rkRoutine:
	return "Routine";
      case rkSimpleRoutine:
	return "SimpleRoutine";
      case rkProcedure:
	return "Procedure";
      case rkSimpleProcedure:
	return "SimpleProcedure";
      case rkFunction:
	return "Function";
      default:
	fatal("rtRoutineKindToStr(%d): not a routine_kind_t", rk);
	/*NOTREACHED*/
    }
}

static void
rtPrintArg(arg)
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (!akCheck(arg->argKind, akbUserArg|akbServerArg) ||
	(akIdent(arg->argKind) == akeCount) ||
	(akIdent(arg->argKind) == akePoly))
	return;

    printf("\n\t");

    switch (akIdent(arg->argKind))
    {
      case akeRequestPort:
	printf("RequestPort");
	break;
      case akeReplyPort:
	printf("ReplyPort");
	break;
      case akeWaitTime:
	printf("WaitTime");
	break;
      case akeMsgOption:
	printf("MsgOption");
	break;
      case akeMsgSeqno:
	printf("MsgSeqno\t");
	break;
      default:
	if (akCheck(arg->argKind, akbRequest))
	    if (akCheck(arg->argKind, akbSend))
		printf("In");
	    else
		printf("(In)");
	if (akCheck(arg->argKind, akbReply))
	    if (akCheck(arg->argKind, akbReturn))
		printf("Out");
	    else
		printf("(Out)");
	printf("\t");
    }

    printf("\t%s: %s", arg->argName, it->itName);

    if (arg->argDeallocate != it->itDeallocate)
	if (arg->argDeallocate == d_YES)
	    printf(", Dealloc");
	else if (arg->argDeallocate == d_MAYBE)
	    printf(", Dealloc[]");
	else
	    printf(", NotDealloc");

    if (arg->argLongForm != it->itLongForm)
	if (arg->argLongForm)
	    printf(", IsLong");
	else
	    printf(", IsNotLong");

    if (arg->argServerCopy != it->itServerCopy)
	if (arg->argServerCopy)
	    printf(", ServerCopy");
}

void
rtPrintRoutine(rt)
    register routine_t *rt;
{
    register argument_t *arg;

    printf("%s (%d) %s(", rtRoutineKindToStr(rt->rtKind),
	   rt->rtNumber, rt->rtName);

    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext)
	rtPrintArg(arg);

    if (rt->rtKind == rkFunction)
	printf("): %s\n", rt->rtReturn->argType->itName);
    else
	printf(")\n");

    printf("\n");
}

/*
 * Determines appropriate value of msg-simple for the message,
 * and whether this value can vary at runtime.  (If it can vary,
 * then the simple value is optimistically returned as TRUE.)
 * Uses itInName values, so useful when sending messages.
 */

static void
rtCheckSimpleIn(args, mask, fixed, simple)
    argument_t *args;
    u_int mask;
    boolean_t *fixed, *simple;
{
    register argument_t *arg;
    boolean_t MayBeComplex = FALSE;
    boolean_t MustBeComplex = FALSE;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheck(arg->argKind, mask))
	{
	    register ipc_type_t *it = arg->argType;

	    if (it->itInName == MACH_MSG_TYPE_POLYMORPHIC)
		MayBeComplex = TRUE;

	    if (it->itIndefinite)
		MayBeComplex = TRUE;

	    if (MACH_MSG_TYPE_PORT_ANY(it->itInName) ||
		!it->itInLine)
		MustBeComplex = TRUE;
	}

    *fixed = MustBeComplex || !MayBeComplex;
    *simple = !MustBeComplex;
}

/*
 * Determines appropriate value of msg-simple for the message,
 * and whether this value can vary at runtime.  (If it can vary,
 * then the simple value is optimistically returned as TRUE.)
 * Uses itOutName values, so useful when receiving messages
 * (and sending reply messages in KernelServer interfaces).
 */

static void
rtCheckSimpleOut(args, mask, fixed, simple)
    argument_t *args;
    u_int mask;
    boolean_t *fixed, *simple;
{
    register argument_t *arg;
    boolean_t MayBeComplex = FALSE;
    boolean_t MustBeComplex = FALSE;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheck(arg->argKind, mask))
	{
	    register ipc_type_t *it = arg->argType;

	    if (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)
		MayBeComplex = TRUE;

	    if (it->itIndefinite)
		MayBeComplex = TRUE;

	    if (MACH_MSG_TYPE_PORT_ANY(it->itOutName) ||
		!it->itInLine)
		MustBeComplex = TRUE;
	}

    *fixed = MustBeComplex || !MayBeComplex;
    *simple = !MustBeComplex;
}

static u_int
rtFindSize(args, mask)
    argument_t *args;
    u_int mask;
{
    register argument_t *arg;
    u_int size = sizeof(mach_msg_header_t);

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheck(arg->argKind, mask))
	{
	    register ipc_type_t *it = arg->argType;

	    if (arg->argLongForm)
		size += sizeof(mach_msg_type_long_t);
	    else
		size += sizeof(mach_msg_type_t);

	    size += it->itMinTypeSize;
	}

    return size;
}

boolean_t
rtCheckMask(args, mask)
    argument_t *args;
    u_int mask;
{
    register argument_t *arg;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	    return TRUE;
    return FALSE;
}

boolean_t
rtCheckMaskFunction(args, mask, func)
    argument_t *args;
    u_int mask;
    boolean_t (*func)(/* argument_t *arg */);
{
    register argument_t *arg;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	    if ((*func)(arg))
		return TRUE;
    return FALSE;
}

int
rtCountMask(args, mask)
    argument_t *args;
    u_int mask;
{
    register argument_t *arg;
    int count = 0;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	    count++;
    return count;
}

/* arg->argType may be NULL in this function */

static void
rtDefaultArgKind(rt, arg)
    routine_t *rt;
    argument_t *arg;
{
    if ((arg->argKind == akNone) &&
	(rt->rtRequestPort == argNULL))
	arg->argKind = akRequestPort;

    if (arg->argKind == akNone)
	arg->argKind = akIn;
}

/*
 * Initializes arg->argDeallocate and arg->argLongForm.
 */

static void
rtProcessArgFlags(arg)
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if ((arg->argFlags&(flLong|flNotLong)) == (flLong|flNotLong))
    {
	warn("%s: IsLong and IsNotLong cancel out", arg->argName);
	arg->argFlags &= ~(flLong|flNotLong);
    }

    if (arg->argFlags&flLong)
    {
	if (it->itLongForm)
	    warn("%s: IsLong on argument is redundant", arg->argName);
	arg->argLongForm = TRUE;
    }
    else if (arg->argFlags&flNotLong)
    {
	if (!it->itLongForm)
	    warn("%s: IsNotLong on argument is redundant", arg->argName);
	arg->argLongForm = FALSE;
    }
    else
	arg->argLongForm = it->itLongForm;


    if (arg->argFlags & flMaybeDealloc)
    {
	if (arg->argFlags & (flDealloc|flNotDealloc))
	{
	    warn("%s: Dealloc or NotDealloc ignored with Dealloc[]",
		 arg->argName);
	    arg->argFlags &= ~(flDealloc|flNotDealloc);
	}
	if (it->itInLine)
	    warn("%s: Dealloc[] won't do anything", arg->argName);
	arg->argDeallocate = d_MAYBE;
    }

    if ((arg->argFlags&(flDealloc|flNotDealloc)) == (flDealloc|flNotDealloc))
    {
	warn("%s: Dealloc and NotDealloc cancel out", arg->argName);
	arg->argFlags &= ~(flDealloc|flNotDealloc);
    }

    if (arg->argFlags&flDealloc)
    {
	if (it->itDeallocate == d_YES)
	    warn("%s: Dealloc on argument is redundant", arg->argName);
	if (it->itInLine &&
	    (it->itInName != MACH_MSG_TYPE_POLYMORPHIC) &&
	    !MACH_MSG_TYPE_PORT_ANY(it->itInName))
	    warn("%s: Dealloc won't do anything", arg->argName);
	arg->argDeallocate = d_YES;
    }
    else if (arg->argFlags&flNotDealloc)
    {
	if (!it->itDeallocate)
	    warn("%s: NotDealloc on argument is redundant", arg->argName);
	arg->argDeallocate = d_NO;
    }
    else if (!(arg->argFlags & flMaybeDealloc))
	arg->argDeallocate = it->itDeallocate;

    if (arg->argFlags & flServerCopy)
    {
	if (it->itServerCopy)
	    warn("%s: ServerCopy on argument is redundant", arg->argName);
	arg->argServerCopy = TRUE;
    }
    else
	arg->argServerCopy = it->itServerCopy;
}

static void
rtAugmentArgKind(arg)
    argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    /* akbVariable means variable-sized inline. */

    if (it->itVarArray && it->itInLine)
    {
	if (akCheckAll(arg->argKind, akbRequest|akbReply))
	    error("%s: Inline variable-sized arguments can't be InOut",
		  arg->argName);
	arg->argKind = akAddFeature(arg->argKind, akbVariable);

	/* akbIndefinite means inline or out-of-line */

	if (it->itIndefinite)
	    arg->argKind = akAddFeature(arg->argKind, akbIndefinite);
    }

    /*
     *	Kernel servers can't do quick-checking of request arguments
     *	which are out-of-line or ports, because the deallocate bit isn't
     *	predictable.  This is because the deallocate bit is preserved
     *	at message copyin time and normalized during message copyout.
     *	This accomodates old IPC programs which expect the deallocate
     *	bit to be preserved.
     */

    if (akCheck(arg->argKind, akbRequest) &&
	!arg->argLongForm &&
	(it->itOutName != MACH_MSG_TYPE_POLYMORPHIC) &&
	!it->itVarArray &&
	!(IsKernelServer && (!it->itInLine ||
			     MACH_MSG_TYPE_PORT_ANY(it->itOutName))))
	arg->argKind = akAddFeature(arg->argKind, akbRequestQC);

    if (akCheck(arg->argKind, akbReply) &&
	!arg->argLongForm &&
	(it->itOutName != MACH_MSG_TYPE_POLYMORPHIC) &&
	!it->itVarArray)
	arg->argKind = akAddFeature(arg->argKind, akbReplyQC);

    /*
     * Need to use a local variable in the following cases:
     *	1) There is a translate-out function & the argument is being
     *	   returned.  We need to translate it before it hits the message.
     *	2) There is a translate-in function & the argument is
     *	   sent and returned.  We need a local variable for its address.
     *	3) There is a destructor function, which will be used
     *	   (SendRcv and not ReturnSnd), and there is a translate-in
     *	   function whose value must be saved for the destructor.
     *	4) This is a count arg, getting returned.  The count can't get
     *	   stored directly into the msg-type, because the msg-type won't
     *	   get initialized until later, and that would trash the count.
     *	5) This is a poly arg, getting returned.  The name can't get
     *	   stored directly into the msg-type, because the msg-type won't
     *	   get initialized until later, and that would trash the name.
     *  6) This is a dealloc arg, being returned.  The name can't be
     *	   stored directly into the msg_type, because the msg-type
     *	   field is a bit-field.
     */

    if (((it->itOutTrans != strNULL) &&
	 akCheck(arg->argKind, akbReturnSnd)) ||
	((it->itInTrans != strNULL) &&
	 akCheckAll(arg->argKind, akbSendRcv|akbReturnSnd)) ||
	((it->itDestructor != strNULL) &&
	 akCheck(arg->argKind, akbSendRcv) &&
	 !akCheck(arg->argKind, akbReturnSnd) &&
	 (it->itInTrans != strNULL)) ||
	((akIdent(arg->argKind) == akeCount) &&
	 akCheck(arg->argKind, akbReturnSnd)) ||
	((akIdent(arg->argKind) == akePoly) &&
	 akCheck(arg->argKind, akbReturnSnd)) ||
	((akIdent(arg->argKind) == akeDealloc) &&
	 akCheck(arg->argKind, akbReturnSnd)))
    {
	arg->argKind = akRemFeature(arg->argKind, akbReplyCopy);
	arg->argKind = akAddFeature(arg->argKind, akbVarNeeded);
    }
}

/* arg->argType may be NULL in this function */

static void
rtCheckRoutineArg(rt, arg)
    routine_t *rt;
    argument_t *arg;
{
    switch (akIdent(arg->argKind))
    {
      case akeRequestPort:
	if (rt->rtRequestPort != argNULL)
	    warn("multiple RequestPort args in %s; %s won't be used",
		 rt->rtName, rt->rtRequestPort->argName);
	rt->rtRequestPort = arg;
	break;

      case akeReplyPort:
	if (rt->rtReplyPort != argNULL)
	    warn("multiple ReplyPort args in %s; %s won't be used",
		 rt->rtName, rt->rtReplyPort->argName);
	rt->rtReplyPort = arg;
	break;

      case akeWaitTime:
	if (rt->rtWaitTime != argNULL)
	    warn("multiple WaitTime args in %s; %s won't be used",
		 rt->rtName, rt->rtWaitTime->argName);
	rt->rtWaitTime = arg;
	break;

      case akeMsgOption:
	if (rt->rtMsgOption != argNULL)
	    warn("multiple MsgOption args in %s; %s won't be used",
		 rt->rtName, rt->rtMsgOption->argName);
	rt->rtMsgOption = arg;
	break;

      case akeMsgSeqno:
	if (rt->rtMsgSeqno != argNULL)
	    warn("multiple MsgSeqno args in %s; %s won't be used",
		 rt->rtName, rt->rtMsgSeqno->argName);
	rt->rtMsgSeqno = arg;
	break;

      case akeReturn:
	if (rt->rtReturn != argNULL)
	    warn("multiple Return args in %s; %s won't be used",
		 rt->rtName, rt->rtReturn->argName);
	rt->rtReturn = arg;
	break;

      default:
	break;
    }
}

/* arg->argType may be NULL in this function */

static void
rtSetArgDefaults(rt, arg)
    routine_t *rt;
    register argument_t *arg;
{
    arg->argRoutine = rt;
    if (arg->argVarName == strNULL)
	arg->argVarName = arg->argName;
    if (arg->argMsgField == strNULL)
	switch(akIdent(arg->argKind))
	{
	  case akeRequestPort:
	    arg->argMsgField = "Head.msgh_request_port";
	    break;
	  case akeReplyPort:
	    arg->argMsgField = "Head.msgh_reply_port";
	    break;
	  case akeMsgSeqno:
	    arg->argMsgField = "Head.msgh_seqno";
	    break;
	  default:
	    arg->argMsgField = arg->argName;
	    break;
	}
    if (arg->argTTName == strNULL)
	arg->argTTName = strconcat(arg->argName, "Type");
    if (arg->argPadName == strNULL)
	arg->argPadName = strconcat(arg->argName, "Pad");

    /*
     *	The poly args for the request and reply ports have special defaults,
     *	because their msg-type-name values aren't stored in normal fields.
     */

    if ((rt->rtRequestPort != argNULL) &&
	(rt->rtRequestPort->argPoly == arg) &&
	(arg->argType != itNULL)) {
	arg->argMsgField = "Head.msgh_bits";
	arg->argType->itInTrans = "MACH_MSGH_BITS_REQUEST";
    }

    if ((rt->rtReplyPort != argNULL) &&
	(rt->rtReplyPort->argPoly == arg) &&
	(arg->argType != itNULL)) {
	arg->argMsgField = "Head.msgh_bits";
	arg->argType->itInTrans = "MACH_MSGH_BITS_REPLY";
    }
}

static void
rtAddCountArg(arg)
    register argument_t *arg;
{
    register argument_t *count;

    count = argAlloc();
    count->argName = strconcat(arg->argName, "Cnt");
    count->argType = itMakeCountType();
    count->argParent = arg;
    count->argMultiplier = arg->argType->itElement->itNumber;
    count->argNext = arg->argNext;
    arg->argNext = count;
    arg->argCount = count;

    if (arg->argType->itString)
	/* C String gets no Count argument on either side.
	   There is no explicit field in the message -
	   the count is passed as part of the descriptor. */
	count->argKind = akeCount;
    else
	count->argKind = akAddFeature(akCount,
				  akCheck(arg->argKind, akbSendReturnBits));

    if (arg->argLongForm)
	count->argMsgField = strconcat(arg->argTTName,
				       ".msgtl_number");
    else
	count->argMsgField = strconcat(arg->argTTName, ".msgt_number");

    if (arg->argType->itString)
	/* There is no Count 'argument' for varying-length C string. */
	count->argVarName = (char *)0;

}

static void
rtAddPolyArg(arg)
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register argument_t *poly;
    arg_kind_t akbsend, akbreturn;

    poly = argAlloc();
    poly->argName = strconcat(arg->argName, "Poly");
    poly->argType = itMakePolyType();
    poly->argParent = arg;
    poly->argNext = arg->argNext;
    arg->argNext = poly;
    arg->argPoly = poly;

    /*
     * akbsend is bits added if the arg is In;
     * akbreturn is bits added if the arg is Out.
     * The mysterious business with KernelServer subsystems:
     * when packing Out arguments, they use OutNames instead
     * of InNames, and the OutName determines if they are poly-in
     * as well as poly-out.
     */

    akbsend = akbSend|akbSendBody;
    akbreturn = akbReturn|akbReturnBody;

    if (it->itInName == MACH_MSG_TYPE_POLYMORPHIC)
    {
	akbsend |= akbUserArg|akbSendSnd;
	if (!IsKernelServer)
	    akbreturn |= akbServerArg|akbReturnSnd;
    }
    if (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)
    {
	akbsend |= akbServerArg|akbSendRcv;
	akbreturn |= akbUserArg|akbReturnRcv;
	if (IsKernelServer)
	    akbreturn |= akbServerArg|akbReturnSnd;
    }

    poly->argKind = akPoly;
    if (akCheck(arg->argKind, akbSend))
	poly->argKind = akAddFeature(poly->argKind,
				     akCheck(arg->argKind, akbsend));
    if (akCheck(arg->argKind, akbReturn))
	poly->argKind = akAddFeature(poly->argKind,
				     akCheck(arg->argKind, akbreturn));

    if (arg->argLongForm)
	poly->argMsgField = strconcat(arg->argTTName,
				      ".msgtl_name");
    else
	poly->argMsgField = strconcat(arg->argTTName, ".msgt_name");
}

static void
rtAddDeallocArg(arg)
    register argument_t *arg;
{
    register argument_t *dealloc;

    dealloc = argAlloc();
    dealloc->argName = strconcat(arg->argName, "Dealloc");
    dealloc->argType = itMakeDeallocType();
    dealloc->argParent = arg;
    dealloc->argNext = arg->argNext;
    arg->argNext = dealloc;
    arg->argDealloc = dealloc;

    dealloc->argKind = akeDealloc;
    if (akCheck(arg->argKind, akbSend))
	dealloc->argKind = akAddFeature(dealloc->argKind,
		akCheck(arg->argKind,
			akbSend|akbSendBody|akbSendSnd|akbUserArg));
    if (akCheck(arg->argKind, akbReturn))
	dealloc->argKind = akAddFeature(dealloc->argKind,
		akCheck(arg->argKind,
			akbReturn|akbReturnBody|akbReturnSnd|akbServerArg));

    if (arg->argLongForm)
	dealloc->argMsgField = strconcat(arg->argTTName,
					 ".msgtl_header.msgt_deallocate");
    else
	dealloc->argMsgField = strconcat(arg->argTTName, ".msgt_deallocate");

}

static void
rtAddSCopyArg(arg)
    register argument_t *arg;
{
    register argument_t *scopy;

    scopy = argAlloc();
    scopy->argName = strconcat(arg->argName, "SCopy");
    scopy->argType = itMakeDeallocType();
    scopy->argParent = arg;
    scopy->argNext = arg->argNext;
    arg->argNext = scopy;
    arg->argSCopy = scopy;

    scopy->argKind = akServerCopy;

    if (arg->argLongForm)
	scopy->argMsgField = strconcat(arg->argTTName,
					".msgtl_header.msgt_inline");
    else
	scopy->argMsgField = strconcat(arg->argTTName, ".msgt_inline");
}

static void
rtCheckRoutineArgs(rt)
    routine_t *rt;
{
    register argument_t *arg;

    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext)
    {
	register ipc_type_t *it = arg->argType;

	rtDefaultArgKind(rt, arg);
	rtCheckRoutineArg(rt, arg);

	/* need to set argTTName before adding implicit args */
	rtSetArgDefaults(rt, arg);

	/* the arg may not have a type (if there was some error in parsing it),
	   in which case we don't want to do these steps. */

	if (it != itNULL)
	{
	    /* need to set argLongForm before adding implicit args */
	    rtProcessArgFlags(arg);
	    rtAugmentArgKind(arg);

	    /* args added here will get processed in later iterations */
	    /* order of args is 'arg poly count dealloc scopy' */

	    if (arg->argServerCopy)
		rtAddSCopyArg(arg);
	    if (arg->argDeallocate == d_MAYBE)
		rtAddDeallocArg(arg);
	    if (it->itVarArray)
		rtAddCountArg(arg);
	    if ((it->itInName == MACH_MSG_TYPE_POLYMORPHIC) ||
		(it->itOutName == MACH_MSG_TYPE_POLYMORPHIC))
		rtAddPolyArg(arg);
	}
    }
}

static void
rtCheckArgTypes(rt)
    routine_t *rt;
{
    if (rt->rtRequestPort == argNULL)
	error("%s %s doesn't have a server port argument",
	      rtRoutineKindToStr(rt->rtKind), rt->rtName);

    if ((rt->rtKind == rkFunction) &&
	(rt->rtReturn == argNULL))
	error("Function %s doesn't have a return arg", rt->rtName);

    if ((rt->rtKind != rkFunction) &&
	(rt->rtReturn != argNULL))
	error("non-function %s has a return arg", rt->rtName);

    if ((rt->rtReturn == argNULL) && !rt->rtProcedure)
	rt->rtReturn = rt->rtRetCode;

    rt->rtServerReturn = rt->rtReturn;

    if ((rt->rtReturn != argNULL) &&
	(rt->rtReturn->argType != itNULL))
	itCheckReturnType(rt->rtReturn->argName,
			  rt->rtReturn->argType);

    if ((rt->rtRequestPort != argNULL) &&
	(rt->rtRequestPort->argType != itNULL))
	itCheckRequestPortType(rt->rtRequestPort->argName,
			       rt->rtRequestPort->argType);

    if ((rt->rtReplyPort != argNULL) &&
	(rt->rtReplyPort->argType != itNULL))
	itCheckReplyPortType(rt->rtReplyPort->argName,
			     rt->rtReplyPort->argType);

    if ((rt->rtWaitTime != argNULL) &&
	(rt->rtWaitTime->argType != itNULL))
	itCheckIntType(rt->rtWaitTime->argName,
		       rt->rtWaitTime->argType);

    if ((rt->rtMsgOption != argNULL) &&
	(rt->rtMsgOption->argType != itNULL))
	itCheckIntType(rt->rtMsgOption->argName,
		       rt->rtMsgOption->argType);

    if ((rt->rtMsgSeqno != argNULL) &&
	(rt->rtMsgSeqno->argType != itNULL))
	itCheckIntType(rt->rtMsgSeqno->argName,
		       rt->rtMsgSeqno->argType);
}

/*
 * Check for arguments which are missing seemingly needed functions.
 * We make this check here instead of in itCheckDecl, because here
 * we can take into account what kind of argument the type is
 * being used with.
 *
 * These are warnings, not hard errors, because mig will generate
 * reasonable code in any case.  The generated code will work fine
 * if the ServerType and TransType are really the same, even though
 * they have different names.
 */

static void
rtCheckArgTrans(rt)
    routine_t *rt;
{
    register argument_t *arg;

    /* the arg may not have a type (if there was some error in parsing it) */

    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext)
    {
	register ipc_type_t *it = arg->argType;

	if ((it != itNULL) &&
	    !streql(it->itServerType, it->itTransType))
	{
	    if (akCheck(arg->argKind, akbSendRcv) &&
		(it->itInTrans == strNULL))
		warn("%s: argument has no in-translation function",
		     arg->argName);

	    if (akCheck(arg->argKind, akbReturnSnd) &&
		(it->itOutTrans == strNULL))
		warn("%s: argument has no out-translation function",
		     arg->argName);
	}
    }
}

/*
 * Adds an implicit return-code argument.  It exists in the reply message,
 * where it is the first piece of data.  Even if there is no reply
 * message (rtOneWay is true), we generate the argument because
 * the server-side stub needs a dummy reply msg to return error codes
 * back to the server loop.
 */

static void
rtAddRetCode(rt)
    routine_t *rt;
{
    register argument_t *arg = argAlloc();

    arg->argName = "RetCode";
    arg->argType = itRetCodeType;
    arg->argKind = akRetCode;
    rt->rtRetCode = arg;

    /* add at beginning, so return-code is first in the reply message  */
    arg->argNext = rt->rtArgs;
    rt->rtArgs = arg;
}

/*
 *  Adds a dummy WaitTime argument to the function.
 *  This argument doesn't show up in any C argument lists;
 *  it implements the global WaitTime statement.
 */

static void
rtAddWaitTime(rt, name)
    routine_t *rt;
    identifier_t name;
{
    register argument_t *arg = argAlloc();
    argument_t **loc;

    arg->argName = "dummy WaitTime arg";
    arg->argVarName = name;
    arg->argType = itWaitTimeType;
    arg->argKind = akeWaitTime;
    rt->rtWaitTime = arg;

    /* add wait-time after msg-option, if possible */

    if (rt->rtMsgOption != argNULL)
	loc = &rt->rtMsgOption->argNext;
    else
	loc = &rt->rtArgs;

    arg->argNext = *loc;
    *loc = arg;

    rtSetArgDefaults(rt, arg);
}

/*
 *  Adds a dummy MsgOption argument to the function.
 *  This argument doesn't show up in any C argument lists;
 *  it implements the global MsgOption statement.
 */

static void
rtAddMsgOption(rt, name)
    routine_t *rt;
    identifier_t name;
{
    register argument_t *arg = argAlloc();
    argument_t **loc;

    arg->argName = "dummy MsgOption arg";
    arg->argVarName = name;
    arg->argType = itMsgOptionType;
    arg->argKind = akeMsgOption;
    rt->rtMsgOption = arg;

    /* add msg-option after msg-seqno */

    if (rt->rtMsgSeqno != argNULL)
	loc = &rt->rtMsgSeqno->argNext;
    else
	loc = &rt->rtArgs;

    arg->argNext = *loc;
    *loc = arg;

    rtSetArgDefaults(rt, arg);
}

/*
 *  Adds a dummy reply port argument to the function.
 */

static void
rtAddDummyReplyPort(rt, type)
    routine_t *rt;
    ipc_type_t *type;
{
    register argument_t *arg = argAlloc();
    argument_t **loc;

    arg->argName = "dummy ReplyPort arg";
    arg->argVarName = "dummy ReplyPort arg";
    arg->argType = type;
    arg->argKind = akeReplyPort;
    rt->rtReplyPort = arg;

    /* add the reply port after the request port */

    if (rt->rtRequestPort != argNULL)
	loc = &rt->rtRequestPort->argNext;
    else
	loc = &rt->rtArgs;

    arg->argNext = *loc;
    *loc = arg;

    rtSetArgDefaults(rt, arg);
}

/*
 * Initializes argRequestPos, argReplyPos, rtMaxRequestPos, rtMaxReplyPos,
 * rtNumRequestVar, rtNumReplyVar, and adds akbVarNeeded to those arguments
 * that need it because of variable-sized inline considerations.
 *
 * argRequestPos and argReplyPos get -1 if the value shouldn't be used.
 */
static void
rtCheckVariable(rt)
    register routine_t *rt;
{
    register argument_t *arg;
    int NumRequestVar = 0;
    int NumReplyVar = 0;
    int MaxRequestPos;
    int MaxReplyPos;

    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext)
    {
	register argument_t *parent = arg->argParent;

	if (parent == argNULL)
	{
	    if (akCheck(arg->argKind, akbRequest|akbSend))
	    {
		arg->argRequestPos = NumRequestVar;
		MaxRequestPos = NumRequestVar;
		if (akCheck(arg->argKind, akbVariable))
		    NumRequestVar++;
	    }
	    else
		arg->argRequestPos = -1;

	    if (akCheck(arg->argKind, akbReply|akbReturn))
	    {
		arg->argReplyPos = NumReplyVar;
		MaxReplyPos = NumReplyVar;
		if (akCheck(arg->argKind, akbVariable))
		    NumReplyVar++;
	    }
	    else
		arg->argReplyPos = -1;
	}
	else
	{
	    arg->argRequestPos = parent->argRequestPos;
	    arg->argReplyPos = parent->argReplyPos;
	}

	/* Out variables that follow a variable-sized field
	   need VarNeeded or ReplyCopy; they can't be stored
	   directly into the reply message. */

	if (akCheck(arg->argKind, akbReturnSnd) &&
	    !akCheck(arg->argKind, akbReplyCopy|akbVarNeeded) &&
	    (arg->argReplyPos > 0))
	    arg->argKind = akAddFeature(arg->argKind, akbVarNeeded);
    }

    rt->rtNumRequestVar = NumRequestVar;
    rt->rtNumReplyVar = NumReplyVar;
    rt->rtMaxRequestPos = MaxRequestPos;
    rt->rtMaxReplyPos = MaxReplyPos;
}

/*
 * Adds akbDestroy where needed.
 */

static void
rtCheckDestroy(rt)
    register routine_t *rt;
{
    register argument_t *arg;

    for (arg = rt->rtArgs; arg != argNULL; arg = arg->argNext)
    {
	register ipc_type_t *it = arg->argType;

	if(akCheck(arg->argKind, akbSendRcv) &&
	   !akCheck(arg->argKind, akbReturnSnd))
	{
	   if ((it->itDestructor != strNULL) ||
	       (akCheck(arg->argKind, akbIndefinite) &&
		!arg->argServerCopy))
	    {
		arg->argKind = akAddFeature(arg->argKind, akbDestroy);
	    }
	}
    }
}

void
rtCheckRoutine(rt)
    register routine_t *rt;
{
    /* Initialize random fields. */

    rt->rtErrorName = ErrorProc;
    rt->rtOneWay = ((rt->rtKind == rkSimpleProcedure) ||
		    (rt->rtKind == rkSimpleRoutine));
    rt->rtProcedure = ((rt->rtKind == rkProcedure) ||
		       (rt->rtKind == rkSimpleProcedure));
    rt->rtUseError = rt->rtProcedure || (rt->rtKind == rkFunction);
    rt->rtServerName = strconcat(ServerPrefix, rt->rtName);
    rt->rtUserName = strconcat(UserPrefix, rt->rtName);

    /* Add implicit arguments. */

    rtAddRetCode(rt);

    /* Check out the arguments and their types.  Add count, poly
       implicit args.  Any arguments added after rtCheckRoutineArgs
       should have rtSetArgDefaults called on them. */

    rtCheckRoutineArgs(rt);

    /* Add dummy WaitTime and MsgOption arguments, if the routine
       doesn't have its own args and the user specified global values. */

    if (rt->rtReplyPort == argNULL)
	if (rt->rtOneWay)
	    rtAddDummyReplyPort(rt, itZeroReplyPortType);
	else
	    rtAddDummyReplyPort(rt, itRealReplyPortType);

    if (rt->rtMsgOption == argNULL)
	if (MsgOption == strNULL)
	    rtAddMsgOption(rt, "MACH_MSG_OPTION_NONE");
	else
	    rtAddMsgOption(rt, MsgOption);

    if ((rt->rtWaitTime == argNULL) &&
	(WaitTime != strNULL))
	rtAddWaitTime(rt, WaitTime);

    /* Now that all the arguments are in place, do more checking. */

    rtCheckArgTypes(rt);
    rtCheckArgTrans(rt);

    if (rt->rtOneWay && rtCheckMask(rt->rtArgs, akbReturn))
	error("%s %s has OUT argument",
	      rtRoutineKindToStr(rt->rtKind), rt->rtName);

    /* If there were any errors, don't bother calculating more info
       that is only used in code generation anyway.  Therefore,
       the following functions don't have to worry about null types. */

    if (errors > 0)
	return;

    rtCheckSimpleIn(rt->rtArgs, akbRequest,
		    &rt->rtSimpleFixedRequest,
		    &rt->rtSimpleSendRequest);
    rtCheckSimpleOut(rt->rtArgs, akbRequest,
		     &rt->rtSimpleCheckRequest,
		     &rt->rtSimpleReceiveRequest);
    rt->rtRequestSize = rtFindSize(rt->rtArgs, akbRequest);

    if (IsKernelServer)
	rtCheckSimpleOut(rt->rtArgs, akbReply,
			 &rt->rtSimpleFixedReply,
			 &rt->rtSimpleSendReply);
    else
	rtCheckSimpleIn(rt->rtArgs, akbReply,
			&rt->rtSimpleFixedReply,
			&rt->rtSimpleSendReply);
    rtCheckSimpleOut(rt->rtArgs, akbReply,
		     &rt->rtSimpleCheckReply,
		     &rt->rtSimpleReceiveReply);
    rt->rtReplySize = rtFindSize(rt->rtArgs, akbReply);

    rtCheckVariable(rt);
    rtCheckDestroy(rt);

    if (rt->rtKind == rkFunction)
	rt->rtNoReplyArgs = FALSE;
    else
	rt->rtNoReplyArgs = !rtCheckMask(rt->rtArgs, akbReturnSnd);
}
