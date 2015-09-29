LIBS	+= /sprite/src/admin/lfslib/installed/${TM}.md/liblfslib.a

#include	<$(SYSMAKEFILE)>

#if empty(TM:Mds3100)
CFLAGS +=  -Wall 
#endif


.PATH.h		:  /sprite/src/admin/lfslib/installed

