/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: scanpost.c,v 1.1 88/02/11 17:08:55 jim Exp $";
#endif

/*
 * ScanPostAmble - read a DVI postamble.
 */

#include <stdio.h>
#include "types.h"
#include "dvicodes.h"
#include "fio.h"
#include "postamble.h"

ScanPostAmble(f, headerfunc, fontfunc)
	register FILE *f;
	int (*headerfunc)();
	register int (*fontfunc)();
{
	register int n;
	register char *s;
	char name[512];

	if (FindPostAmble(f))
		GripeCannotFindPostamble();
	if (GetByte(f) != Sign8(DVI_POST))
		GripeMissingOp("POST");

	/* Read the postamble info stuff. */
	{
		struct PostAmbleInfo pai;
		register struct PostAmbleInfo *p = &pai;

		p->pai_PrevPagePointer = GetLong(f);
		p->pai_Numerator = GetLong(f);
		p->pai_Denominator = GetLong(f);
		p->pai_DVIMag = GetLong(f);
		p->pai_TallestPageHeight = GetLong(f);
		p->pai_WidestPageWidth = GetLong(f);
		p->pai_DVIStackSize = GetWord(f);
		p->pai_NumberOfPages = GetWord(f);

		(*headerfunc)(p);
	}

	/* Now read all the font definitions. */
	{
		struct PostAmbleFont paf;
		register struct PostAmbleFont *p = &paf;

		for (;;) {
			switch (UnSign8(getc(f))) {

			case DVI_FNTDEF1:
				p->paf_DVIFontIndex = UnSign8(getc(f));
				break;

			case DVI_FNTDEF2:
				p->paf_DVIFontIndex = UnSign16(GetWord(f));
				break;

			case DVI_FNTDEF3:
				p->paf_DVIFontIndex = UnSign24(Get3Byte(f));
				break;

			case DVI_FNTDEF4:
				p->paf_DVIFontIndex = GetLong(f);
				break;

			case DVI_POSTPOST:
				return;

			default:
				GripeMissingOp("POSTPOST");
				/*NOTREACHED*/
			}
			p->paf_DVIChecksum = GetLong(f);
			p->paf_DVIMag = GetLong(f);
			p->paf_DVIDesignSize = GetLong(f);
			p->paf_n1 = UnSign8(getc(f));
			p->paf_n2 = UnSign8(getc(f));
			p->paf_name = name;	/* never trust people not to
						   clobber it */
			n = p->paf_n1 + p->paf_n2;
			s = name;
			while (--n >= 0)
				*s++ = GetByte(f);
			*s = 0;
			(*fontfunc)(p);
		}
	}
}
