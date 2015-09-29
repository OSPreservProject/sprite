
#if   !empty(TM:Mds3100)
CFLAGS	+=  -DHOST_SYS=DEC3100_SYS 
#elif !empty(TM:Msun4)
CFLAGS	+=  -DDEFAULT_VECTOR=sunos_big_vec -DHOST_SYS=SUN4_SYS 
#endif

#ifmake depend
#	makedepend can't handle the if M68 || I386 || MIPS stuff in some
#	of the files like coffcode.h.  It tries to include a file 
#	named "Your aouthdr magic number is not being set" because it
#	thinks that __A_MAGIC_SET is not set.  We correct this problem
#	by defining it on the command line if we are running makedepend.
CFLAGS	+= -D__A_MAGIC_SET__
#endif 

#include	<$(SYSMAKEFILE)>

.PATH.h		: ../gnu_include
