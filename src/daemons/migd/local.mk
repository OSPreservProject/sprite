#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Have mkmf/pmake set up a version.c & version.h to define Version().
#
use_version = yes

#
# SEEK_REOPEN controls whether to do a seek after an freopen due to
# a bug in the sprite C library.
#
# CFLAGS +=  -DSEEK_REOPEN

#
# For debugging a problem with List_Remove
#
CFLAGS += -DDEBUG_LIST_REMOVE

#
# Use a different migd directory, etc.
#
# TESTING=yes, please
#ifdef TESTING
CFLAGS += -DGLOBAL_PDEV=\"/tmp/migd/global.pdev\" \
          -DLOCAL_PDEV=\"/tmp/migd/%s.pdev\" \
          -DMIGD_CHECKPOINT_FILE=\"/tmp/migd/check\" \
          -DMIGD_LOCK_FILE=\"/tmp/migd/lock\" \
	  -DSHARED_ERROR_NAME=\"/tmp/migd/global-log\" \
	  -DLOCAL_ERROR_NAME=\"/tmp/migd/%s.log\"
CFLAGS += -DDEBUG
#endif


# Memory tracing
#CFLAGS +=  -DMEMTRACE
#LIBS += -lmemtrace

#
# Must run set-user-id to root because it signals processes of random users.
#
INSTALLFLAGS	+= -o root -m 4775

# 
# Always use the debuggable C library
#
LIBS += -lc_g

#include	<$(SYSMAKEFILE)>

