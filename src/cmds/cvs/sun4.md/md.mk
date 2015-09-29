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
# Thu Dec 17 16:55:58 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= add.c build_entry.c checkin.c checkout.c collect_sets.c commit.c create_admin.c diff.c entries_file.c find_names.c join.c locate_rcs.c log.c main.c maketime.c modules.c name_repository.c no_difference.c options.c partime.c patch.c register.c remove.c scratch_entry.c set_lock.c status.c subr.c tag.c update.c version_number.c version_ts.c info.c
HDRS		= cvs.h patchlevel.h
MDPUBHDRS	= 
OBJS		= sun4.md/add.o sun4.md/build_entry.o sun4.md/checkin.o sun4.md/checkout.o sun4.md/collect_sets.o sun4.md/commit.o sun4.md/create_admin.o sun4.md/diff.o sun4.md/entries_file.o sun4.md/find_names.o sun4.md/info.o sun4.md/join.o sun4.md/locate_rcs.o sun4.md/log.o sun4.md/main.o sun4.md/maketime.o sun4.md/modules.o sun4.md/name_repository.o sun4.md/no_difference.o sun4.md/options.o sun4.md/partime.o sun4.md/patch.o sun4.md/register.o sun4.md/remove.o sun4.md/scratch_entry.o sun4.md/set_lock.o sun4.md/status.o sun4.md/subr.o sun4.md/tag.o sun4.md/update.o sun4.md/version_number.o sun4.md/version_ts.o
CLEANOBJS	= sun4.md/add.o sun4.md/build_entry.o sun4.md/checkin.o sun4.md/checkout.o sun4.md/collect_sets.o sun4.md/commit.o sun4.md/create_admin.o sun4.md/diff.o sun4.md/entries_file.o sun4.md/find_names.o sun4.md/join.o sun4.md/locate_rcs.o sun4.md/log.o sun4.md/main.o sun4.md/maketime.o sun4.md/modules.o sun4.md/name_repository.o sun4.md/no_difference.o sun4.md/options.o sun4.md/partime.o sun4.md/patch.o sun4.md/register.o sun4.md/remove.o sun4.md/scratch_entry.o sun4.md/set_lock.o sun4.md/status.o sun4.md/subr.o sun4.md/tag.o sun4.md/update.o sun4.md/version_number.o sun4.md/version_ts.o sun4.md/info.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile tags
SACREDOBJS	= 
