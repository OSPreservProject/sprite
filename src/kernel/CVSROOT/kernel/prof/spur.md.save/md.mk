#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Thu Aug 23 13:58:52 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/_mcount.c spur.md/profStack.s spur.md/profSubr.c profMigrate.c profProfil.c
HDRS		= spur.md/profInt.h prof.h
MDPUBHDRS	= 
OBJS		= spur.md/_mcount.o spur.md/profStack.o spur.md/profSubr.o spur.md/profMigrate.o spur.md/profProfil.o
CLEANOBJS	= spur.md/_mcount.o spur.md/profStack.o spur.md/profSubr.o spur.md/profMigrate.o spur.md/profProfil.o
INSTFILES	= spur.md/md.mk spur.md/md.mk.sed spur.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 

#
# Limit extract calls by expanding shifts inline.
XCFLAGS += -mexpand-shifts
#
# Special massaging of _mcount.c needed to create non-profiled .po file
# and .o file with _mcount renamed to the mcount everyone expects
#
$(TM).md/_mcount.po	: $(TM).md/_mcount.o .NOTMAIN
	$(CP) $(TM).md/_mcount.o $(TM).md/_mcount.po
$(TM).md/_mcount.o	: .NOTMAIN
	$(CC) $(CFLAGS) -S $(.IMPSRC)
	$(SED) -e s/_mcount/mcount/g < _mcount.s > $(TM).md/_mcount.s
	$(AS) $(AFLAGS) -o $(.TARGET) $(TM).md/_mcount.s
	$(RM) -f _mcount.s $(TM).md/_mcount.s
