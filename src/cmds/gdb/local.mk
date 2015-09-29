#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME		= gdb
LIBS		+= -ltermlib

NOOPTIMIZATION=true

#include	<$(SYSMAKEFILE)>

.PATH.h		: sprite . dist dist/readline

CFLAGS          += -Isprite -I. -Idist -Idist/readline 


DISTFILES   =   dist sprite

