#
# Makefile --
#
# Makefile for to build Extended wish.
# 
#------------------------------------------------------------------------------
# Copyright 1992 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: Makefile,v 1.1 92/12/03 15:35:56 jhh Exp Locker: jhh $
#------------------------------------------------------------------------------
#

include ../Config.mk
include ../config/$(TCL_CONFIG_FILE)
SHELL=/bin/sh

#------------------------------------------------------------------------------

CFLAGS= $(OPTIMIZE_FLAG) $(XCFLAGS) -I$(TCL_UCB_DIR) $(MEM_DEBUG_FLAGS)

#------------------------------------------------------------------------------

all: ../$(TCL_TK_SHELL)

../$(TCL_TK_SHELL): ../libtk.a ../libtcl.a
	$(RANLIB_CMD) ../libtk.a
	$(CC) $(CFLAGS) ../tkucbsrc/main.o ../libtk.a ../libtcl.a \
            $(XLDFLAGS) $(TCL_TK_LIBS) -o ../$(TCL_TK_SHELL)
	if $(DO_STRIPPING) ; then \
	    strip ../$(TCL_TK_SHELL); \
	    $(MCS_CMD) ../$(TCL_TK_SHELL); fi

#------------------------------------------------------------------------------

clean:
	-rm -f ../$(TCL_TK_SHELL)
