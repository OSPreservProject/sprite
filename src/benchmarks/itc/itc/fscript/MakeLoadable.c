/****************************************************************************\
* 									     *
* 	File: MakeLoadableKeymap.c					     *
* 	Copyright (c) 1984 IBM						     *
* 	Date: Tue Feb 28 10:17:25 1984					     *
* 	Author: James Gosling						     *
* 									     *
* Compile a description of a loadable keyboard mapping table.  These beasts  *
* are used to describe keyboard interpretations to the window manager.	     *
* 									     *
* HISTORY								     *
* 									     *
\****************************************************************************/


#include "stdio.h"
#include "ctype.h"
#include "keymap.h"

struct keymap {
    struct keymap *parent;
    int slot;
    int min, max;
    struct keyent {
	char *proc;
	struct keymap *subent;
    } ent [128];
}	root;

FILE *out;
char line[200];
char *linep;
int lastlen;
char *tablename = 0;
int StringTable = 0;

error (fmt, a, b) {
    fprintf (stderr, "MakeKeymap: ");
    fprintf (stderr, fmt, a, b);
    fprintf (stderr, "\r");
    exit (1);
}

char   *getword () {
    register char  *p = linep;
    register char  *d = linep;
    while (isspace (*p))
	p++;
    while (*p && !isspace (*p))
	if (*p == '\\X') {
	    switch (*++p) {
		case 0: 
		    break;
		case '^': 
		    *d++ = *++p=='?' ? 0177 : *p & 037;
		    break;
		case 'n': 
		    *d++ = '\n';
		    break;
		case '?': 
		    *d++ = '\177';
		    break;
		case 'r': 
		    *d++ = '\r';
		    break;
		case 'b': 
		    *d++ = '\b';
		    break;
		case 't': 
		    *d++ = '\t';
		    break;
		case 'e': 
		    *d++ = '\033';
		    break;
		default: 
		    *d++ = *p;
	    }
	    if (*p)
		p++;
	}
	else
	    *d++ = *p++;
    while (isspace (*p))
	p++;
    lastlen = d - linep;
    *d++ = 0;
    d = linep;
    linep = p;
    return d;
}

struct word {
    char *word;
    int hash;
    char IsProc;
    struct word *next;
};

#define HashLength 137
struct word *whash[HashLength];

struct word *savestr(s)
char   *s; {
    register char  *r;
    register    hash = 0;
    register struct word **p;
    register    len;
    for (r = s; *r;)
	hash = hash * 37 + *r++;
    len = r - s;
    if (hash < 0)
	hash = -hash;
    p = &whash[hash % HashLength];
    while (*p)
	if ((*p) -> hash == hash && strcmp (s, (*p) -> word) == 0)
	    return * p;
	else
	    p = &(*p) -> next;
    *p = (struct word  *) malloc (sizeof (struct word));
    (*p) -> next = 0;
    (*p) -> hash = hash;
    (*p) -> IsProc = 0;
    strcpy ((*p) -> word = (char *) malloc (len + 1), s);
    return * p;
}

putctlchar (c) {
    if (c < 040) {
	putchar ('^');
	putchar (c + '@');
    } else if (c == 0177) {
	putchar ('^');
	putchar ('?');
    } else putchar (c);
}

PrintTableName (k)
register struct keymap *k; {
    if (k -> parent) {
	PrintTableName (k -> parent);
	putctlchar (k -> slot);
    }
}

int StringOffset = 0;

OffsetTableSize (k)
register struct keymap *k; {
    register    i;
    register    n = k -> max - k -> min + 1;
    for (i = k -> min; i <= k -> max; i++)
	if (k -> ent[i].subent)
	    n += OffsetTableSize (k -> ent[i].subent);
    return n;
}

DumpTable (k)
register struct keymap *k; {
    register    i;
    register    nzero = 0;
    for (i = k -> min; i <= k -> max; i++) {
	register char  *str;
	int     val;
	register struct keyent *key = &k -> ent[i];
	if (str = key -> proc) {
	    val = StringOffset;
	    StringOffset += strlen (str) + 1;
	}
	else
	    val = key -> subent ? -1 : 0;
	fwrite (&val, sizeof val, 1, out);
    }
    for (i = k -> min; i <= k -> max; i++)
	if (k -> ent[i].subent)
	    DumpTable (k -> ent[i].subent);
}


DumpStrings (k)
register struct keymap *k; {
    register    i;
    register    nzero = 0;
    for (i = k -> min; i <= k -> max; i++) {
	register char  *name;
	register struct keyent *key = &k -> ent[i];
	if (name = key -> proc) {
	    if (key -> subent)
		error ("%s overlaps with a submap", name);
	    fwrite (name, strlen (name) + 1, 1, out);
	}
    }
    for (i = k -> min; i <= k -> max; i++)
	if (k -> ent[i].subent)
	    DumpStrings (k -> ent[i].subent);
}

Hashcode (k)
register struct keymap *k; {
    return k == 0 || k -> parent == 0
	? 0
	: (Hashcode (k -> parent) << 8) + k -> slot;
}

DumpDict (k)
register struct keymap *k; {
    register    i;
    struct PrefixTable t;
    t.PackedPrefixKeys = Hashcode (k);
    t.FirstKey = k -> min;
    t.NumberOfKeys = k -> max - k -> min + 1;
    fwrite (&t, sizeof t, 1, out);
    for (i = k -> min; i <= k -> max; i++)
	if (k -> ent[i].subent)
	    DumpDict (k -> ent[i].subent);
}

DictSize (k)
register struct keymap *k; {
    register    i;
    register t = 1;
    for (i = k -> min; i <= k -> max; i++)
	if (k -> ent[i].subent)
	    t += DictSize (k -> ent[i].subent);
    return t;
}

main (argc, argv)
char  **argv; {
    char    outfile[50];
    if (argc != 2) {
	printf ("Usage: %s filename\n", argv[0]);
	exit (0);
    }
    tablename = argv[1];
    if (freopen (tablename, "r", stdin) == NULL) {
	printf ("Can't open %s\n", tablename);
	exit (0);
    }
    {
	register char  *extpos = 0;
	register char  *s,
	               *d;
	s = tablename;
	d = outfile;
	while (*d = *s++)
	    if (*d++ == '.')
		extpos = d;
	strcpy (extpos ? extpos - 1 : strlen (tablename) + outfile, ".wmap");
    }
    if ((out = fopen (outfile, "w")) == 0) {
	printf ("Can't create %s\n", outfile);
	exit (0);
    }
    printf ("%s => %s\n", tablename, outfile);
    root.min = 9999;
    root.max = -1;
    while (gets (line)) {
	char   *com;
	int     coml;
	register struct word   *proc;
	if (line[0] == '#')
	    continue;
	linep = line;
	com = getword ();
	coml = lastlen;
	proc = savestr (getword ());
	proc -> IsProc = 1;
	if (coml > 2 && com[coml - 2] == '-')
	    while (com[coml - 3] <= com[coml - 1]) {
		InsertEntry (com, coml - 2, proc -> word);
		com[coml - 3]++;
	    }
	else
	    InsertEntry (com, coml, proc -> word);
    }
    if (tablename == 0)
	error ("No table name");
    {
	long    t = DictSize (&root);
	fwrite (&t, sizeof t, 1, out);
	StringOffset = OffsetTableSize (&root) * sizeof (int) + t * sizeof (struct PrefixTable) + sizeof t;
    }
    DumpDict (&root);
    DumpTable (&root);
    DumpStrings (&root);
    fclose (out);
    exit (0);
}

InsertEntry (com, len, proc)
char   *com,
       *proc; {
    register struct keymap *k = &root;
    while (--len >= 0) {
	register    slot = *com++;
	register struct keyent * p = &k -> ent[slot];
	    if (slot < k -> min) k -> min = slot;
	    if (slot > k -> max) k -> max = slot;
	if (len == 0) {
	    p -> proc = proc;
	    break;
	}
	if (p -> subent == 0) {
	    register    i;
	    register struct keymap *nk = (struct keymap *) malloc
	                                                (sizeof (struct keymap));
	    p -> subent = nk;
	    for (i = 0; i < 128; i++) {
		nk -> ent[i].proc = 0;
		nk -> ent[i].subent = 0;
	    }
	    nk -> parent = k;
	    nk -> slot = slot;
	    nk -> min = 9999;
	    nk -> max = -1;
	}
	k = p -> subent;
    }
}
