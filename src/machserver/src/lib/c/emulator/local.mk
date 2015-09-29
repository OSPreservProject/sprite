#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#if empty(TM:Mds3100) && empty(TM:Mds5000)
CFLAGS		+= -Wall
#endif

SPRITEDINCLUDE	?= /users/kupfer/spriteserver/include

# Define the search path for Sprite include files.

APPLINCLUDE	= $(TM).md . $(SPRITEDINCLUDE)/user/$(TM).md \
  $(SPRITEDINCLUDE)/user $(SPRITEDINCLUDE)/$(TM).md $(SPRITEDINCLUDE)
  

# Get all that good Mach support stuff...
#include "/users/kupfer/lib/pmake/mach.mk"
