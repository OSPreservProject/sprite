#ifndef FONTOS_SEEN
#define	FONTOS_SEEN 1

#include <X11/Xmd.h>

/* normally in stdio.h */
#ifndef BUFSIZ
#define	BUFSIZ	1024
#endif
#ifndef NULL
#define	NULL	0
#endif
#ifndef NUL
#define	NUL '\0'
#endif

/* only include this if we haven't already pulled these definitions from */
/* server/include/os.h */
#ifndef OS_H
#define MAXSHORT 32767
#define MINSHORT -MAXSHORT 
#define	TRUE	1
#define	FALSE	0
typedef	int	Bool;
typedef	unsigned long int	ATOM;
#endif /* OS_H */

/* only define lowbit if we haven't already pulled it from 
   server/include/misc.h */
#ifndef MISC_H
/* 5/5/89 (ef) -- doesn't belong here.  should be fontInternal.h */
/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#define lowbit(x) ((x) & (~(x) + 1))
typedef unsigned char	*pointer;
#endif /* MISC_H */

#ifdef SYSV
#include <memory.h>
#define bzero(b,length) memset(b,0,length)
/* these are not strictly equivalent, but suffice for uses here */
#define bcopy(b1,b2,length) memcpy(b2,b1,length)
#endif /* SYSV */

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

struct _BuildParams	*fosNaturalParams;

extern	pointer	fosTmpAlloc();
extern	void	fosTmpFree();
extern	pointer	fosAlloc();
extern	pointer	fosCalloc();
extern	pointer	fosRealloc();
extern	void	fosFree();

extern	void	fosWarning();
extern	void	fosError();
extern	void	fosFatalError();
extern	void	fosInternalError();

#ifdef SANITY_CHECKS
#define	ASSERT(f,e)	\
if (!(e)) {\
  fosInternalError("assertion botched in %s: e \n",(f)); exit(1);\
}
#else
#define	ASSERT(f,e)
#endif /* SANITY_CHECKS */

#endif /* FONTOS_SEEN */
