#
# LFS local.mk file.  
#

#
# Turn off optimization until we work out the bugs.
#
NOOPTIMIZATION=true

CFLAGS += -DLFS_STATS_COLLECT -DERROR_CHECK -DVERIFY_CLEAN
#include <$(SYSMAKEFILE)>
