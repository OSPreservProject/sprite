#
# Makefile of rules for creating dependencies using the 'makedepend'
# program. DEPFILE contains the name of the file wherein the dependencies
# should be placed. This defaults to 'dependencies.mk' in the local directory.
# It may, however, be changed to 'Makefile' without harm.
#
# If you want the dependencies to include the full pathname of each include
# file, you must specify the '-p' flag in the DEPFLAGS variable.
#
# If the SRCS variable is defined, the 'depend' target will be set up
# to create dependencies for all files in that variable.
#
# The MAKEDEPEND rule will pass all -I and -D flags given in the CFLAGS
# variable to makedepend. Usage is like this:
#
# depend : $(LIBSRCS) $(PROGSRC) MAKEDEPEND
# 
DEPFLAGS	?=

#if defined(SRCS) && !defined(NODEPEND)
depend		: $(SRCS) MAKEDEPEND .NOTMAIN
#endif

DEPFILE		?= dependencies.mk

MAKEDEPEND	: .USE
	makedepend $(CFLAGS:M-[ID]*) $(DEPFLAGS) -f $(DEPFILE) $(.ALLSRC)
