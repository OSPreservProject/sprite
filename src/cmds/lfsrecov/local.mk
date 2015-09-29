LIBS	+= /sprite/src/admin/lfslib/${TM}.md/liblfslib.a

#if !empty(TM:Mds3100)
NOOPTIMIZATION	= no -O please
#endif 

#include	<$(SYSMAKEFILE)>


#if empty(TM:Mds3100)
CFLAGS +=  -Wall 
#endif


.PATH.h		:  /sprite/src/admin/lfslib

