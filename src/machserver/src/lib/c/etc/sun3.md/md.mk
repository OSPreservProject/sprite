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
# Sun Dec  8 15:19:58 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= status.c Rpc_GetName.c fmt.c isatty.c isinf.c isnan.c iszero.c ldexp.c panic.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/status.o sun3.md/Rpc_GetName.o sun3.md/fmt.o sun3.md/isatty.o sun3.md/isinf.o sun3.md/isnan.o sun3.md/iszero.o sun3.md/ldexp.o sun3.md/panic.o
CLEANOBJS	= sun3.md/status.o sun3.md/Rpc_GetName.o sun3.md/fmt.o sun3.md/isatty.o sun3.md/isinf.o sun3.md/isnan.o sun3.md/iszero.o sun3.md/ldexp.o sun3.md/panic.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
