#
# special local.mk for ddx/cfb
#
# $Source: /mic/X11R4/src/cmds/X/ddx/cfb/RCS/local.mk,v $
# $Date: 90/02/14 20:28:30 $
# $Revision: 1.1 $ $Author: tve $

#include <../../common.mk>

# Additional include file paths
.PATH.h: ../mfb ../mi

#
# "Imported" from Imakefile:
# (cfbseg.c is a hard link to cfbline.c, this has to be done by hand...)
#
$(TM).md/cfbseg.o: cfbseg.c
	${CC} -DPOLYSEGMENT $(CFLAGS) -c cfbseg.c -o $@

$(TM).md/cfbseg.po: cfbseg.c
	${CC} -DPOLYSEGMENT $(CFLAGS) -c cfbseg.c -o $@
