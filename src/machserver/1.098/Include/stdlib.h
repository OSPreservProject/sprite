/*
 * stdlib.h --
 *
 *	Declares facilities exported by the "stdlib" portion of
 *	the C library.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/stdlib.h,v 1.18 90/11/27 11:24:17 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _STDLIB
#define _STDLIB

/* 
 * sprite.h is needed for typedefs that are used in some function
 * prototypes.  Unfortunately, some user programs define conflicting
 * typedefs.  Because many programs probably include stdlib.h, we
 * should give advance warning before forcing users to use the
 * typedefs from sprite.h.  This must be done before we can turn on
 * function prototypes for Sprite user programs.  (Or, change the 
 * prototypes so that they don't use the Sprite typedefs.)
 */
#include <cfuncproto.h>

#ifdef KERNEL
#include <sprite.h>
#include <sys/types.h>
#endif

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#if defined(_HAS_PROTOTYPES) && !defined(_SIZE_T)
#define _SIZE_T
/* size_t is also defined in sys/types.h, stddef.h, and unistd.h. */
typedef	int	size_t;
#endif

/*
 *----------------------------
 * String conversion routines:
 *----------------------------
 */

_EXTERN double	 atof _ARGS_((_CONST char *string));
_EXTERN int	 atoi _ARGS_((_CONST char *string));
_EXTERN long int atol _ARGS_((_CONST char *string));
_EXTERN double	 strtod _ARGS_((_CONST char *string, char **endPtr));
_EXTERN long int strtol _ARGS_((_CONST char *string, char **endPtr, int base));
_EXTERN unsigned long int
		strtoul _ARGS_((_CONST char *string, char **endPtr, int base));

/*
 *------------------
 * Memory allocator:
 *------------------
 */

/* 
 * (Many of the "char *"s should be "Address".  See the above comments 
 * about sprite.h.)
 */

#ifdef KERNEL

_EXTERN Address	malloc _ARGS_((unsigned int bytesNeeded));

/*
 * The mips compiler cannot handle some coercions on the left hand side.
 */
#ifndef mips

_EXTERN int	_free _ARGS_((Address blockPtr));

#ifdef lint
#define		free(ptr) _free(ptr)
#else
#define		free(ptr) {_free(ptr); (ptr) = (Address) NIL; }
#endif /* lint */

#else /* mips */

_EXTERN int	free _ARGS_((Address blockPtr));

#endif /* mips */

#else /* KERNEL */

_EXTERN _VoidPtr	malloc _ARGS_((unsigned int bytesNeeded));
_EXTERN int	free _ARGS_((_VoidPtr blockPtr));

#endif /* KERNEL */

_EXTERN _VoidPtr	alloca _ARGS_((int size));
_EXTERN _VoidPtr calloc _ARGS_((unsigned int numElems, unsigned int elemSize));
_EXTERN _VoidPtr	realloc _ARGS_((_VoidPtr ptr, unsigned int newSize));
_EXTERN void	Mem_Bin _ARGS_((int numBytes));
_EXTERN char *	Mem_CallerPC();
_EXTERN void	Mem_DumpTrace _ARGS_((int blockSize));
_EXTERN void	Mem_PrintConfig _ARGS_((void));
_EXTERN void	Mem_PrintInUse _ARGS_((void));
_EXTERN void	Mem_PrintStats _ARGS_((void));
_EXTERN void	Mem_PrintStatsInt _ARGS_((void));
/* 
 * The "proc" argument to Mem_SetPrintProc is a varargs function, 
 * so we have delayed declaring the correct prototype for it.
 */
_EXTERN void	Mem_SetPrintProc _ARGS_((void (*proc)(), ClientData data));
_EXTERN int	Mem_Size _ARGS_((Address blockPtr));

/*
 * Structure used to set up memory allocation traces.
 */

typedef struct {
    int		size;	/* Size of block to trace. */
    int		flags;	/* Flags defined below */
} Mem_TraceInfo;

_EXTERN void	Mem_SetTraceSizes _ARGS_((int numSizes,
					  Mem_TraceInfo *arrayPtr));
/*
 * Flags to determine what type of tracing to do.
 *
 *	MEM_PRINT_TRACE		A trace record will be printed each time that
 *				an object of this size is alloc'd or freed.
 *	MEM_STORE_TRACE		The number of blocks in use by each caller
 *				up to a predefined maximum number of callers
 *				is kept in a trace array .
 *	MEM_DONT_USE_ORIG_SIZE	Don't use the original size for tracing, but use
 *				the modified size used by malloc.
 *	MEM_TRACE_NOT_INIT	The trace records stored for MEM_STORE_TRACE
 *				have not been initialized yet.
 */

#define	MEM_PRINT_TRACE		0x1
#define	MEM_STORE_TRACE		0x2
#define	MEM_DONT_USE_ORIG_SIZE	0x4
#define	MEM_TRACE_NOT_INIT	0x8

extern int	mem_SmallMinNum;
extern int	mem_LargeMinNum;
extern int	mem_LargeMaxSize;

/*
 * Statistics counters;  only incremented when tracing is enabled.
 */

extern int	mem_NumAllocs;
extern int	mem_NumFrees;

/*
 *----------------------------------------------------------------
 * Additional integer math routines, plus structures for returning
 * results from them:
 *----------------------------------------------------------------
 */

typedef struct div_t {
    int quot;
    int rem;
} div_t;

typedef struct {
    long int quot;
    long int rem;
} ldiv_t;

_EXTERN int	abs _ARGS_((int j));
_EXTERN div_t	div _ARGS_((int numer, int denom));
_EXTERN long int labs _ARGS_((long j));
_EXTERN ldiv_t	ldiv _ARGS_((long int numer, long int denom));

/*
 *-----------------------------------
 * Miscellaneous additional routines:
 *-----------------------------------
 */

_EXTERN void	abort _ARGS_((void));
_EXTERN int	atexit _ARGS_((void (*func)(void)));
_EXTERN _VoidPtr bsearch _ARGS_((_CONST _VoidPtr key, _CONST _VoidPtr base,
        size_t n, size_t size,
	int (*cmp)(_CONST _VoidPtr searchKey, _CONST _VoidPtr tableEntry)));
_EXTERN int	exit _ARGS_((int status));
_EXTERN char *	getenv _ARGS_((char *name));
_EXTERN void	qsort _ARGS_((_VoidPtr base, int n, int size,
	  int (*compar)(_CONST _VoidPtr element1, _CONST _VoidPtr element2)));
_EXTERN int	rand _ARGS_((void));
_EXTERN long	random _ARGS_((void));
_EXTERN void	setenv _ARGS_((_CONST char *name, _CONST char *value));
_EXTERN int	srand _ARGS_((int seed));
_EXTERN int	srandom _ARGS_((int seed));
_EXTERN int	system _ARGS_((_CONST char *command));

#endif /* _STDLIB */
