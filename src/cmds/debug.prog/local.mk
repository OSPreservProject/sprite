#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# This file is needed so that the program can have a different name than
# its directory.  The reason for this is that the Makefile in .. has a
# target for each of its subdirectories, plus a standard target with the
# same name as this program's name.  Thus if the directory weree named
# after its program, which is the usual case, there would be conflict of
# Makefile targets and weird things would happen during makes.  Instead,
# this directory is given a ".prog" extension, which means that NAME is
# set wrong in Makefile.  To compensate, we reset NAME here.

NAME		:= $(NAME:S|.prog||)

#include	<$(SYSMAKEFILE)>
