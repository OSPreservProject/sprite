#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		+= -lmach

# At least for the time being, use the same compilation environment as
# the Sprite server.  It probably makes more sense to use the server's
# environment than the Sprite "user" environment, because (a) the
# emulation code is closely tied to the server and (b) the emulation
# code needs to know about Mach.
#include "/users/kupfer/lib/pmake/sprited.mk"

