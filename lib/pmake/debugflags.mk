#
# This Makefile is included by several other system Makefiles.  It
# sets OFLAG and GFLAG, which control optimization and debugging
# support, and are eventually included in CFLAGS.  This Makefile is
# distinct from tm.mk because the comments in tm.mk state that this
# functionality should be separate.
# This Makefile uses the following variables as input:
# NOOPTIMIZATION	If this variable is defined, then optimization
#			is suppressed.
# TM			The machine type being compiled for.
#
# $Header: /sprite/lib/pmake/RCS/debugflags.mk,v 1.3 92/11/27 17:16:46 jhh Exp $
#

#
# OFLAG : optimization flag
# GFLAG : debugging support when optimization is turned on
# GDFLAG : debugging support when optimization is turned off (D for Debug)
# AGFLAG: same as for GFLAG, but for .s files
# AGDFLAG: samse as for GDFLAG, but for .s files

#ifndef NOOPTIMIZATION
OFLAG = -O
#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
#if empty(CC:Mgcc)
GFLAG = -g3
GDFLAG = -g
AGFLAG = $(GFLAG)
AGDFLAG = $(GDFLAG)
#else
# I wish I knew why GFLAG is set empty here. -mdk
GFLAG	=
GDFLAG	=
AGFLAG = $(GFLAG)
AGDFLAG = $(GDFLAG)
#endif /* gcc */
#else /* not compiling for a DECstation */
GFLAG = -g
GDFLAG = -g
AGFLAG =
AGDFLAG = 
#endif

#else /* NOOPTIMIZATION */

OFLAG =
GFLAG = -g
GDFLAG = -g
#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
AGFLAG = $(GFLAG)
AGDFLAG = $(GDFLAG)
#else
AGFLAG =
AGDFLAG = 
#endif
#endif /* NOOPTIMIZATION */
