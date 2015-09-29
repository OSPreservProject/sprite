#
# Makefile  --
#
# Makefile for Extended Tcl C sources.  This will compile all of Extended Tcl
# and add it to the libtcl.a in the parent directory.  Generates a Tcl shell
# in the parent directory.
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
# $Id: Makefile,v 2.0 1992/10/16 04:51:22 markd Rel $
#------------------------------------------------------------------------------
#

include ../Config.mk
include ../config/$(TCL_CONFIG_FILE)
SHELL=/bin/sh

#------------------------------------------------------------------------------

CFLAGS= $(OPTIMIZE_FLAG) $(XCFLAGS) -I../$(TCL_UCB_DIR) $(MEM_DEBUG_FLAGS) \
        $(SYS_DEP_FLAGS)

#------------------------------------------------------------------------------

OBJS= \
    main.o           tclXbsearch.o    tclXchmod.o      tclXclock.o     \
    tclXcmdloop.o    tclXcnvclock.o   tclXcreate.o     tclXdebug.o     \
    tclXdup.o        tclXfcntl.o      tclXfilecmds.o   tclXfilescan.o  \
    tclXfmath.o      tclXgeneral.o    tclXhandles.o    tclXid.o        \
    tclXlib.o        tclXlist.o       tclXmath.o       tclXmatherr.o   \
    tclXmsgcat.o     tclXprocess.o    tclXprofile.o    tclXregexp.o    \
    tclXselect.o     tclXsignal.o     tclXstartup.o    tclXstring.o    \
    tclXunixcmds.o   tclXutil.o       tclXgetdate.o    tclXkeylist.o
 
#------------------------------------------------------------------------------

all: made.tmp ../tcl

made.tmp: $(OBJS) $(CPLUSOBJS)
	$(AR) cr ../libtcl.a $(OBJS) $(CPLUSOBJS)
	touch made.tmp

tclXstartup.o: patchlevel.h tclXstartup.c
	$(CC) -c $(CFLAGS) -DTCL_DEFAULT=\"$(TCL_DEFAULT)\" tclXstartup.c

main.o: patchlevel.h main.c
	$(CC) -c $(CFLAGS) $(HISTORY_FLAG) main.c

tclXgetdate.c: tclXgetdate.y
	$(YACC) tclXgetdate.y
	mv y.tab.c tclXgetdate.c

../tcl: ../libtcl.a made.tmp 
	$(RANLIB_CMD) ../libtcl.a
	$(CC) $(CFLAGS) main.o ../libtcl.a $(LIBS) $(XLDFLAGS) -o ../tcl
	if $(DO_STRIPPING) ; then \
	    strip ../tcl; \
	    $(MCS_CMD) ../tcl; fi


#------------------------------------------------------------------------------
# This is a painful situation.  The file tcl++.h requires string.h, which is
# redefined in the UCB Tcl directory.  We force the compiler to search the
# C++ directories first.

tcl++.o: tcl++.C tcl++.h
	$(CCPLUS) -c -I $(CCPLUSINCL) $(CFLAGS) $(HISTORY_FLAG) tcl++.C

#------------------------------------------------------------------------------
# This is just to test if it compiles.

TCL++: ../tcl++

../tcl++: main++.o
	$(CCPLUS) $(CFLAGS) main++.o ../libtcl.a $(LIBS) $(XLDFLAGS) \
	-o ../tcl++

main++.o: tcl++.h main++.C
	$(CCPLUS) -c -I /usr/include/CC $(CFLAGS) $(HISTORY_FLAG) main++.C


#------------------------------------------------------------------------------

clean:
	-rm -f made.tmp tclXgetdate.c
	-rm -f *.o ../tcl ../tcl++

