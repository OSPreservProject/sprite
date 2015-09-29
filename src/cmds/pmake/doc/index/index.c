/*-
 * index.c --
 *	Postprocessor for ditroff. Takes various things which have been
 *	placed as clear text in the ditroff output and does nice things
 *	with them. The things in clear text are produced using the macros
 *	in tmac.index.
 *
 *	The ditroff output is sent to lpr unless the -t flag is given.
 *
 *
 * Copyright (c) 1988 by the Regents of the University of California
 * Copyright (c) 1988 by Adam de Boor, UC Berkeley
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California,
 * Berkeley Softworks and Adam de Boor make no representations about
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Id: index.c,v 1.3 89/01/08 20:05:00 adam Exp $";
#endif lint

#include    <stdio.h>
#include    <ctype.h>
#include    <strings.h>
#include    <sys/file.h>
#include    <sys/signal.h>
extern char *malloc();

typedef enum { REF, DEF } refType;

typedef struct Ref {	/* Reference to a key */
    int    	  pageStart;/* Page number on which references started */
    int	    	  pageEnd;  /* Page number where they ended. If pageStart ==
			     * pageEnd, then only one page. */
    struct Ref    *next;    /* Next reference to this */
} Ref;

typedef struct Key {	/* An index key */
    char    	  *key;	    /* The text of the key */
    struct Key	  *subRefs; /* Sub references (the 'ref' list is ignored) */
    int	    	  defPage;  /* Page where key defined */
    int	    	  defined;  /* True if key defined */
    Ref   	  *refHead; /* Head of list of references */
    Ref   	  *refTail; /* Tail of list of references */
    struct Key	  *next;    /* Next key in the index */
} Key;

typedef struct Index {
    Key	    	  *allKeys; /* List of keys in the index */
    char    	  *title;   /* Title of the index */
    int	    	  valid;
} Index;

Index	*indices = (Index *)NULL;   /* Array of indices */
int	numIndices = 0;	    	    /* Number of indices going */

/*
 * Data for verifying section references. secRefs is indexed by the
 * reference number and contains either the expected or actual section number
 * for that reference. If there is a mismatch (excluding the final period),
 * an error is signaled. The array expands as needed to encompass all
 * reference numbers.
 */
typedef struct SecRef {
    char    *expect;	    /* Section number expected */
    int	    defined;	    /* Non-zero if defined */
} SecRef;

SecRef	*secRefs = NULL;
int	numSecRefs = 0;

#define DITROFF	  "/usr/public/troff_p"

/*-
 *-----------------------------------------------------------------------
 * strnew --
 *	Duplicate a string into fresh storage and return a pointer to
 *	that storage.
 *
 * Results:
 *	The new string.
 *
 * Side Effects:
 *	Memory is allocated.
 *
 *-----------------------------------------------------------------------
 */
char *
strnew(str)
    char    *str;
{
    char    *newstr;

    newstr = malloc(strlen(str)+1);
    (void) strcpy (newstr, str);
    return (newstr);
}

/*-
 *-----------------------------------------------------------------------
 * ustrcmp --
 *	Compare two strings in a lexically-unsigned manner. I.e. fold
 *	upper-case letters and lower-case letters together. All other
 *	characters are unaffected.
 *
 * Results:
 *	< 0 if s1 is lexically less, 0 if s1 and s2 are equal and > 0
 *	if s1 is lexically greater than s2.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */

int
ustrcmp (s1, s2)
    register char *s1;
    register char *s2;
{
    register int  diff;
    register int  c1;
    register int  c2;

    if (!s1) {
	s1 = "";
    }
    if (!s2) {
	s2 = "";
    }
    c1 = *s1++;
    c2 = *s2++;
    while (c1 && c2) {
	if (isupper(c1)) {
	    /*
	     * Force c1 to lower-case
	     */
	    c1 |= 040;
	}
	if (isupper(c2)) {
	    /*
	     * Force c2 to lower-case
	     */
	    c2 |= 040;
	}
	diff = c1 - c2;
	if (diff != 0) {
	    /*
	     * If there's any difference between the two characters,
	     * break out of this loop. Because of the ordering of the subtract,
	     * diff is set up to have the correct sign for the return value,
	     * so we just return it.
	     */
	    return (diff);
	} else {
	    c1 = *s1++;
	    c2 = *s2++;
	}
    }
    if (c1) {
	return (c1);
    } else if (c2) {
	return (c2);
    } else {
	return (0);
    }
}
    
    
/*-
 *-----------------------------------------------------------------------
 * findKey --
 *	Look for a key on a sorted list of keys. If the key cannot be
 *	found, create and initialize a Key structure for it.
 *
 * Results:
 *	The Key structure for the string.
 *
 * Side Effects:
 *	A Key structure may be allocated and linked into the list.
 *
 *-----------------------------------------------------------------------
 */
Key *
findKey (key, headPtr)
    char    *key;
    Key	    **headPtr;
{
    register Key  *keyPtr;
    Key	    	  *newKeyPtr;
    int	    	  diff;
    char    	  *newline = index(key, '\n');

    /*
     * First strip off any trailing white-space
     */
    if (newline) {
	*newline = '\0';
    } else {
	newline = key + strlen(key);
    }

    while (*--newline == ' ') {
	continue;
    }
    *++newline = '\0';

    /*
     * Now look for the key using the unsigned lexical comparison function
     * If it is found, the Key is returned. If the loop goes beyond the
     * last place where the key could be (the list is sorted), the loop
     * is broken, and keyPtr is left pointing at the Key before which the
     * new Key should be linked. headPtr then points to the place to
     * store the new Key's address.
     */
    for (keyPtr = *headPtr; keyPtr; keyPtr = keyPtr->next) {
	diff = ustrcmp (key, keyPtr->key);
	if (diff == 0) {
	    return (keyPtr);
	} else if (diff < 0) {
	    break;
	} else {
	    headPtr = &keyPtr->next;
	}
    }
    newKeyPtr = (Key *) malloc (sizeof(Key));
    newKeyPtr->subRefs = (Key *)NULL;
    newKeyPtr->refHead = newKeyPtr->refTail = (Ref *)NULL;
    newKeyPtr->defined = 0;
    newKeyPtr->key = strnew (key);
    newKeyPtr->next = keyPtr;
    *headPtr = newKeyPtr;
    return (newKeyPtr);
}

/*-
 *-----------------------------------------------------------------------
 * PrintKey --
 *	Print a multi-level index key.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Stuff is printed to the give FILE
 *
 *-----------------------------------------------------------------------
 */
void
PrintKey (troff, key)
    FILE    *troff;
    Key	    *key;
{
    Key	    *subKey;
    Ref	    *ref;

    fprintf (troff, ".in \\n(INu\n.ti -0.25i\n\\&%s\\ \\ ", key->key);

    if (key->defined) {
	fprintf (troff, "\\fB%d\\fP%s", key->defPage,
		 key->refHead ? ";\\ " : "");
    }
    /*
     * Print out all the pages which refer to this key. The definition
     * of a key is printed in bold italics.
     */
    if (key->refHead) {
	for (ref = key->refHead; ref; ref = ref->next){
	    if (ref->pageStart != ref->pageEnd) {
		fprintf (troff, " %d-%d", ref->pageStart, ref->pageEnd);
	    } else {
		fprintf (troff, " %d", ref->pageStart);
	    }
	    if (ref->next == (Ref *)NULL) {
		putc ('\n', troff);
	    } else {
		putc (',', troff);
	    }
	}
    } else {
	fputc ('\n', troff);
    }
    
    if (key->subRefs) {
	/*
	 * If this entry has subentries, increase the indentation
	 * one notch and recurse on all the subentries. When done,
	 * decrease the indentation a notch to get back to where we
	 * started from...
	 */
	fprintf (troff, ".in \\n+(INu\n");
	for (subKey = key->subRefs; subKey; subKey = subKey->next) {
	    PrintKey (troff, subKey);
	}
	fprintf (troff, ".in \\n-(INu\n");
    }
}
    
/*-
 *-----------------------------------------------------------------------
 * AddRef --
 *	Given the rest of the line for an index entry, fracture it into
 *	a reference to a single Key structure. The string consists of
 *	a series of space-separated, double-quote enclosed pieces, each
 *	of which is a key. The first piece is the major key and further
 *	pieces specialize that key until the final one is reached. When
 *	the final Key structure is found, place the given reference onto
 *	the end of its reference list.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Of course.
 *
 *-----------------------------------------------------------------------
 */
AddRef (index, page, type, keyString)
    int	    index;	/* The index number to which to add the reference */
    int	    page; 	/* Page number of reference */
    refType type;  	/* Type of reference */
    char    *keyString;	/* Start of the entry string. Points to the " */
{
    char    *cp;
    Key	    **keyListPtr;
    Key	    *key;
    Ref     *ref, **refPtr;

    keyListPtr = &indices[index].allKeys;
    keyString++;

    /*
     * We loop down the string looking for the end of each piece. When we
     * reach it (hit a doublequote), we look for the key in keyString on
     * the current list (either the list of allKeys or the subRefs list
     * of some key). When we get the Key structure for the current piece,
     * we update keyString to point to the start of the next piece (or the
     * end of the string) and loop.
     */
    for (cp = keyString; *cp; cp++) {
	if (*cp == '"') {
	    *cp++ = '\0';
	    key = findKey (keyString, keyListPtr);
	    keyListPtr = &key->subRefs;
	    while (*cp && isspace(*cp)) {
		cp++;
	    }
	    if (*cp) {
		keyString = ++cp;
	    } else {
		keyString = cp;
		break;
	    }
	}
    }

    if (cp != keyString) {
	/*
	 * Some piece left over -- find its Key structure
	 */
	key = findKey (keyString, keyListPtr);
    }

    /*
     * Link the reference into the final Key we found.
     * If the reference is on the same page as the most recent reference, or
     * just one page away, and it's the same type of reference, then we can
     * smush this reference into the previous one.
     * Otherwise, we have to add a new reference to the end of the list.
     * After this 'if', refPtr will point to the address of a pointer in which
     * the ref should be stuffed to maintain the list structure.
     */
    if (type != DEF) {
	if (key->refTail) {
	    if ((key->refTail->pageEnd == page) ||
		(key->refTail->pageEnd + 1 == page)) {
		    key->refTail->pageEnd = page;
		    return;
	    } else {
		refPtr = &key->refTail->next;
	    }
	} else {
	    refPtr = &key->refHead;
	}
	ref = (Ref *)malloc (sizeof(Ref));
	ref->pageStart = ref->pageEnd = page;
	key->refTail = *refPtr = ref;
	ref->next = (Ref *)NULL;
    } else {
	key->defined = 1;
	key->defPage = page;
    }
}

PrintIndex (troff, title, keys)
    FILE    *troff;
    char    *title;
    Key	    *keys;
{
    register Key *key;
    register int lastLetter;

    /*
     * Now come the initializing macro and the index itself.
     * The index is a two-column beast with a bold-faced letter at the
     * start of each section. The entries in the section are alphabetized
     * and each sub level is shifted right one place.
     */
    fprintf (troff, ".1C\n");
    fprintf (troff, ".LP\n");
    fprintf (troff, ".ce\n");
    fprintf (troff, "\\fB\\s+2%s\\s0\\fP\n", title);
    fprintf (troff, ".sp 2v\n");
    fprintf (troff, ".2C\n");
    lastLetter = '\0';
    
    /*
     * Now Print out the top-level entries in the index. The most recent
     * section letter printed is stored in the variable 'lastLetter'.
     * If it is a letter, it is upper-case. Each time the first letter of
     * a key doesn't match lastLetter, a new section letter is printed
     * and lastLetter set to the new section letter.
     */
    for (key = keys; key != (Key *)NULL; key = key->next) {
	char c = islower(key->key[0]) ? toupper(key->key[0]) : key->key[0];
	
	if (c != lastLetter) {
	    fprintf (troff, ".sp 1v\n");
	    fprintf (troff, ".in 0.25i\n");
	    fprintf (troff, ".ti -0.25i\n");
	    fprintf (troff, ".nr IN 0.5i 0.25i\n");
	    if (c == '\\') {
		fprintf (troff, "\\s+2\\fB\\e\\fP\\s0\n");
	    } else {
		fprintf (troff, "\\&\\s+2\\fB%c\\fP\\s0\n", c);
	    }
	    fprintf (troff, ".sp 0.5v\n");
	    lastLetter = c;
	}
	PrintKey (troff, key);
    }
}

main (argc, argv)
    int	    argc;
    char    **argv;
{
    char    line[256];	    	    	/* Input line */
    char    *printer = (char *)NULL;	/* Printer to give to lpr */
    FILE    *out = (FILE *)NULL;    	/* Output file (stdout or pipe) */
    char    typesetter[64]; 	    	/* Typesetter used for input file
					 * (passed to second ditroff) */
    int	    inPipes[2];	    	    	/* Pipe to ditroff's input */
    int	    outid;	    	    	/* Stream open to ditroff's output
					 * file */
    static char outFile[] = "/tmp/indexXXXXXX";/* File for ditroff's output */
    int	    pid;  	    	    	/* Process ID of ditroff */
    char    *LH = NULL;	    	    	/* Left header string */
    char    *CH = NULL;	    	    	/* Center header string */
    char    *RH = NULL;	    	    	/* Right header string */
    char    *EH = NULL;	    	    	/* Even header string */
    char    *OH = NULL;	    	    	/* Odd header string */
    char    *LF = NULL;	    	    	/* Left footer string */
    char    *CF = NULL;	    	    	/* Center footer string */
    char    *RF = NULL;	    	    	/* Right footer string */
    char    *EF = NULL;	    	    	/* Even footer string */
    char    *OF = NULL;	    	    	/* Odd footer string */
    int	    pageInit = 0;   	    	/* Initial page number for index */
    Key	    *key;
    Ref     *ref;
    int	    i;
    int	    errors = 0;	    	    	/* Number of section-reference errors
					 * encountered */
    int	    lprPID;


    /*
     * Process all arguments. Don't bitch about unknown ones...
     */
    while (--argc) {
	argv++;
	if (**argv == '-') {
	    switch (argv[0][1]) {
		case 'P':
		    printer = *argv;
		    break;
		case 't':
		    out = stdout;
		    break;
	    }
	} else {
	    freopen (*argv, "r", stdin);
	}
    }

    /*
     * If -t flag not given, set "out" to be a pipe to an lpr process. All
     * our output is sent through the 'out' file, including that of the
     * secondary DITROFF process, after being suitably filtered, of course.
     */
    if (out == (FILE *)NULL) {
	char	  *lprArgv[4];
	int 	  pipes[2];

	lprArgv[0] = "/usr/ucb/lpr";
	lprArgv[1] = "-n";
	lprArgv[2] = printer;
	lprArgv[3] = (char *)0;

	pipe(pipes);
	lprPID = vfork();
	if (lprPID == 0) {
	    dup2 (pipes[0], 0);
	    close (pipes[0]);
	    close (pipes[1]);
	    execv (lprArgv[0], lprArgv);
	    _exit(2);
	} else if (lprPID == -1) {
	    fprintf (stderr, "index: could not fork\n");
	    exit(1);
	} else {
	    close (pipes[0]);
	    out = fdopen (pipes[1], "w");
	}
    } else {
	lprPID = 0;
    }

    /*
     * Initialize the section references array, since some systems
     * bitch about realloc(NULL, n);
     */
    numSecRefs = 1;
    secRefs = (SecRef *)calloc(1, sizeof(SecRef));

    /*
     * First filter the input file, removing the 'stop' command and
     * extracting the clear-text things placed in the output by the
     * various macros. All clear-text commands are on a line by themselves
     * and begin with the letter 'X'.
     */
    while (fgets (line, sizeof(line), stdin) != (char *)NULL) {
	switch (line[0]) {
	    case 'x':
		switch (line[2]) {
		    case 's':
			/*
			 * This line is the 'stop' command to the driver.
			 * We don't want the driver to stop, however, so we
			 * just remove the command and tack on the index to
			 * end of the output we send to the output file.
			 */
			continue;
		    case 'T': 
			/*
			 * This is the specification of the typesetter used for
			 * the original output. We extract the name from the
			 * line and pass it on to the second ditroff process
			 * which we will invoke later.
			 */
			sscanf (line, "x T %s", &typesetter[2]);
			typesetter[0] = '-';
			typesetter[1] = 'P';
		    default:
			/*
			 * Any other driver-control line can be printed, as
			 * it won't do any harm.
			 */
			fputs (line, out);
			break;
		}
		break;
	    case 'X':
		/*
		 * This is one of ours. The possible lines are:
		 * 	X ENTRY	    An entry for an index
		 *	X TITLE	    Title for an index
		 *	X PN	    The page number *before* the index.
		 *	X HL
		 *	X HC
		 *	X HR
		 *	X HO
		 *	X HE	    The Left, Center, Right, Odd or Even header
		 *	X FL
		 *	X FC
		 * 	X FR
		 *	X FO
		 *	X FE	    The Left, Center, Right, Odd or Even header
		 *	X REF	    A section reference.
		 *	X DEF	    The resolution of an earlier (or later)
		 *	    	    section reference.
		 */
		switch (line[2]) {
		    case 'E': {
			register char	*cp;
			register int 	page;
			register int	index;
			refType	    	type;
			
			cp = &line[sizeof("X ENTRY")];
			for (page = 0; *cp != ' '; cp++) {
			    page = page * 10 + *cp - '0';
			}
			cp++;
			for (index = 0; *cp != ' '; cp++) {
			    index = index * 10 + *cp - '0';
			}
			cp++;
			type = *cp++ == 'r' ? REF : DEF;
			cp += 3;
			if (index >= numIndices) {
			    /*
			     * A new index is being started. Allocate room for
			     * it and initialize all those not yet initialized
			     * (in case the person starts in on index 2...),
			     * then mark the new index valid.
			     */
			    if (indices == (Index *)NULL) {
				indices = (Index *)malloc((index + 1) *
							  sizeof(Index));
			    } else {
				indices = (Index *)realloc(indices,
							   (index + 1) *
							   sizeof(Index));
			    }
			    while (numIndices <= index) {
				indices[numIndices].allKeys = (Key *)NULL;
				indices[numIndices].title = "INDEX";
				indices[numIndices].valid = 0;
				numIndices++;
			    }
			}
			indices[index].valid = 1;
			AddRef (index, page, type, cp);
			break;
		    }
		    case 'T': {
			register char *cp;
			register int index;

			cp = &line[sizeof("X TITLE")];
			for (index = 0; *cp != ' '; cp++) {
			    index = index * 10 + *cp - '0';
			}
			cp++;
			if (index >= numIndices) {
			    /*
			     * A new index is being started. Allocate room for
			     * it and initialize all those not yet initialized
			     * (in case the person starts in on index 2...),
			     * then mark the new index valid.
			     */
			    if (indices == (Index *)NULL) {
				indices = (Index *)malloc((index + 1) *
							  sizeof(Index));
			    } else {
				indices = (Index *)realloc(indices,
							   (index + 1) *
							   sizeof(Index));
			    }
			    while (numIndices <= index) {
				indices[numIndices].allKeys = (Key *)NULL;
				indices[numIndices].title = "INDEX";
				indices[numIndices].valid = 0;
				numIndices++;
			    }
			}
			/*
			 * Note that an index isn't valid until something has
			 * been entered in it...
			 */
			indices[index].title = strnew(cp);
			for (cp = indices[index].title;
			     *cp && (*cp != '\n'); cp++)
			    ;
			*cp = '\0';
			break;
		    }
		    case 'H':
			switch (line[3]) {
			    case 'C':
				CH = strnew(&line[5]);
				break;
			    case 'L':
				LH = strnew(&line[5]);
				break;
			    case 'R':
				RH = strnew(&line[5]);
				break;
			    case 'O':
				OH = strnew(&line[5]);
				break;
			    case 'E':
				EH = strnew(&line[5]);
				break;
			}
			break;
		    case 'F':
			switch (line[3]) {
			    case 'C':
				CF = strnew(&line[5]);
				break;
			    case 'L':
				LF = strnew(&line[5]);
				break;
			    case 'R':
				RF = strnew(&line[5]);
				break;
			    case 'O':
				OF = strnew(&line[5]);
				break;
			    case 'E':
				EF = strnew(&line[5]);
				break;
			}
			break;
		    case 'P':
			pageInit = atoi (&line[5]);
			break;
		    case 'R':
		    case 'D':
		    {
			int lineno = atoi(&line[6]);
			int refNum;
			char *cp, *cp2;

			cp = index(&line[6], ' ') + 1;
			refNum = atoi(cp);
			
			if (refNum >= numSecRefs) {
			    secRefs =
				(SecRef *)realloc(secRefs,
						  (refNum+10)*sizeof(SecRef));
			    bzero(&secRefs[numSecRefs],
				  (refNum+10-numSecRefs)*sizeof(SecRef));
			    numSecRefs = refNum+10;
			}
			/*
			 * Isolate the section number
			 */
			cp = index(cp, ' ') + 1;
			cp2 = rindex(cp, '.');
			if (cp2 == NULL || cp2[1] != '\n') {
			    /*
			     * No trailing . -- find the end of the line
			     * instead.
			     */
			    cp2 = index(cp, '\n');
			}
			*cp2 = '\0';

			if (secRefs[refNum].expect == NULL) {
			    /*
			     * No reference yet -- store this section number
			     * as the expected one.
			     */
			    secRefs[refNum].expect = malloc(cp2 - cp + 1);
			    strcpy(secRefs[refNum].expect, cp);
			    if (line[2] == 'D') {
				secRefs[refNum].defined = 1;
			    }
			} else if ((line[2]=='D') && secRefs[refNum].defined) {
			    fprintf(stderr,
				    "line %d: reference %d multi-defined\n",
				    lineno, refNum);
			    errors++;
			} else if (strcmp(secRefs[refNum].expect, cp) != 0) {
			    fprintf(stderr,
				    "line %d: reference %d invalid -- ",
				    lineno, refNum);
			    if (secRefs[refNum].defined) {
				fprintf(stderr, "referred to as %s, ", cp);
				fprintf(stderr, "defined as %s\n",
					secRefs[refNum].expect);
			    } else if (line[2] == 'D') {
				fprintf(stderr, "referred to as %s, ",
					secRefs[refNum].expect);
				fprintf(stderr, "defined as %s\n", cp);
				/*
				 * Store in correct definition
				 */
				free(secRefs[refNum].expect);
				secRefs[refNum].expect = malloc(cp2-cp+1);
				strcpy(secRefs[refNum].expect, cp);
				secRefs[refNum].defined = 1;
			    } else {
				fprintf(stderr, "referred to as %s and %s\n",
					secRefs[refNum].expect, cp);
			    }
			    errors++;
			} else if (line[2] == 'D') {
			    /*
			     * Current string OK, now mark as defined.
			     */
			    secRefs[refNum].defined = 1;
			}
			break;
		    }
		}
		break;
	    default:
		fputs (line, out);
		break;
	}
    }

    for (i = 0; i < numSecRefs; i++) {
	if (secRefs[i].expect && !secRefs[i].defined) {
	    fprintf(stderr, "Undefined reference %d -- expected %s\n", i,
		    secRefs[i].expect);
	    errors++;
	}
    }

    if (errors) {
	/*
	 * Stop now if we got errors, making sure to stop the lpr if it's in
	 * progress.
	 */
	if (lprPID != 0) {
	    if (kill(lprPID, SIGINT) < 0) {
		perror("couldn't kill lpr");
	    }
	}
	exit(errors);
    }
    
    /*
     * Start up the secondary ditroff process to format the index nicely.
     * We have one pipe to its input, for writing the index, and one pipe
     * from its output, for capturing the resulting ditroff text.
     */
    pipe (inPipes);
    outid = mkstemp (outFile);

    pid = vfork();
    if (pid == 0) {
	dup2 (inPipes[0], 0);
	dup2 (outid, 1);
	close(inPipes[0]); close (inPipes[1]);
	close(outid);
	if (out != stdout) {
	    close (fileno(out));
	}
#ifndef DEBUG
	execl (DITROFF, "troff_p", typesetter, "-t", "-rv1", "-ms", 0);
#endif DEBUG
	_exit(2);
    } else if (pid == -1) {
	perror ("vfork");
	exit(3);
    } else {
	/*
	 * Once the ditroff is off and running, we create another FILE for
	 * ease of writing the indices. This FILE is used at first for writing
	 * the indices, and then for reading the result.
	 */
	FILE  	  *troff;
	int 	  cpid;
	
	close (inPipes[0]);
	close (outid);
#ifndef DEBUG
	troff = fdopen (inPipes[1], "w");
#else
	troff = stderr;
#endif DEBUG	

	/*
	 * First set up the environment. Note that this does not capture all
	 * the important things, such as line spacing, point size, font,
	 * etc., but it does capture the most important ones, I think. These
	 * are the page number and the various headers and footers.
	 * The headers and footers end in \n, so no need to print it...
	 */
	fprintf (troff, ".pn %d\n", pageInit+1);
	fprintf (troff, ".ec \7\n");
	if (LH) {
	    fprintf (troff, ".ds LH %s", LH);
	}
	if (CH) {
	    fprintf (troff, ".ds CH %s", CH);
	}
	if (RH) {
	    fprintf (troff, ".ds RH %s", RH);
	}
	if (LF) {
	    fprintf (troff, ".ds LF %s", LF);
	}
	if (CF) {
	    fprintf (troff, ".ds CF %s", CF);
	}
	if (RF) {
	    fprintf (troff, ".ds RF %s", RF);
	}
	if (EH) {
	    fprintf (troff, ".EH %s", EH);
	}
	if (OH) {
	    fprintf (troff, ".OH %s", OH);
	}
	if (EF) {
	    fprintf (troff, ".EF %s", EF);
	}
	if (OF) {
	    fprintf (troff, ".OF %s", OF);
	}
	fprintf (troff, ".ec \\\n");
	for (i = 0; i < numIndices; i++) {
	    if (indices[i].valid) {
		PrintIndex (troff, indices[i].title, indices[i].allKeys);
	    }
	}
	(void) fclose (troff);
	/*
	 * Wait for ditroff to finish before going for its output...
	 */
	while ((cpid = wait(0)) != pid && (cpid != -1))
	    ;
	
	troff = fopen (outFile, "r");
	if (troff == (FILE *)NULL) {
	    perror (outFile);
	} else {
	    while (fgets (line, sizeof(line), troff) != NULL) {
		if (line[0] != 'x' || line[2] != 'i') {
		    fputs (line, out);
		}
	    }
	    (void) fclose (troff);
	}
	(void) fclose (out);
	
	(void) unlink (outFile);
    }
}
