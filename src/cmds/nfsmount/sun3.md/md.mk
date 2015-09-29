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
# Wed Jun 10 17:43:14 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= mount.c nfsAttr.c nfsIO.c nfs_prot_xdr.c nfsLookup.c mount_xdr.c nfsName.c nfs_prot_clnt.c mount_clnt.c main.c nfsStatusMap.c pdev.c pfs.c
HDRS		= mount.h nfs.h nfs_prot.h voiddef.h
MDPUBHDRS	= 
OBJS		= sun3.md/main.o sun3.md/mount.o sun3.md/mount_clnt.o sun3.md/mount_xdr.o sun3.md/nfsAttr.o sun3.md/nfsIO.o sun3.md/nfsLookup.o sun3.md/nfsName.o sun3.md/nfsStatusMap.o sun3.md/nfs_prot_clnt.o sun3.md/nfs_prot_xdr.o sun3.md/pdev.o sun3.md/pfs.o
CLEANOBJS	= sun3.md/mount.o sun3.md/nfsAttr.o sun3.md/nfsIO.o sun3.md/nfs_prot_xdr.o sun3.md/nfsLookup.o sun3.md/mount_xdr.o sun3.md/nfsName.o sun3.md/nfs_prot_clnt.o sun3.md/mount_clnt.o sun3.md/main.o sun3.md/nfsStatusMap.o sun3.md/pdev.o sun3.md/pfs.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
