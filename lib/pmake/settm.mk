#
# This Makefile is included by the user's Makefile.  It sets the
# target machine type (TM), though the user can override it in the
# "make" command.  This operation is done here so that we can add
# backward-compatible machine types (e.g., ds5000) and have users
# easily treat the new machine type the same as the old one.
#
# $Header: /sprite/lib/pmake/RCS/settm.mk,v 1.3 92/06/10 13:05:28 jhh Exp $
# 

# If there is only one allowable machine type, that's the default
# target type.  Otherwise the default is the type we're running on
# now.

NUM_MACHINES   != echo $(MACHINES) | wc -w

#if $(NUM_MACHINES) == 1
TM	       ?= $(MACHINES)
#else
TM	       ?= $(MACHINE)
#endif

# Map ds5000 to ds3100 (only if we are not in a kernel module as determined
# by having a type of kernel). Also, if the type isn't defined then assume
# we are not in a kernel module. This is to be backwards compatible with
# older Makefiles..

#if !empty(TM:Mds5000) 
#if !defined(TYPE) || empty(TYPE:Mkernel)
TM		= ds3100
#endif
#endif
