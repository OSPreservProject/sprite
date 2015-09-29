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
# Thu Dec 17 16:55:41 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= add.c build_entry.c checkin.c checkout.c collect_sets.c commit.c create_admin.c diff.c entries_file.c find_names.c join.c locate_rcs.c log.c main.c maketime.c modules.c name_repository.c no_difference.c options.c partime.c patch.c register.c remove.c scratch_entry.c set_lock.c status.c subr.c tag.c update.c version_number.c version_ts.c info.c
HDRS		= cvs.h patchlevel.h
MDPUBHDRS	= 
OBJS		= ds3100.md/add.o ds3100.md/build_entry.o ds3100.md/checkin.o ds3100.md/checkout.o ds3100.md/collect_sets.o ds3100.md/commit.o ds3100.md/create_admin.o ds3100.md/diff.o ds3100.md/entries_file.o ds3100.md/find_names.o ds3100.md/info.o ds3100.md/join.o ds3100.md/locate_rcs.o ds3100.md/log.o ds3100.md/main.o ds3100.md/maketime.o ds3100.md/modules.o ds3100.md/name_repository.o ds3100.md/no_difference.o ds3100.md/options.o ds3100.md/partime.o ds3100.md/patch.o ds3100.md/register.o ds3100.md/remove.o ds3100.md/scratch_entry.o ds3100.md/set_lock.o ds3100.md/status.o ds3100.md/subr.o ds3100.md/tag.o ds3100.md/update.o ds3100.md/version_number.o ds3100.md/version_ts.o
CLEANOBJS	= ds3100.md/add.o ds3100.md/build_entry.o ds3100.md/checkin.o ds3100.md/checkout.o ds3100.md/collect_sets.o ds3100.md/commit.o ds3100.md/create_admin.o ds3100.md/diff.o ds3100.md/entries_file.o ds3100.md/find_names.o ds3100.md/join.o ds3100.md/locate_rcs.o ds3100.md/log.o ds3100.md/main.o ds3100.md/maketime.o ds3100.md/modules.o ds3100.md/name_repository.o ds3100.md/no_difference.o ds3100.md/options.o ds3100.md/partime.o ds3100.md/patch.o ds3100.md/register.o ds3100.md/remove.o ds3100.md/scratch_entry.o ds3100.md/set_lock.o ds3100.md/status.o ds3100.md/subr.o ds3100.md/tag.o ds3100.md/update.o ds3100.md/version_number.o ds3100.md/version_ts.o ds3100.md/info.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags
SACREDOBJS	= 
