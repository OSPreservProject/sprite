#
# LFS local.mk file.  
#

#
# Turn off optimization until we work out the bugs.
#
NOOPTIMIZATION=true

CFLAGS += -DLFS_STATS_COLLECT -DERROR_CHECK -DVERIFY_CLEAN
#include <$(SYSMAKEFILE)>

CTAGS = true	# i.e. just return success, don't do anything

tags:: $(ALLCSRCS) $(HDRS)
	/sprite/cmds/ctags -f tags.here $(CTFLAGS) $(ALLCSRCS) $(HDRS)
	/sprite/src/kernel/fs/fstags
