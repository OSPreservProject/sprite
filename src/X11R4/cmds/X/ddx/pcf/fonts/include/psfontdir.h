#ifndef KNOWS_PS
#define KNOWS_PS 1

	/*
	 * KNOWS_PS is also used by fontdir.h
	 */

typedef struct _PSXInfo {
	int	 pointSize;
	int	 xResolution;
	int	 yResolution;
	char	*xName;
	struct _PSXInfo *pNext;
} PSXInfo;

typedef struct _PSFontMap {
	char	*psName;
	PSXInfo	*xMappings;
	struct _PSFontMap *pNext;
} PSFontMap;

#endif
