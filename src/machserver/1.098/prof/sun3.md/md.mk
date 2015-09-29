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
# Tue Jul  2 14:01:05 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/_mcount.c sun3.md/profStack.s sun3.md/profSubr.c profMigrate.c profProfil.c
HDRS		= sun3.md/profInt.h sun3.md/timer.h prof.h
MDPUBHDRS	= 
OBJS		= sun3.md/_mcount.o sun3.md/profMigrate.o sun3.md/profProfil.o sun3.md/profStack.o sun3.md/profSubr.o
CLEANOBJS	= sun3.md/_mcount.o sun3.md/profStack.o sun3.md/profSubr.o sun3.md/profMigrate.o sun3.md/profProfil.o
INSTFILES	= sun3.md/md.mk sun3.md/md.mk.sed sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 

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
