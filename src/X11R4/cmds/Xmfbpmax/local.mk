#
X=/X11/R4

LIBS	+= -lX11 -lm
SUBDIRS	= snf mi mfb dix os Xmfbpmax extensions

# needs fontdir.o in src/cmds/mkfontdir !!!

#include	<$(SYSMAKEFILE)>
