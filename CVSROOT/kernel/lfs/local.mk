#
# LFS local.mk file.  
#

CFLAGS += -DLFS_STATS_COLLECT -DERROR_CHECK 
#include <$(SYSMAKEFILE)>
.PATH.h: /sprite/src/kernel/mgbaker/hdrs
