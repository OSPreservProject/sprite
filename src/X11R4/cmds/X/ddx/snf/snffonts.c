/************************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/* $XConsortium: snffonts.c,v 1.8 89/03/23 09:33:29 rws Exp $ */

#define NEED_REPLIES
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "snfstruct.h"

extern Atom MakeAtom();

/*
 * description
 *
 * Allocates storage with xalloc(), and returns a pointer to it.
 * Client can call xfree() to deallocate font.
 *
 * file I/O is all in this routine
 */


/*
 * XXX
 * ReadSNFFont and ReadSNFFontProperties should call a single procedure
 * with a parameter to say whether to read or skip the glyphs. If the structure
 * ever changes, it would be best to have this in a single location.
 */

FontPtr 
ReadSNFFont(fp)
    FID		fp;	/* caller has to open the file himself */
{
    FontPtr 	pfont;
    FontInfoRec	fi;
    DIXFontProp *pdfp;
    FontPropPtr	pffp;
    char	*propspace;
    unsigned	bytestoread, bytestoalloc, bytestoink;
    int		i;
    char *strings;
    char	*fontspace;
	
    bytestoread = BYTESOFFONTINFO(&fi);
    if ( FontFileRead((char *)&fi, 1, bytestoread, fp) != bytestoread)
        return NullFont;

    if (fi.version1 != FONT_FILE_VERSION || fi.version2 != FONT_FILE_VERSION)
	return NullFont;

    /*
     * we'll allocate one chunk of memory and split it among
     * the various parts of the font:
     *
     *	FontRec
     *  FontInfoRec
     *  CharInfoRec's
     *  Glyphs
     *  DIX Properties
     *  Ink CharInfoRec's
     */

    bytestoalloc = sizeof (FontRec);
    bytestoalloc += BYTESOFFONTINFO(&fi);
    bytestoalloc += BYTESOFCHARINFO(&fi);
    bytestoalloc += BYTESOFGLYPHINFO(&fi);
    bytestoalloc += fi.nProps * sizeof (DIXFontProp);
    bytestoink = bytestoalloc;
    if (fi.inkMetrics)
	bytestoalloc += BYTESOFINKINFO(&fi);

    fontspace = (char *) xalloc(bytestoalloc);
    if (!fontspace)
	return NullFont;

    /*
     * now fix up pointers
     */

    pfont = (FontPtr) fontspace;

    pfont->pFI = (FontInfoPtr) (fontspace + sizeof (FontRec));
    *pfont->pFI = fi;	/* copy data previously read */

    pfont->pCI = ADDRCharInfoRec(pfont->pFI);
    pfont->pGlyphs = ADDRCHARGLYPHS(pfont->pFI);
    pfont->pFP = ADDRXFONTPROPS(pfont->pFI);
    if (pfont->pFI->inkMetrics)
    {
	pfont->pInkMin = (CharInfoPtr)(fontspace + bytestoink);
	pfont->pInkMax = pfont->pInkMin + 1;
	pfont->pInkCI = pfont->pInkMax + 1;
    }
    else
    {
	pfont->pInkMin = &pfont->pFI->minbounds;
	pfont->pInkMax = &pfont->pFI->maxbounds;
	pfont->pInkCI = pfont->pCI;
    }

    /*
     * read the CharInfo
     */

    if ( FontFileRead((char *) pfont->pCI, 1,
    		BYTESOFCHARINFO(pfont->pFI), fp) 
		!= BYTESOFCHARINFO (pfont->pFI))
    {
	xfree(fontspace);
        return NullFont;
    }

    /*
     * read the glyphs
     */
    
    if ( FontFileRead((char *) pfont->pGlyphs, 1,
    		BYTESOFGLYPHINFO(pfont->pFI), fp) 
		!= BYTESOFGLYPHINFO (pfont->pFI))
    {
	xfree(fontspace);
        return NullFont;
    }

    /* now read and atom'ize properties */

    bytestoalloc = BYTESOFPROPINFO(pfont->pFI) + BYTESOFSTRINGINFO(pfont->pFI);
    propspace = (char *) xalloc(bytestoalloc);
    if (!propspace)
    {
	xfree(fontspace);
        return NullFont;
    }
    
    bytestoread = bytestoalloc;

    if ( FontFileRead (propspace, 1, bytestoread, fp) != bytestoread)
    {
	xfree(fontspace);
	xfree(propspace);
        return NullFont;
    }

    if (pfont->pFI->inkMetrics)
    {
	if ( FontFileRead((char *) pfont->pInkMin, 1,
			  BYTESOFINKINFO(pfont->pFI), fp) 
		    != BYTESOFINKINFO(pfont->pFI))
	{
	    xfree(fontspace);
	    xfree(propspace);
	    return NullFont;
	}
    }

    pffp = (FontPropPtr) propspace;
    
    strings = propspace + BYTESOFPROPINFO(pfont->pFI);

    for (i=0, pdfp=pfont->pFP; i<fi.nProps; i++, pdfp++, pffp++)
    {
	pdfp->name = MakeAtom(&strings[pffp->name],
			      (unsigned)strlen(&strings[pffp->name]), 1);
	if (pffp->indirect)
	    pdfp->value = (INT32)MakeAtom(&strings[pffp->value],
				   (unsigned)strlen(&strings[pffp->value]), 1);
	else
	    pdfp->value = pffp->value;
    }

    xfree(propspace);

    return pfont;
}

/*
 * read just the properties.  Similar to the code above,
 * except most of the file is skipped
 */

Bool
ReadSNFProperties(fp, pfi, ppdfp)
    FID		fp;	/* caller has to FiOpenForRead the file himself */
    FontInfoPtr pfi;
    DIXFontPropPtr *ppdfp;	/* return */
{
    int i;
    unsigned bytesskip, bytesprops;
    DIXFontPropPtr pdfp;
    FontPropPtr pffp;
    char *strings;
    char *propspace;

    *ppdfp = NullDIXFontProp;

    if ((FontFileRead((char *)pfi, 1, sizeof(FontInfoRec), fp) != sizeof(FontInfoRec)) ||
	pfi->version1 != FONT_FILE_VERSION ||
	pfi->version2 != FONT_FILE_VERSION)
        return(FALSE);

    bytesskip = BYTESOFCHARINFO(pfi) + BYTESOFGLYPHINFO(pfi);
    bytesprops = BYTESOFPROPINFO(pfi) + BYTESOFSTRINGINFO(pfi);

    /*
     * attempt to get all of the memory needed before doing any computation
     */

    propspace = (char *) xalloc(bytesprops);
    if (!propspace)
        return(FALSE);

    pdfp = (DIXFontPropPtr) xalloc (pfi->nProps * sizeof (DIXFontProp));
    if (!pdfp)
    {
	xfree (propspace);
	return FALSE;
    }

    /*
     * read both the properties and the strings, as they
     * are contiguous in the file
     */

    (void)FontFileSkip(bytesskip, fp);

    if (FontFileRead(propspace, 1, bytesprops, fp) != bytesprops)
    {
	xfree(propspace);
	xfree (pdfp);
        return(FALSE);
    }

    if (pfi->inkMetrics)
    {
	if ((FontFileRead((char *)&pfi->minbounds, 1, sizeof(CharInfoRec), fp)
	      != sizeof(CharInfoRec)) ||
	    (FontFileRead((char *)&pfi->maxbounds, 1, sizeof(CharInfoRec), fp)
	      != sizeof(CharInfoRec)))
	{
	    xfree(propspace);
	    xfree (pdfp);
	    return(FALSE);
	}
	(void)FontFileSkip(BYTESOFCHARINFO(pfi), fp);
    }

    /*
     * scan through the properties turning strings into atoms
     * and moving the data from FontProps into DIXFontProps
     */

    pffp = (FontPropPtr) (propspace);
    strings = propspace + BYTESOFPROPINFO (pfi);
    *ppdfp = pdfp;

    for (i=0; i<pfi->nProps; i++, pdfp++, pffp++)
    {
	pdfp->name = MakeAtom(&strings[pffp->name],
			      (unsigned)strlen(&strings[pffp->name]), 1);
	if (pffp->indirect)
	    pdfp->value = (INT32)MakeAtom(&strings[pffp->value],
				   (unsigned)strlen(&strings[pffp->value]), 1);
	else
	    pdfp->value = pffp->value;
    }

    xfree(propspace);
    return(TRUE);
}

void
FreeSNFFont(font)
    FontPtr font;
{
    /*
     * the font was allocated in one chunk, so we can free it
     * all at once as well
     */
    xfree(font);
}
