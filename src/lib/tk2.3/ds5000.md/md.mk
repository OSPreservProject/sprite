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
# Wed Dec  2 22:26:43 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/main.c ds3100.md/tk3d.c ds3100.md/tkArgv.c ds3100.md/tkAtom.c ds3100.md/tkBind.c ds3100.md/tkBitmap.c ds3100.md/tkButton.c ds3100.md/tkCanvArc.c ds3100.md/tkCanvBmap.c ds3100.md/tkCanvLine.c ds3100.md/tkCanvPoly.c ds3100.md/tkCanvText.c ds3100.md/tkCanvWind.c ds3100.md/tkCanvas.c ds3100.md/tkCmds.c ds3100.md/tkColor.c ds3100.md/tkConfig.c ds3100.md/tkCursor.c ds3100.md/tkEntry.c ds3100.md/tkError.c ds3100.md/tkEvent.c ds3100.md/tkFont.c ds3100.md/tkFrame.c ds3100.md/tkGC.c ds3100.md/tkGeometry.c ds3100.md/tkGet.c ds3100.md/tkGrab.c ds3100.md/tkListbox.c ds3100.md/tkMenu.c ds3100.md/tkMenubutton.c ds3100.md/tkMessage.c ds3100.md/tkOption.c ds3100.md/tkPack.c ds3100.md/tkPlace.c ds3100.md/tkPreserve.c ds3100.md/tkRectOval.c ds3100.md/tkScale.c ds3100.md/tkScrollbar.c ds3100.md/tkSelect.c ds3100.md/tkSend.c ds3100.md/tkShare.c ds3100.md/tkText.c ds3100.md/tkTextBTree.c ds3100.md/tkTextDisp.c ds3100.md/tkTextIndex.c ds3100.md/tkTextTag.c ds3100.md/tkTrig.c ds3100.md/tkWindow.c ds3100.md/tkWm.c
HDRS		= ds3100.md/default.h ds3100.md/ks_names.h ds3100.md/tk.h ds3100.md/tkCanvas.h ds3100.md/tkConfig.h ds3100.md/tkInt.h ds3100.md/tkText.h
MDPUBHDRS	= 
OBJS		= ds3100.md/main.o ds3100.md/tk3d.o ds3100.md/tkArgv.o ds3100.md/tkAtom.o ds3100.md/tkBind.o ds3100.md/tkBitmap.o ds3100.md/tkButton.o ds3100.md/tkCanvArc.o ds3100.md/tkCanvBmap.o ds3100.md/tkCanvLine.o ds3100.md/tkCanvPoly.o ds3100.md/tkCanvText.o ds3100.md/tkCanvWind.o ds3100.md/tkCanvas.o ds3100.md/tkCmds.o ds3100.md/tkColor.o ds3100.md/tkConfig.o ds3100.md/tkCursor.o ds3100.md/tkEntry.o ds3100.md/tkError.o ds3100.md/tkEvent.o ds3100.md/tkFont.o ds3100.md/tkFrame.o ds3100.md/tkGC.o ds3100.md/tkGeometry.o ds3100.md/tkGet.o ds3100.md/tkGrab.o ds3100.md/tkListbox.o ds3100.md/tkMenu.o ds3100.md/tkMenubutton.o ds3100.md/tkMessage.o ds3100.md/tkOption.o ds3100.md/tkPack.o ds3100.md/tkPlace.o ds3100.md/tkPreserve.o ds3100.md/tkRectOval.o ds3100.md/tkScale.o ds3100.md/tkScrollbar.o ds3100.md/tkSelect.o ds3100.md/tkSend.o ds3100.md/tkShare.o ds3100.md/tkText.o ds3100.md/tkTextBTree.o ds3100.md/tkTextDisp.o ds3100.md/tkTextIndex.o ds3100.md/tkTextTag.o ds3100.md/tkTrig.o ds3100.md/tkWindow.o ds3100.md/tkWm.o
CLEANOBJS	= ds3100.md/main.o ds3100.md/tk3d.o ds3100.md/tkArgv.o ds3100.md/tkAtom.o ds3100.md/tkBind.o ds3100.md/tkBitmap.o ds3100.md/tkButton.o ds3100.md/tkCanvArc.o ds3100.md/tkCanvBmap.o ds3100.md/tkCanvLine.o ds3100.md/tkCanvPoly.o ds3100.md/tkCanvText.o ds3100.md/tkCanvWind.o ds3100.md/tkCanvas.o ds3100.md/tkCmds.o ds3100.md/tkColor.o ds3100.md/tkConfig.o ds3100.md/tkCursor.o ds3100.md/tkEntry.o ds3100.md/tkError.o ds3100.md/tkEvent.o ds3100.md/tkFont.o ds3100.md/tkFrame.o ds3100.md/tkGC.o ds3100.md/tkGeometry.o ds3100.md/tkGet.o ds3100.md/tkGrab.o ds3100.md/tkListbox.o ds3100.md/tkMenu.o ds3100.md/tkMenubutton.o ds3100.md/tkMessage.o ds3100.md/tkOption.o ds3100.md/tkPack.o ds3100.md/tkPlace.o ds3100.md/tkPreserve.o ds3100.md/tkRectOval.o ds3100.md/tkScale.o ds3100.md/tkScrollbar.o ds3100.md/tkSelect.o ds3100.md/tkSend.o ds3100.md/tkShare.o ds3100.md/tkText.o ds3100.md/tkTextBTree.o ds3100.md/tkTextDisp.o ds3100.md/tkTextIndex.o ds3100.md/tkTextTag.o ds3100.md/tkTrig.o ds3100.md/tkWindow.o ds3100.md/tkWm.o
INSTFILES	= Makefile
SACREDOBJS	= 
