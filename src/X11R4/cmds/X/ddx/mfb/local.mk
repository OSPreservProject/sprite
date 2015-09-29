#
# special local.mk for ddx/mfb
#
# $Source: /mic/X11R4/src/cmds/X/ddx/mfb/RCS/local.mk,v $
# $Date: 90/02/14 20:13:11 $
# $Revision: 1.1 $ $Author: tve $

#
# Because of the way object files are generated (many from a single source,
# with no object file using the source's name!), it's easier simply to
# override the information set up by Mkmf.
#
OBJS =   $(TM).md/mfbgc.o $(TM).md/mfbwindow.o $(TM).md/mfbfont.o \
	 $(TM).md/mfbfillrct.o $(TM).md/mfbpntwin.o \
	 $(TM).md/maskbits.o $(TM).md/mfbpixmap.o \
	 $(TM).md/mfbimage.o $(TM).md/mfbline.o $(TM).md/mfbbres.o \
	 $(TM).md/mfbhrzvert.o $(TM).md/mfbbresd.o $(TM).md/mfbseg.o \
	 $(TM).md/mfbpushpxl.o $(TM).md/mfbzerarc.o $(TM).md/mfbfillarc.o \
	 $(TM).md/mfbfillsp.o $(TM).md/mfbsetsp.o $(TM).md/mfbscrinit.o \
	 $(TM).md/mfbscrclse.o $(TM).md/mfbclip.o \
         $(TM).md/mfbbitblt.o $(TM).md/mfbgetsp.o $(TM).md/mfbpolypnt.o \
	 $(TM).md/mfbpgbwht.o $(TM).md/mfbpgbblak.o $(TM).md/mfbpgbinv.o \
	 $(TM).md/mfbigbwht.o $(TM).md/mfbigbblak.o $(TM).md/mfbcmap.o \
	 $(TM).md/mfbpawhite.o $(TM).md/mfbpablack.o $(TM).md/mfbpainv.o \
	 $(TM).md/mfbtile.o $(TM).md/mfbtewhite.o $(TM).md/mfbteblack.o \
	 $(TM).md/mfbmisc.o $(TM).md/mfbbstore.o

#include <../../common.mk>

#Additional include file paths
.PATH.h: ../mi

# The folks below need this
RM	= rm -f
LN	= ln

#
# Weird stuff taken (almost) straight out of the Imakefile
# For Sprite, we have to add "$(TM.md/" in front of all .o files and
# we have to add "-o $@" to all $(CC) lines...
#

$(TM).md/mfbseg.o: mfbseg.c $(TM).md/mfbline.o
	$(CC) -DPOLYSEGMENT $(CFLAGS) -c mfbseg.c -o $@

mfbseg.c:
	$(LN) mfbline.c mfbseg.c

$(TM).md/mfbpntarea.o $(TM).md/mfbimggblt.o $(TM).md/mfbplygblt.o $(TM).md/mfbtegblt.o:
	$(RM) $@; touch $@

$(TM).md/mfbpgbwht.o: mfbplygblt.c $(TM).md/mfbplygblt.o
	$(RM) $@ mfbpgbwht.c
	$(LN) mfbplygblt.c mfbpgbwht.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltWhite \
	   -DOPEQ=\|=  $(CFLAGS) -c mfbpgbwht.c -o $@
	$(RM) mfbpgbwht.c

$(TM).md/mfbpgbblak.o: mfbplygblt.c $(TM).md/mfbplygblt.o
	$(RM) $@ mfbpgbblak.c
	$(LN) mfbplygblt.c mfbpgbblak.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltBlack \
	   -DOPEQ=\&=~ $(CFLAGS) -c mfbpgbblak.c -o $@
	$(RM) mfbpgbblak.c

$(TM).md/mfbpgbinv.o: mfbplygblt.c $(TM).md/mfbplygblt.o
	$(RM) $@ mfbpgbinv.c
	$(LN) mfbplygblt.c mfbpgbinv.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltInvert \
	   -DOPEQ=\^=  $(CFLAGS) -c mfbpgbinv.c -o $@
	$(RM) mfbpgbinv.c

$(TM).md/mfbigbwht.o: mfbimggblt.c $(TM).md/mfbimggblt.o
	$(RM) $@ mfbigbwht.c
	$(LN) mfbimggblt.c mfbigbwht.c
	${CC} -DMFBIMAGEGLYPHBLT=mfbImageGlyphBltWhite \
	   -DOPEQ=\|=  $(CFLAGS) -c mfbigbwht.c -o $@
	$(RM) mfbigbwht.c

$(TM).md/mfbigbblak.o: mfbimggblt.c $(TM).md/mfbimggblt.o
	$(RM) $@ mfbigbblak.c
	$(LN) mfbimggblt.c mfbigbblak.c
	${CC} -DMFBIMAGEGLYPHBLT=mfbImageGlyphBltBlack \
	   -DOPEQ=\&=~ $(CFLAGS) -c mfbigbblak.c -o $@
	$(RM) mfbigbblak.c

$(TM).md/mfbpawhite.o: mfbpntarea.c $(TM).md/mfbpntarea.o
	$(RM) $@ mfbpawhite.c
	$(LN) mfbpntarea.c mfbpawhite.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidWhiteArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleWhiteArea \
	   -DOPEQ=\|=  -DEQWHOLEWORD=\=~0 \
	   $(CFLAGS) -c mfbpawhite.c -o $@
	$(RM) mfbpawhite.c

$(TM).md/mfbpablack.o: mfbpntarea.c $(TM).md/mfbpntarea.o
	$(RM) $@ mfbpablack.c
	$(LN) mfbpntarea.c mfbpablack.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidBlackArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleBlackArea \
	   -DOPEQ=\&=~ -DEQWHOLEWORD=\=0 \
	   $(CFLAGS) -c mfbpablack.c -o $@
	$(RM) mfbpablack.c

$(TM).md/mfbpainv.o: mfbpntarea.c $(TM).md/mfbpntarea.o
	$(RM) $@ mfbpainv.c
	$(LN) mfbpntarea.c mfbpainv.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidInvertArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleInvertArea \
	   -DOPEQ=\^=  -DEQWHOLEWORD=\^=~0 \
	   $(CFLAGS) -c mfbpainv.c -o $@
	$(RM) mfbpainv.c

$(TM).md/mfbtewhite.o: mfbtegblt.c $(TM).md/mfbtegblt.o
	$(RM) $@ mfbtewhite.c
	$(LN) mfbtegblt.c mfbtewhite.c
	${CC} -DMFBTEGLYPHBLT=mfbTEGlyphBltWhite \
	   -DOP= -DCLIPTETEXT=mfbImageGlyphBltWhite $(CFLAGS) -c mfbtewhite.c -o $@
	$(RM) mfbtewhite.c

$(TM).md/mfbteblack.o: mfbtegblt.c $(TM).md/mfbtegblt.o
	$(RM) $@ mfbteblack.c
	$(LN) mfbtegblt.c mfbteblack.c
	${CC} -DMFBTEGLYPHBLT=mfbTEGlyphBltBlack \
	   -DOP=~ -DCLIPTETEXT=mfbImageGlyphBltBlack $(CFLAGS) -c mfbteblack.c -o $@
	$(RM) mfbteblack.c

$(TM).md/mfbseg.po: mfbseg.c $(TM).md/mfbline.po
	$(CC) -DPOLYSEGMENT $(CFLAGS) -c mfbseg.c -o $@

$(TM).md/mfbpntarea.po $(TM).md/mfbimggblt.po $(TM).md/mfbplygblt.po $(TM).md/mfbtegblt.po:
	$(RM) $@; touch $@

$(TM).md/mfbpgbwht.po: mfbplygblt.c $(TM).md/mfbplygblt.po
	$(RM) $@ mfbpgbwht.c
	$(LN) mfbplygblt.c mfbpgbwht.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltWhite \
	   -DOPEQ=\|=  $(CFLAGS) -c mfbpgbwht.c -o $@
	$(RM) mfbpgbwht.c

$(TM).md/mfbpgbblak.po: mfbplygblt.c $(TM).md/mfbplygblt.po
	$(RM) $@ mfbpgbblak.c
	$(LN) mfbplygblt.c mfbpgbblak.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltBlack \
	   -DOPEQ=\&=~ $(CFLAGS) -c mfbpgbblak.c -o $@
	$(RM) mfbpgbblak.c

$(TM).md/mfbpgbinv.po: mfbplygblt.c $(TM).md/mfbplygblt.po
	$(RM) $@ mfbpgbinv.c
	$(LN) mfbplygblt.c mfbpgbinv.c
	${CC} -DMFBPOLYGLYPHBLT=mfbPolyGlyphBltInvert \
	   -DOPEQ=\^=  $(CFLAGS) -c mfbpgbinv.c -o $@
	$(RM) mfbpgbinv.c

$(TM).md/mfbigbwht.po: mfbimggblt.c $(TM).md/mfbimggblt.po
	$(RM) $@ mfbigbwht.c
	$(LN) mfbimggblt.c mfbigbwht.c
	${CC} -DMFBIMAGEGLYPHBLT=mfbImageGlyphBltWhite \
	   -DOPEQ=\|=  $(CFLAGS) -c mfbigbwht.c -o $@
	$(RM) mfbigbwht.c

$(TM).md/mfbigbblak.po: mfbimggblt.c $(TM).md/mfbimggblt.po
	$(RM) $@ mfbigbblak.c
	$(LN) mfbimggblt.c mfbigbblak.c
	${CC} -DMFBIMAGEGLYPHBLT=mfbImageGlyphBltBlack \
	   -DOPEQ=\&=~ $(CFLAGS) -c mfbigbblak.c -o $@
	$(RM) mfbigbblak.c

$(TM).md/mfbpawhite.po: mfbpntarea.c $(TM).md/mfbpntarea.po
	$(RM) $@ mfbpawhite.c
	$(LN) mfbpntarea.c mfbpawhite.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidWhiteArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleWhiteArea \
	   -DOPEQ=\|=  -DEQWHOLEWORD=\=~0 \
	   $(CFLAGS) -c mfbpawhite.c -o $@
	$(RM) mfbpawhite.c

$(TM).md/mfbpablack.po: mfbpntarea.c $(TM).md/mfbpntarea.po
	$(RM) $@ mfbpablack.c
	$(LN) mfbpntarea.c mfbpablack.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidBlackArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleBlackArea \
	   -DOPEQ=\&=~ -DEQWHOLEWORD=\=0 \
	   $(CFLAGS) -c mfbpablack.c -o $@
	$(RM) mfbpablack.c

$(TM).md/mfbpainv.po: mfbpntarea.c $(TM).md/mfbpntarea.po
	$(RM) $@ mfbpainv.c
	$(LN) mfbpntarea.c mfbpainv.c
	${CC} -DMFBSOLIDFILLAREA=mfbSolidInvertArea \
	   -DMFBSTIPPLEFILLAREA=mfbStippleInvertArea \
	   -DOPEQ=\^=  -DEQWHOLEWORD=\^=~0 \
	   $(CFLAGS) -c mfbpainv.c -o $@
	$(RM) mfbpainv.c

$(TM).md/mfbtewhite.po: mfbtegblt.c $(TM).md/mfbtegblt.po
	$(RM) $@ mfbtewhite.c
	$(LN) mfbtegblt.c mfbtewhite.c
	${CC} -DMFBTEGLYPHBLT=mfbTEGlyphBltWhite \
	   -DOP= -DCLIPTETEXT=mfbImageGlyphBltWhite $(CFLAGS) -c mfbtewhite.c -o $@
	$(RM) mfbtewhite.c

$(TM).md/mfbteblack.po: mfbtegblt.c $(TM).md/mfbtegblt.po
	$(RM) $@ mfbteblack.c
	$(LN) mfbtegblt.c mfbteblack.c
	${CC} -DMFBTEGLYPHBLT=mfbTEGlyphBltBlack \
	   -DOP=~ -DCLIPTETEXT=mfbImageGlyphBltBlack $(CFLAGS) -c mfbteblack.c -o $@
	$(RM) mfbteblack.c

source_links:
	$(RM) mfbpgbwht.c
	$(LN) mfbplygblt.c mfbpgbwht.c
	$(RM) mfbpgbblak.c
	$(LN) mfbplygblt.c mfbpgbblak.c
	$(RM) mfbpgbinv.c
	$(LN) mfbplygblt.c mfbpgbinv.c
	$(RM) mfbigbwht.c
	$(LN) mfbimggblt.c mfbigbwht.c
	$(RM) mfbigbblak.c
	$(LN) mfbimggblt.c mfbigbblak.c
	$(RM) mfbpawhite.c
	$(LN) mfbpntarea.c mfbpawhite.c
	$(RM) mfbpablack.c
	$(LN) mfbpntarea.c mfbpablack.c
	$(RM) mfbpainv.c
	$(LN) mfbpntarea.c mfbpainv.c
	$(RM) mfbtewhite.c
	$(LN) mfbtegblt.c mfbtewhite.c
	$(RM) mfbteblack.c
	$(LN) mfbtegblt.c mfbteblack.c

clean::
	$(RM) mfbpgbwht.c
	$(RM) mfbpgbblak.c
	$(RM) mfbpgbinv.c
	$(RM) mfbigbwht.c
	$(RM) mfbigbblak.c
	$(RM) mfbpawhite.c
	$(RM) mfbpablack.c
	$(RM) mfbpainv.c
	$(RM) mfbseg.c
	$(RM) mfbtewhite.c
	$(RM) mfbteblack.c
