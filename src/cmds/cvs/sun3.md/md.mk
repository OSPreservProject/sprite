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
# Thu Dec 17 16:55:50 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= add.c build_entry.c checkin.c checkout.c collect_sets.c commit.c create_admin.c diff.c entries_file.c find_names.c join.c locate_rcs.c log.c main.c maketime.c modules.c name_repository.c no_difference.c options.c partime.c patch.c register.c remove.c scratch_entry.c set_lock.c status.c subr.c tag.c update.c version_number.c version_ts.c info.c
HDRS		= cvs.h patchlevel.h
MDPUBHDRS	= 
OBJS		= sun3.md/add.o sun3.md/build_entry.o sun3.md/checkin.o sun3.md/checkout.o sun3.md/collect_sets.o sun3.md/commit.o sun3.md/create_admin.o sun3.md/diff.o sun3.md/entries_file.o sun3.md/find_names.o sun3.md/join.o sun3.md/locate_rcs.o sun3.md/log.o sun3.md/main.o sun3.md/maketime.o sun3.md/modules.o sun3.md/name_repository.o sun3.md/no_difference.o sun3.md/options.o sun3.md/partime.o sun3.md/patch.o sun3.md/register.o sun3.md/remove.o sun3.md/scratch_entry.o sun3.md/set_lock.o sun3.md/status.o sun3.md/subr.o sun3.md/tag.o sun3.md/update.o sun3.md/version_number.o sun3.md/version_ts.o sun3.md/info.o
CLEANOBJS	= sun3.md/add.o sun3.md/build_entry.o sun3.md/checkin.o sun3.md/checkout.o sun3.md/collect_sets.o sun3.md/commit.o sun3.md/create_admin.o sun3.md/diff.o sun3.md/entries_file.o sun3.md/find_names.o sun3.md/join.o sun3.md/locate_rcs.o sun3.md/log.o sun3.md/main.o sun3.md/maketime.o sun3.md/modules.o sun3.md/name_repository.o sun3.md/no_difference.o sun3.md/options.o sun3.md/partime.o sun3.md/patch.o sun3.md/register.o sun3.md/remove.o sun3.md/scratch_entry.o sun3.md/set_lock.o sun3.md/status.o sun3.md/subr.o sun3.md/tag.o sun3.md/update.o sun3.md/version_number.o sun3.md/version_ts.o sun3.md/info.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile tags
SACREDOBJS	= 
