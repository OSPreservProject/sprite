/*
 * This file is part of libdyn.a, the C Dynamic Object library.  It
 * contains the public header file.
 *
 * There are no restrictions on this code; however, if you make any
 * changes, I request that you document them so that I do not get
 * credit or blame for your modifications.
 *
 * Written by Barr3y Jaspan, Student Information Processing Board (SIPB)
 * and MIT-Project Athena, 1989.
 */


/*
 * dyn.h -- header file to be included by programs linking against
 * libdyn.a.
 */

#ifndef _Dyn_h
#define _Dyn_h

#ifdef notdef
typedef void *DynPtr;
#else
typedef char *DynPtr;
#endif

typedef struct _DynObject DynObjectRec, *DynObject;

/* Function macros */
#define DynHigh(obj)	(DynSize(obj) - 1)
#define DynLow(obj)	(0)

/* Return status codes */
#define DYN_OK		-1000
#define DYN_NOMEM	-1001
#define DYN_BADINDEX	-1002
#define DYN_BADVALUE	-1003
     
/* Function declarations */
DynObject 	DynCreate();
int		DynAdd(), DynDelete(), DynDestroy(), DynDebug();
int		DynInsert(), DynParanoid();
DynPtr		DynGet();

#endif /* _Dyn_h */
/* DO NOT ADD ANYTHING AFTER THIS #endif */
