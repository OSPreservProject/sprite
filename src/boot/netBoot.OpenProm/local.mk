#if !empty(TM:Msun4c)
INSTALL_PROG	= /sprite/cmds/update -s
CFLAGS		+= -DKERNEL
#endif

#include	<$(SYSMAKEFILE)>

.PATH.h		: /sprite/lib/include/kernel
.PATH.h		: /sprite/lib/include/kernel/$(TM).md
