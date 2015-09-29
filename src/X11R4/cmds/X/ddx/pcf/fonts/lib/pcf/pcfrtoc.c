#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#include "pcf.h"

	/*\
	 * Format of font file is;
	 * FontFile	::=	version_num		: Format
         *			table_of_contents	: DescTables
	 *			tables			: LISTofFontTable
	 *
	 * DescTables	::=	num_tables		: CARD32
	 *			tables			: LISTofDescTable
	 *
	 * DescTable	::=	table_type		: CARD32
	 *			table_format		: CARD32
	 *			table_size		: CARD32
	 *			table_offset		: CARD32
	 *
	\*/

/***====================================================================***/

TableDesc *
pcfReadTableOfContents(file,pNEntries)
fosFilePtr	 file;
int		*pNEntries;
{
TableDesc	*toc;
int		i,nTables;

    /* verify version number */
    i= fosReadLSB32(file);
    if (i!=PCF_FILE_VERSION) {
	fosError("incorrect font file version (expected %d, found %d)\n",
						PCF_FILE_VERSION,i);
    }

    /* read table of contents */
    nTables= fosReadLSB32(file);
    if (pNEntries)
	*pNEntries= nTables;
    toc= (TableDesc *)fosCalloc(nTables+1,sizeof(TableDesc));
    if (toc==NULL) {
	fosError("allocation failure in pcfReadTableOfContents (toc,%d)\n",
						(nTables+1)*sizeof(TableDesc));
	return(NULL);
    }

    for (i=0;i<nTables;i++) {
	toc[i].label=	fosReadLSB32(file);
	toc[i].format=	fosReadLSB32(file);
	toc[i].size=	fosReadLSB32(file);
	toc[i].start=	fosReadLSB32(file);
    }
    toc[nTables].label=		0;
    toc[nTables].format=	0;
    toc[nTables].size=		0;
    toc[nTables].start=		0;
    return(toc);
}
