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
 * $Log:	type.c,v $
 * Revision 2.6  91/08/28  11:17:27  jsb
 * 	Removed itMsgKindType.
 * 	[91/08/12            rpd]
 * 
 * Revision 2.5  91/07/31  18:11:12  dbg
 * 	Indefinite-length inline arrays.
 * 
 * 	Change itDeallocate to an enumerated type, to allow for
 * 	user-specified deallocate flag.
 * 
 * 	Add itCStringDecl.
 * 	[91/07/17            dbg]
 * 
 * Revision 2.4  91/06/25  10:32:09  rpd
 * 	Changed itCalculateNameInfo to change type names from mach_port_t
 * 	to ipc_port_t for KernelServer and KernelUser interfaces.
 * 	[91/05/28            rpd]
 * 
 * Revision 2.3  91/02/05  17:56:02  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:56:12  mrt]
 * 
 * Revision 2.2  90/06/02  15:05:54  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:14:07  rpd]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 17-Aug-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed translation of MSG_TYPE_INVALID as that type
 *	is no longer defined by the kernel.
 * 
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed itPrintTrans and itPrintDecl to reflect new translation syntax.
 *	Changed itCheckDecl to set itServerType to itType if is is strNULL.
 *
 *  4-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added a check to itCheckDecl to make sure that in-line
 *	variable length arrays have a non-zero maximum length.
 *
 * 22-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Removed warning message for translation.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Changed itVarArrayDecl to take a 'max' parameter for maximum
 *	number of elements, and to make type not be 'struct'.
 *	Added itDestructor.
 *
 * 18-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added initialization of itPortType
 *
 * 14-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added initialization for itTidType
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed prototype for itAlloc; was missing itServerType field.
 *
 * 10-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed extern from procedure definitions to make hi-c happy
 *	Changed the c type names of itDummyType from caddr_t to
 *	char * and of itCountType from u_int to unsigned int to
 *	eliminate the need to import sys/types.h into the mig generated
 *	code.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#include <mach/message.h>
#include "error.h"
#include "alloc.h"
#include "global.h"
#include "type.h"

ipc_type_t *itRetCodeType;	/* used for return codes */
ipc_type_t *itDummyType;	/* used for camelot dummy args */
ipc_type_t *itTidType;		/* used for camelot tids */
ipc_type_t *itRequestPortType;	/* used for default Request port arg */
ipc_type_t *itZeroReplyPortType;/* used for dummy Reply port arg */
ipc_type_t *itRealReplyPortType;/* used for default Reply port arg */
ipc_type_t *itWaitTimeType;	/* used for dummy WaitTime args */
ipc_type_t *itMsgOptionType;	/* used for dummy MsgOption args */

static ipc_type_t *list = itNULL;

/*
 *  Searches for a named type.  We use a simple
 *  self-organizing linked list.
 */
ipc_type_t *
itLookUp(name)
    identifier_t name;
{
    register ipc_type_t *it, **last;

    for (it = *(last = &list); it != itNULL; it = *(last = &it->itNext))
	if (streql(name, it->itName))
	{
	    /* move this type to the front of the list */
	    *last = it->itNext;
	    it->itNext = list;
	    list = it;

	    return it;
	}

    return itNULL;
}

/*
 *  Enters a new name-type association into
 *  our self-organizing linked list.
 */
void
itInsert(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    it->itName = name;
    it->itNext = list;
    list = it;
}

static ipc_type_t *
itAlloc()
{
    static ipc_type_t prototype =
    {
	strNULL,		/* identifier_t itName */
	0,			/* ipc_type_t *itNext */
	0,			/* u_int itTypeSize */
	0,			/* u_int itPadSize */
	0,			/* u_int itMinTypeSize */
	0,			/* u_int itInName */
	0,			/* u_int itOutName */
	0,			/* u_int itSize */
	1,			/* u_int itNumber */
	TRUE,			/* boolean_t itInLine */
	FALSE,			/* boolean_t itLongForm */
	d_NO,			/* dealloc_t itDeallocate */
	FALSE,			/* boolean_t itServerCopy */
	strNULL,		/* string_t itInNameStr */
	strNULL,		/* string_t itOutNameStr */
	flNone,			/* ipc_flags_t itFlags */
	TRUE,			/* boolean_t itStruct */
	FALSE,			/* boolean_t itString */
	FALSE,			/* boolean_t itVarArray */
	FALSE,			/* boolean_t itIndefinite */
	itNULL,			/* ipc_type_t *itElement */
	strNULL,		/* identifier_t itUserType */
	strNULL,		/* identifier_t itServerType */
	strNULL,		/* identifier_t itTransType */
	strNULL,		/* identifier_t itInTrans */
	strNULL,		/* identifier_t itOutTrans */
	strNULL,		/* identifier_t itDestructor */
    };
    register ipc_type_t *new;

    new = (ipc_type_t *) malloc(sizeof *new);
    if (new == itNULL)
	fatal("itAlloc(): %s", unix_error_string(errno));
    *new = prototype;
    return new;
}

/*
 * Convert an IPC type-name into a string.
 */
static char *
itNameToString(name)
    u_int name;
{
    char buffer[100];

    (void) sprintf(buffer, "%u", name);
    return strmake(buffer);
}

/*
 * Calculate itTypeSize, itPadSize, itMinTypeSize.
 * Every type needs this info; it is recalculated
 * when itInLine, itNumber, or itSize changes.
 */
static void
itCalculateSizeInfo(it)
    register ipc_type_t *it;
{
    if (it->itInLine)
    {
	u_int bytes = (it->itNumber * it->itSize + 7) / 8;
	u_int padding = (4 - bytes%4)%4;

	it->itTypeSize = bytes;
	it->itPadSize = padding;
	if (it->itVarArray)
	    it->itMinTypeSize = 0;
	else
	    it->itMinTypeSize = bytes + padding;
    }
    else
    {
	/* out-of-line, so use size of pointers */
	u_int bytes = sizeof(char *);

	it->itTypeSize = bytes;
	it->itPadSize = 0;
	it->itMinTypeSize = bytes;
    }

    /* Unfortunately, these warning messages can't give a type name;
       we haven't seen a name yet (it might stay anonymous.) */

    if ((it->itNumber * it->itSize) % 8 != 0)
	warn("size of C types must be multiples of 8 bits");

    if ((it->itTypeSize == 0) && !it->itVarArray)
	warn("sizeof(%s) == 0");
}

/*
 * Fill in default values for some fields used in code generation:
 *	itInNameStr, itOutNameStr, itUserType, itServerType, itTransType
 * Every argument's type should have these values filled in.
 */
static void
itCalculateNameInfo(it)
    register ipc_type_t *it;
{
    if (it->itInNameStr == strNULL)
	it->itInNameStr = strmake(itNameToString(it->itInName));
    if (it->itOutNameStr == strNULL)
	it->itOutNameStr = strmake(itNameToString(it->itOutName));

    if (it->itUserType == strNULL)
	it->itUserType = it->itName;
    if (it->itServerType == strNULL)
	it->itServerType = it->itName;

    /*
     *	KernelServer and KernelUser interfaces get special treatment here.
     *	On the kernel side of the interface, ports are really internal
     *	port pointers (ipc_port_t), not port names (mach_port_t).
     *	At this point, we don't know if the argument is in or out,
     *	so we don't know if we should look at itInName or itOutName.
     *	Looking at both should be OK.
     *
     *	This is definitely a hack, but I think it is cleaner than
     *	mucking with type declarations throughout the kernel .def files,
     *	hand-conditionalizing on KERNEL_SERVER and KERNEL_USER.
     */

    if (IsKernelServer &&
	streql(it->itServerType, "mach_port_t") &&
	(((it->itInName == MACH_MSG_TYPE_POLYMORPHIC) &&
	  (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)) ||
	 MACH_MSG_TYPE_PORT_ANY(it->itInName) ||
	 MACH_MSG_TYPE_PORT_ANY(it->itOutName)))
	it->itServerType = "ipc_port_t";

    if (IsKernelUser &&
	streql(it->itUserType, "mach_port_t") &&
	(((it->itInName == MACH_MSG_TYPE_POLYMORPHIC) &&
	  (it->itOutName == MACH_MSG_TYPE_POLYMORPHIC)) ||
	 MACH_MSG_TYPE_PORT_ANY(it->itInName) ||
	 MACH_MSG_TYPE_PORT_ANY(it->itOutName)))
	it->itUserType = "ipc_port_t";

    if (it->itTransType == strNULL)
	it->itTransType = it->itServerType;
}

/******************************************************
 *  Checks for non-implemented types, conflicting type
 *  flags and whether the long or short form of msg type
 *  descriptor is appropriate. Called after each type statement
 *  is parsed.
 ******************************************************/
static void
itCheckDecl(name, it)
    identifier_t name;
    register ipc_type_t *it;
{
    it->itName = name;

    itCalculateNameInfo(it);

    /* do a bit of error checking, mostly necessary because of
       limitations in Mig */

    if (it->itVarArray)
    {
	if ((it->itInTrans != strNULL) || (it->itOutTrans != strNULL))
	    error("%s: can't translate variable-sized arrays", name);

	if (it->itDestructor != strNULL)
	    error("%s: can't destroy variable-sized array", name);
    }

    if (it->itVarArray && it->itInLine)
    {
#if 0
	if (it->itNumber <= 0)
	    error("%s: variable-sized in-line arrays need a maximum size",
		  name);
#endif

	if ((it->itElement->itUserType == strNULL) ||
	    (it->itElement->itServerType == strNULL))
	    error("%s: variable-sized in-line arrays need a named base type",
		  name);
    }

    /* process the IPC flag specification */

    if ((it->itFlags&(flLong|flNotLong)) == (flLong|flNotLong))
    {
	warn("%s: IsLong and IsNotLong cancel out", name);
	it->itFlags &= ~(flLong|flNotLong);
    }

    if (it->itFlags & flMaybeDealloc)
    {
	if (it->itFlags & (flDealloc|flNotDealloc))
	{
	    warn("%s: Dealloc or NotDealloc ignored with Dealloc[]", name);
	    it->itFlags &= ~(flDealloc|flNotDealloc);
	}
	if (it->itInLine)
	    warn("%s: Dealloc[] won't do anything", name);

	it->itDeallocate = d_MAYBE;
    }

    if ((it->itFlags&(flDealloc|flNotDealloc)) == (flDealloc|flNotDealloc))
    {
	warn("%s: Dealloc and NotDealloc cancel out", name);
	it->itFlags &= ~(flDealloc|flNotDealloc);
    }

    if (it->itFlags&flDealloc)
    {
	if (it->itInLine)
	    warn("%s: Dealloc won't do anything", name);
	it->itDeallocate = d_YES;
    }

    if (it->itFlags&flNotDealloc)
	warn("%s: doesn't need NotDealloc", name);

{
    enum { NotLong, CanBeLong, ShouldBeLong, MustBeLong, TooLong } uselong;

    uselong = NotLong;

    if ((it->itInName == MACH_MSG_TYPE_POLYMORPHIC) ||
	(it->itOutName == MACH_MSG_TYPE_POLYMORPHIC))
	uselong = CanBeLong;

    if (it->itVarArray && !it->itInLine)
	uselong = ShouldBeLong;

    if (((it->itInName != MACH_MSG_TYPE_POLYMORPHIC) &&
	 (it->itInName >= (1<<8))) ||
	((it->itOutName != MACH_MSG_TYPE_POLYMORPHIC) &&
	 (it->itOutName >= (1<<8))) ||
	(it->itSize >= (1<<8)) ||
	(it->itNumber >= (1<<12)) ||
	(it->itIndefinite))
	uselong = MustBeLong;

    if (((it->itInName != MACH_MSG_TYPE_POLYMORPHIC) &&
	 (it->itInName >= (1<<16))) ||
	((it->itOutName != MACH_MSG_TYPE_POLYMORPHIC) &&
	 (it->itOutName >= (1<<16))) ||
	(it->itSize >= (1<<16)))
	uselong = TooLong;

    switch (uselong)
    {
      case NotLong:
	if (it->itFlags&flLong)
	{
	    warn("%s: doesn't need IsLong", name);
	    it->itLongForm = TRUE;
	}
	break;

      case CanBeLong:
	if (it->itFlags&flLong)
	    it->itLongForm = TRUE;
	break;

      case ShouldBeLong:
	if (!(it->itFlags&flNotLong))
	    it->itLongForm = TRUE;
	break;

      case MustBeLong:
	if (it->itFlags&flNotLong)
	    warn("%s: too big for IsNotLong; ignoring", name);
	it->itLongForm = TRUE;
	break;

      case TooLong:
	warn("%s: too big for mach_msg_type_long_t", name);
	it->itLongForm = TRUE;
	break;
    }
}
    if (it->itFlags & flServerCopy)
	it->itServerCopy = TRUE;
}

/*
 *  Pretty-prints translation/destruction/type information.
 */
static void
itPrintTrans(it)
    register ipc_type_t *it;
{
    if (!streql(it->itName, it->itUserType))
	printf("\tCUserType:\t%s\n", it->itUserType);

    if (!streql(it->itName, it->itServerType))
	printf("\tCServerType:\t%s\n", it->itServerType);

    if (it->itInTrans != strNULL)
       printf("\tInTran:\t\t%s %s(%s)\n",
	      it->itTransType, it->itInTrans, it->itServerType);

    if (it->itOutTrans != strNULL)
       printf("\tOutTran:\t%s %s(%s)\n",
	      it->itServerType, it->itOutTrans, it->itTransType);

    if (it->itDestructor != strNULL)
	printf("\tDestructor:\t%s(%s)\n", it->itDestructor, it->itTransType);
}

/*
 *  Pretty-prints type declarations.
 */
static void
itPrintDecl(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    printf("Type %s = ", name);
    if (!it->itInLine)
	printf("^ ");
    if (it->itVarArray)
	if (it->itNumber == 0 || it->itIndefinite)
	    printf("array [] of ");
	else
	    printf("array [*:%d] of ", it->itNumber);
    else if (it->itStruct && ((it->itNumber != 1) ||
			      (it->itInName == MACH_MSG_TYPE_STRING_C)))
	printf("struct [%d] of ", it->itNumber);
    else if (it->itNumber != 1)
	printf("array [%d] of ", it->itNumber);

    if (streql(it->itInNameStr, it->itOutNameStr))
	printf("(%s,", it->itInNameStr);
    else
	printf("(%s|%s", it->itInNameStr, it->itOutNameStr);

    printf(" %d%s%s%s)\n",
	it->itSize,
	it->itLongForm ? ", IsLong" : "",
	it->itDeallocate == d_YES ? ", Dealloc"
		: it->itDeallocate == d_MAYBE ? ", Dealloc[]"
			: "",
	it->itServerCopy ? ", ServerCopy" : "");

    itPrintTrans(it);

    printf("\n");
}

/*
 *  Handles named type-specs, which can occur in type
 *  declarations or in argument lists.  For example,
 *	type foo = type-spec;	// itInsert will get called later
 *	routine foo(arg : bar = type-spec);	// itInsert won't get called
 */
void
itTypeDecl(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    itCheckDecl(name, it);

    if (BeVerbose)
	itPrintDecl(name, it);
}

/*
 *  Handles declarations like
 *	type new = name;
 *	type new = inname|outname;
 */
ipc_type_t *
itShortDecl(inname, instr, outname, outstr, defsize)
    u_int inname;
    string_t instr;
    u_int outname;
    string_t outstr;
    u_int defsize;
{
    if (defsize == 0)
	error("must use full IPC type decl");

    return itLongDecl(inname, instr, outname, outstr,
		      defsize, defsize, flNone);
}

/*
 *  Handles declarations like
 *	type new = (name, size, flags...)
 *	type new = (inname|outname, size, flags...)
 */
ipc_type_t *
itLongDecl(inname, instr, outname, outstr, defsize, size, flags)
    u_int inname;
    string_t instr;
    u_int outname;
    string_t outstr;
    u_int defsize;
    u_int size;
    ipc_flags_t flags;
{
    register ipc_type_t *it;

    if ((defsize != 0) && (defsize != size))
	warn("IPC type decl has strange size (%u instead of %u)",
	     size, defsize);

    it = itAlloc();
    it->itInName = inname;
    it->itInNameStr = instr;
    it->itOutName = outname;
    it->itOutNameStr = outstr;
    it->itSize = size;
    if (inname == MACH_MSG_TYPE_STRING_C)
    {
	it->itStruct = FALSE;
	it->itString = TRUE;
    }
    it->itFlags = flags;

    itCalculateSizeInfo(it);
    return it;
}

static ipc_type_t *
itCopyType(old)
    ipc_type_t *old;
{
    register ipc_type_t *new = itAlloc();

    *new = *old;
    new->itName = strNULL;
    new->itNext = itNULL;
    new->itElement = old;

    /* size info still valid */
    return new;
}

/*
 * A call to itCopyType is almost always followed with itResetType.
 * The exception is itPrevDecl.  Also called before adding any new
 * translation/destruction/type info (see parser.y).
 *
 *	type new = old;	// new doesn't get old's info
 *	type new = array[*:10] of old;
 *			// new doesn't get old's info, but new->itElement does
 *	type new = array[*:10] of struct[3] of old;
 *			// new and new->itElement don't get old's info
 */

ipc_type_t *
itResetType(old)
    ipc_type_t *old;
{
    /* reset all special translation/destruction/type info */

    old->itInTrans = strNULL;
    old->itOutTrans = strNULL;
    old->itDestructor = strNULL;
    old->itUserType = strNULL;
    old->itServerType = strNULL;
    old->itTransType = strNULL;
    return old;
}

/*
 *  Handles the declaration
 *	type new = old;
 */
ipc_type_t *
itPrevDecl(name)
    identifier_t name;
{
    register ipc_type_t *old;

    old = itLookUp(name);
    if (old == itNULL)
    {
	error("type '%s' not defined", name);
	return itAlloc();
    }
    else
	return itCopyType(old);
}

/*
 *  Handles the declarations
 *	type new = array[] of old;	// number is 0
 *	type new = array[*] of old;	// number is 0
 *	type new = array[*:number] of old;
 */
ipc_type_t *
itVarArrayDecl(number, old)
    u_int number;
    register ipc_type_t *old;
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    if (number != 0)
	it->itNumber *= number;
    else {
	/*
	 * Send at most 2048 bytes inline.
	 */
	u_int	bytes;

	bytes = (it->itNumber * it->itSize + 7) / 8;
	it->itNumber = (2048 / bytes) * it->itNumber;
	it->itIndefinite = TRUE;
    }
    it->itVarArray = TRUE;
    it->itStruct = FALSE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = array[number] of old;
 */
ipc_type_t *
itArrayDecl(number, old)
    u_int number;
    ipc_type_t *old;
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    it->itNumber *= number;
    it->itStruct = FALSE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = ^ old;
 */
ipc_type_t *
itPtrDecl(it)
    ipc_type_t *it;
{
    if (!it->itInLine ||
	(it->itVarArray && !it->itIndefinite && (it->itNumber > 0)))
	error("IPC type decl is too complicated");
    it->itNumber = 0;
    it->itIndefinite = FALSE;
    it->itInLine = FALSE;
    it->itStruct = TRUE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 *  Handles the declaration
 *	type new = struct[number] of old;
 */
ipc_type_t *
itStructDecl(number, old)
    u_int number;
    ipc_type_t *old;
{
    register ipc_type_t *it = itResetType(itCopyType(old));

    if (!it->itInLine || it->itVarArray)
	error("IPC type decl is too complicated");
    it->itNumber *= number;
    it->itStruct = TRUE;
    it->itString = FALSE;

    itCalculateSizeInfo(it);
    return it;
}

/*
 * Treat 'c_string[n]' as
 * 'array[n] of (MSG_TYPE_STRING_C, 8)'
 */
ipc_type_t *
itCStringDecl(count, varying)
    int	count;
    boolean_t varying;
{
    register ipc_type_t *it;
    register ipc_type_t *itElement;

    itElement = itShortDecl(MACH_MSG_TYPE_STRING_C,
			    "MACH_MSG_TYPE_STRING_C",
			    MACH_MSG_TYPE_STRING_C,
			    "MACH_MSG_TYPE_STRING_C",
			    8);
    itCheckDecl("char", itElement);

    it = itResetType(itCopyType(itElement));
    it->itNumber = count;
    it->itVarArray = varying;
    it->itStruct = FALSE;
    it->itString = TRUE;

    itCalculateSizeInfo(it);
    return it;
}

extern ipc_type_t *
itMakeCountType()
{
    ipc_type_t *it = itAlloc();

    it->itName = "mach_msg_type_number_t";
    it->itInName = MACH_MSG_TYPE_INTEGER_32;
    it->itInNameStr = "MACH_MSG_TYPE_INTEGER_32";
    it->itOutName = MACH_MSG_TYPE_INTEGER_32;
    it->itOutNameStr = "MACH_MSG_TYPE_INTEGER_32";
    it->itSize = 32;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

extern ipc_type_t *
itMakePolyType()
{
    ipc_type_t *it = itAlloc();

    it->itName = "mach_msg_type_name_t";
    it->itInName = MACH_MSG_TYPE_INTEGER_32;
    it->itInNameStr = "MACH_MSG_TYPE_INTEGER_32";
    it->itOutName = MACH_MSG_TYPE_INTEGER_32;
    it->itOutNameStr = "MACH_MSG_TYPE_INTEGER_32";
    it->itSize = 32;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

extern ipc_type_t *
itMakeDeallocType()
{
    ipc_type_t *it = itAlloc();

    it->itName = "boolean_t";
    it->itInName = MACH_MSG_TYPE_BOOLEAN;
    it->itInNameStr = "MACH_MSG_TYPE_BOOLEAN";
    it->itOutName = MACH_MSG_TYPE_BOOLEAN;
    it->itOutNameStr = "MACH_MSG_TYPE_BOOLEAN";
    it->itSize = 32;

    itCalculateSizeInfo(it);
    itCalculateNameInfo(it);
    return it;
}

/*
 *  Initializes the pre-defined types.
 */
void
init_type()
{
    itRetCodeType = itAlloc();
    itRetCodeType->itName = "kern_return_t";
    itRetCodeType->itInName = MACH_MSG_TYPE_INTEGER_32;
    itRetCodeType->itInNameStr = "MACH_MSG_TYPE_INTEGER_32";
    itRetCodeType->itOutName = MACH_MSG_TYPE_INTEGER_32;
    itRetCodeType->itOutNameStr = "MACH_MSG_TYPE_INTEGER_32";
    itRetCodeType->itSize = 32;
    itCalculateSizeInfo(itRetCodeType);
    itCalculateNameInfo(itRetCodeType);

    itDummyType = itAlloc();
    itDummyType->itName = "char *";
    itDummyType->itInName = MACH_MSG_TYPE_UNSTRUCTURED;
    itDummyType->itInNameStr = "MACH_MSG_TYPE_UNSTRUCTURED";
    itDummyType->itOutName = MACH_MSG_TYPE_UNSTRUCTURED;
    itDummyType->itOutNameStr = "MACH_MSG_TYPE_UNSTRUCTURED";
    itDummyType->itSize = 32;
    itCalculateSizeInfo(itDummyType);
    itCalculateNameInfo(itDummyType);

    itTidType = itAlloc();
    itTidType->itName = "tid_t";
    itTidType->itInName = MACH_MSG_TYPE_INTEGER_32;
    itTidType->itInNameStr = "MACH_MSG_TYPE_INTEGER_32";
    itTidType->itOutName = MACH_MSG_TYPE_INTEGER_32;
    itTidType->itOutNameStr = "MACH_MSG_TYPE_INTEGER_32";
    itTidType->itSize = 32;
    itTidType->itNumber = 6;
    itCalculateSizeInfo(itTidType);
    itCalculateNameInfo(itTidType);

    itRequestPortType = itAlloc();
    itRequestPortType->itName = "mach_port_t";
    itRequestPortType->itInName = MACH_MSG_TYPE_COPY_SEND;
    itRequestPortType->itInNameStr = "MACH_MSG_TYPE_COPY_SEND";
    itRequestPortType->itOutName = MACH_MSG_TYPE_PORT_SEND;
    itRequestPortType->itOutNameStr = "MACH_MSG_TYPE_PORT_SEND";
    itRequestPortType->itSize = 32;
    itCalculateSizeInfo(itRequestPortType);
    itCalculateNameInfo(itRequestPortType);

    itZeroReplyPortType = itAlloc();
    itZeroReplyPortType->itName = "mach_port_t";
    itZeroReplyPortType->itInName = 0;
    itZeroReplyPortType->itInNameStr = "0";
    itZeroReplyPortType->itOutName = 0;
    itZeroReplyPortType->itOutNameStr = "0";
    itZeroReplyPortType->itSize = 32;
    itCalculateSizeInfo(itZeroReplyPortType);
    itCalculateNameInfo(itZeroReplyPortType);

    itRealReplyPortType = itAlloc();
    itRealReplyPortType->itName = "mach_port_t";
    itRealReplyPortType->itInName = MACH_MSG_TYPE_MAKE_SEND_ONCE;
    itRealReplyPortType->itInNameStr = "MACH_MSG_TYPE_MAKE_SEND_ONCE";
    itRealReplyPortType->itOutName = MACH_MSG_TYPE_PORT_SEND_ONCE;
    itRealReplyPortType->itOutNameStr = "MACH_MSG_TYPE_PORT_SEND_ONCE";
    itRealReplyPortType->itSize = 32;
    itCalculateSizeInfo(itRealReplyPortType);
    itCalculateNameInfo(itRealReplyPortType);

    itWaitTimeType = itMakeCountType();
    itMsgOptionType = itMakeCountType();
}

/******************************************************
 *  Make sure return values of functions are assignable.
 ******************************************************/
void
itCheckReturnType(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    if (!it->itStruct)
	error("type of %s is too complicated", name);
    if ((it->itInName == MACH_MSG_TYPE_POLYMORPHIC) ||
	(it->itOutName == MACH_MSG_TYPE_POLYMORPHIC))
	error("type of %s can't be polymorphic", name);
}


/******************************************************
 *  Called by routine.c to check that request ports are
 *  simple and correct ports with send rights.
 ******************************************************/
void
itCheckRequestPortType(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    if (((it->itOutName != MACH_MSG_TYPE_PORT_SEND) &&
	 (it->itOutName != MACH_MSG_TYPE_PORT_SEND_ONCE) &&
	 (it->itOutName != MACH_MSG_TYPE_POLYMORPHIC)) ||
	(it->itNumber != 1) ||
	(it->itSize != 32) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper request port", name);
}


/******************************************************
 *  Called by routine.c to check that reply ports are
 *  simple and correct ports with send rights.
 ******************************************************/
void
itCheckReplyPortType(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    if (((it->itOutName != MACH_MSG_TYPE_PORT_SEND) &&
	 (it->itOutName != MACH_MSG_TYPE_PORT_SEND_ONCE) &&
	 (it->itOutName != MACH_MSG_TYPE_POLYMORPHIC) &&
	 (it->itOutName != 0)) ||
	(it->itNumber != 1) ||
	(it->itSize != 32) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper reply port", name);
}


/******************************************************
 *  Used by routine.c to check that WaitTime is a
 *  simple bit 32 integer.
 ******************************************************/
void
itCheckIntType(name, it)
    identifier_t name;
    ipc_type_t *it;
{
    if ((it->itInName != MACH_MSG_TYPE_INTEGER_32) ||
	(it->itOutName != MACH_MSG_TYPE_INTEGER_32) ||
	(it->itNumber != 1) ||
	(it->itSize != 32) ||
	!it->itInLine ||
	it->itDeallocate != d_NO ||
	!it->itStruct ||
	it->itVarArray)
	error("argument %s isn't a proper integer", name);
}
