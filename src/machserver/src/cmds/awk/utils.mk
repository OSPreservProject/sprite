#
# This Makefile is a special one for the awk program.  Its purpose is
# to generate an executable copy of the "proc" utility program, which
# is then used to generate one of the awk source files.  This Makefile
# must be separate from the main Makefile, and must be processed in
# a separate invocation of Pmake, because its target machine must be
# the machine on which we're executing now, not the machine on which
# the awk program is to execute.
#

HDRS		=
INSTALLDIR	=
INSTALLMAN	=
LIBS		=
MACHINES	= $(TM)
MAKEFILE	= utils.mk
MANPAGES	=
NAME		= proc
OBJS		= $(TM).md/proc.o $(TM).md/token.o
SRCS		= proc.c token.c
TM		?= sun3

#include <command.mk>

#if exists($(TM).md/dependencies.mk)
#include	"$(TM).md/dependencies.mk"
#endif
