#
# Mach Operating System
# Copyright (c) 1991,1990 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie the
# rights to redistribute these changes.
#
#
# HISTORY
# $Log:	Makefile,v $
# Revision 2.6  91/08/24  12:29:43  af
# 	Fine tuning: ... -if vs if
# 	[91/08/14            rvb]
# 	Fixed release to actually link each time.
# 	[91/07/23            danner]
# 
# Revision 2.5.3.1  91/08/19  13:49:07  danner
# 	Fine tuning: ... -if vs if
# 	[91/08/14            rvb]
# 	Fixed release to actually link each time.
# 	[91/07/23            danner]
# 
# Revision 2.5  91/02/05  17:54:02  mrt
# 	Changed to new Mach copyright
# 	[91/02/01  17:53:29  mrt]
# 
# Revision 2.4  90/12/14  11:02:05  jsb
# 	Use $(atsys) (as defined in Makeconf) instead of @sys.
# 	[90/12/13  21:42:30  jsb]
# 
# Revision 2.3  90/11/05  14:32:28  rpd
# 	SUN->SUN3.
# 	[90/10/31            rwd]
# 	Change MACHINE to TARGET_MACHINE.
# 	[90/10/29            rwd]
# 
# Revision 2.2  90/06/02  15:04:16  rpd
# 	Don't use /usr/cs/etc/install; 386 doesn't have it.
# 	[90/06/01            rpd]
# 
# 	Created for new IPC.
# 	[90/03/26  21:09:47  rpd]
# 
#

INSTALL	=	/usr/cs/etc/install -q -c

all:		mig.sh migcom.x

install:	mig.install

mig.install:	$(DDIR)/mig

$(DDIR)/mig:	mig.sh
	cp -p mig.sh $(DDIR)/mig
	chmod 755 $(DDIR)/mig

migcom.install:	$(DDIR)/migcom

$(DDIR)/migcom:	migcom.x
	cp -p migcom.x $(DDIR)/migcom
	strip $(DDIR)/migcom
	chmod 755 $(DDIR)/migcom


OBJS	 =	mig.o parser.o lexxer.o error.o string.o type.o routine.o \
		statement.o global.o header.o user.o server.o utils.o

# Base of source tree.  Look here for include files.
IDIR = ../..

PMAXCFLAGS = -Dvoid=int
SUN3CFLAGS = -Dvoid=int
CFLAGS= $(CDEBUGFLAGS) $($(TARGET_MACHINE)CFLAGS) -I$(IDIR)

migcom.x:	$(OBJS)
	$(CC) -o migcom.x $(CFLAGS) $(OBJS) -ll

.c.o:
	$(CC) -c -MD $(CFLAGS) $*.c
	md -u Makedep -d $*.d

parser.c parser.h:	parser.y
	yacc -d parser.y &&		\
	mv y.tab.c parser.c &&		\
	mv y.tab.h parser.h

lexxer.c:	lexxer.l
	lex lexxer.l &&			\
	mv lex.yy.c lexxer.c

LIBBASE = $(MAKECONF:h)/../../../release/$(atsys)/${BCSBBASE?$(BCSBBASE:t):latest}/lib
BINBASE = $(MAKECONF:h)/../../../release/$(atsys)/${BCSBBASE?$(BCSBBASE:t):latest}/bin
OBJBASE = $(MAKECONF:h)/../../../obj/$(atsys)/${BCSBBASE?$(BCSBBASE:t):latest}/kernel

release:
	-if [ -f $(LIBBASE)/migcom ]; then mv $(LIBBASE)/migcom $(LIBBASE)/migcom.bak; else true ; fi
	ln -s $(OBJBASE)/lib/migcom $(LIBBASE) 
	-if [ -f $(LIBBASE)/migcom.bak ]; then rm -f $(LIBBASE)/migcom.bak; else true; fi
	-if [ -f $(BINBASE)/mig ]; then mv $(BINBASE)/mig $(BINBASE)/mig.bak; else true ; fi
	ln -s $(OBJBASE)/bin/mig $(BINBASE) 
	-if [ -f $(BINBASE)/mig.bak ]; then rm -f $(BINBASE)/mig.bak; else true; fi


-include Makedep
