LIBS	+= /sprite/src/admin/lfslib/installed/${TM}.md/liblfslib.a

#include	<$(SYSMAKEFILE)>

#if empty(TM:Mds3100)
CFLAGS +=  -Wall 
#endif


.PATH.h		:  /sprite/src/admin/lfslib/installed

statprint.h: /sprite/src/kernel/Include/lfsStats.h mkstatprint.awk
	nawk -f mkstatprint.awk  /sprite/src/kernel/Include/lfsStats.h > statprint.h

