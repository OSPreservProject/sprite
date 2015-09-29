#ifndef BDFINT_H
#define BDFINT_H

#define bdfIsPrefix(buf,str)	(!strncmp(buf,str,strlen(str)))
#define	bdfStrEqual(s1,s2)	(!strcmp(s1,s2))

#define	BDF_GENPROPS	6
#define NullProperty	((FontPropPtr)0)

/*
 * This structure holds some properties we need to generate if they aren't
 * specified in the BDF file and some other values read from the file
 * that we'll need to calculate them.  We need to keep track of whether 
 * or not we've read them.
 */
typedef struct BDFSTAT {
	int		 linenum;
	char		*fileName;
	char		 fontName[BUFSIZ];
	float		 pointSize;
	int		 resolution;
	int		 digitCount;
	int		 digitWidths;
	int		 exHeight;

	FontPropPtr	 fontProp;
	FontPropPtr	 pointSizeProp;
	FontPropPtr	 resolutionProp;
	FontPropPtr	 xHeightProp;
	FontPropPtr	 weightProp;
	FontPropPtr	 quadWidthProp;
	BOOL		 haveFontAscent;
	BOOL		 haveFontDescent;
} bdfFileState;

extern	Atom	BDFA_FONT_ASCENT;
extern	Atom	BDFA_FONT_DESCENT;
extern	Atom	BDFA_DEFAULT_CHAR;

#define	bdfPrivate(f)	((bdfFileState *)(f)->fmtPrivate)

extern	unsigned char	*bdfGetLine();

#endif /* BDFINT_H */
