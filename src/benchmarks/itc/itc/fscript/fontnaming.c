/*****************************************\
* 					  *
* 	File: fontnaming.c		  *
* 	Copyright (c) 1984 IBM		  *
* 	Date: Tue May  1 21:01:31 1984	  *
* 	Author: James Gosling		  *
* 					  *
* Routines for manipulating font names.	  *
* 					  *
* HISTORY				  *
* 					  *
\*****************************************/

#include "font.h"
#include "ctype.h"

char *
FormatFontname (n)
register struct FontName *n; {
    static char buf[40];
    char    rbuf[5];
    register char  *p;
    if (n->rotation)
	sprintf (rbuf, "r%d", n->rotation);
    else
	rbuf[0] = 0;
    sprintf (buf, "%s%d%s%s%s%s%s",
	    n->FamilyName,
	    n->height,
	    rbuf,
	    n->FaceCode & BoldFace ? "b" : "",
	    n->FaceCode & ItalicFace ? "i" : "",
	    n->FaceCode & FixedWidthFace ? "f" : "",
	    n->FaceCode & ShadowFace ? "s" : "");
    for (p = buf; *p; p++)
	if (isupper (*p))
	    *p = tolower (*p);
    return buf;
}

parsefname (name, n)
register char  *name;
register struct FontName *n; {
    register    err = 0;
    register char *p = n->FamilyName;
    int i = sizeof n->FamilyName;
    while (isalpha (*name))
	if (--i>0) *p++ = *name++;
	else name++;
    *p = 0;
    n->height = 0;
    n->rotation = 0;
    while (isdigit (*name))
	n->height = n->height * 10 + (*name++ - '0');
    n->FaceCode = 0;
    while (*name)
	switch (*name++) {
	    case 'b': 
		n->FaceCode |= BoldFace;
		break;
	    case 'i': 
		n->FaceCode |= ItalicFace;
		break;
	    case 'f': 
		n->FaceCode |= FixedWidthFace;
		break;
	    case 'r': 
		while (isdigit (*name))
		    n->rotation = n->rotation * 10 + *name++ - '0';
		break;
	    case 's': 
		n->FaceCode |= ShadowFace;
		break;
	    default: 
		err++;
		break;
	}
    return err;
}
