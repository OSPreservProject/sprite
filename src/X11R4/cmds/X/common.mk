#
# This file is linked to symbolically by the "local.mk" files of most
# of the subdirectories.
#
# $Source: /X11/R4/src/cmds/X/RCS/common.mk,v $
# $Date: 91/01/09 15:18:00 $
# $Revision: 1.3 $ $Author: kupfer $

X	= /X11/R4

INSERVER=
#include <$(X)/root.mk>

#
# Extra flags to define or undefine for compilations:
#
# TCPCONN	includes driver for network connections via TCP
# UNIXCONN	includes driver for network connections via unix domain
# DNETCONN	includes driver for network connections via DECnet
# SPRITEPDEVCONN include driver for sprite pdev connections.
# NOLOGOHACK	is defined because the sprite OS layer doesn't support it,
#		whatever the logo hack may be.
# SHAPE		the MIT shape extension.  See root.mk.
#
CFLAGS		+= -DNOLOGOHACK
CFLAGS		+= -DTCPCONN
CFLAGS		+= -DSPRITEPDEVCONN
CFLAGS		+= -DSHAPE

# Font/color support
CFLAGS		+= $(FONTDEFINES)
CFLAGS		+= $(DEFFONTPATH) $(DEFRGB_DB)

# More include files
.PATH.h		:  $(SERVINCDIR) $(INCDIR)/X11
