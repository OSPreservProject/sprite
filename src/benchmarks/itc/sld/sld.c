/* $Header: sld.c,v 2.26 88/10/02 16:46:55 hilfingr Exp $ */

/* 		SPUR Loader 				*/

/* Copyright (C) 1987 by the Regents of the University of California.  All 
 * rights reserved.
 *
 * Author: P. N. Hilfinger
 */

static char *rcsid = "$Header: sld.c,v 2.26 88/10/02 16:46:55 hilfingr Exp $";

#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ar.h>
#include <ranlib.h>
#include "a.out.h"

           /* System parameters */

#define MAXLOADFILES     300	/* Maximum number of files that may be 
				 * loaded together */
#define GLOBALHASHSIZE	2053	/* Size of hash table for global 
				 * symbols (prime) */
#define MAXEXPRSYLS      100    /* Maximum number of syllables in a relocation 
				 * expression */
#define MAXFILENAMESIZE 100

           /* Various constants */
#define TRUE	1
#define FALSE	0

#define SCONSLOWMASK  0x000001ff
#define SCONSHIGHMASK 0x01f00000
#define SCONSMASK     (SCONSLOWMASK | SCONSHIGHMASK)
#define SCONSHIGHPOSN 20
#define SCONSLOWLEN   9

    /* Masks, etc.,  corresponding to bits indicated by RP_LOW8, ..., RP_32 */
static unsigned int lengthBitMask[] = {
    0xff, 0x1ff, 0x3fff, 0xffff, 0x01f001ff, 0x0fffffff, 0xffffffff };

    /* Numbers of bits affected by each of RP_LOW8, etc.   RP_LOW28 is a
     * special case, not covered by this table.  
     */
static unsigned int lengthMap[] = {
    8, 9, 14, 16, 14, 0, 32 
    };

    /* Maximum and minimum values for fields.  See comment on RP_LOW28
     * above. The values for LOW8 and
     * LOW16 are chosen to allow any valid 16-bit 2's complement
     * signed integer or 16-bit unsigned integer.
     */
static unsigned int minMap[] = {
    -(1<<7), -(1<<8), -(1<<13), -(1<<15), -(1<<13), 0, -(1<<31)
    };
static unsigned int maxMap[] = {
    (1<<8)-1, (1<<8)-1, (1<<13)-1, (1<<16)-1, (1<<13)-1, 0, (1<<31)-1
    };

    /* Additional load file inquiries */

				/* Given address after text and magic */
				/* number, the beginning of private data. */
				/* WARNING: SHOULD REALLY BE IN a.out.h */
#define DATA_ADDR(textEnd,magic) \
    ((magic) == ZMAGIC ? 0xc0000000 : (textEnd))

				/* Length of actual text (excluding */
				/* header) in file with given header. */
				/* WARNING: SHOULD REALLY BE IN a.out.h */
#define REAL_TEXT_LENGTH(header) \
    ((header).a_magic == ZMAGIC ? (header).a_text - sizeof(struct exec) \
     : (header).a_text)

    /* Standard load libraries */

static char *standardLibs[] = 
{
    "/lib/", "/usr/lib/", "/usr/local/lib/", NULL
    };

		/* Data structures */

typedef int bool;

struct _globalSymType {		/* Serve as unique representatives for global 
				   symbols */
    char 	*name;		/* External identifier */
    int		file;		/* Index of load file of representative 
				   instance. */
    struct nlist *sym;		/* Representative instance (redundant). */
    struct _globalSymType
	        *link;		/* Link for hash table. */
    struct _globalSymType
	        *nextUnresolved, *lastUnresolved;
				/* Links for list of unresolved references. */
};

typedef struct _globalSymType	globalSymType;

       /* Forwarding pointer for symbols */
#define globalReferent(sym)     (*((globalSymType **) &((sym) -> n_un.n_strx)))

                /* Values for r_extra field in struct relocation_infos. */
#define RELOC_DONE      1	/* No further relocation needed. */
#define RELOC_EXCLUDE   2	/* No further relocation needed and the 
				 * relocation
				 * item need not be retained for future 
				 * relocations */

                /* Values for n_other fields */
#define SYM_USED        1	/* Symbol used for relocation */
#define SYM_EXCLUDE     2       /* Symbol to be thrown out */

		/* Global objects */

static char *programName;	/* Argument 0 to program. */

static int errorCount;

static struct exec aHeader;	/* Handy header (put here to avoid lint
				 * hassles) */

static globalSymType 	*globals[GLOBALHASHSIZE];	
				/* Hash table for global symbols */
static globalSymType	unresolvedSyms =
     { NULL, 0, NULL, NULL, 0, 0};
				/* Header for list of unresolved symbols */
static struct exec	fileHeader[MAXLOADFILES];
				/* The headers for the loaded files. */
static struct nlist  	*fileDefns[MAXLOADFILES];
				/* The symbol tables of the loaded files. */
static struct relocation_info
                        *fileRelocs[MAXLOADFILES];
                                /* The relocation data for the loaded files. */
static char 		*fileStrings[MAXLOADFILES],
				/* String areas for the loaded files. */
                        *fileName[MAXLOADFILES];

static union reloc_expr *fileRelocExprs[MAXLOADFILES];
				/* Relocation expressions for the loaded 
				 * files. */
static unsigned int
				/* Starts of text and data areas for the loaded
				   files. */
			*fileText[MAXLOADFILES],
			*fileData[MAXLOADFILES],
                        *fileSData[MAXLOADFILES],

				/* Load addresses for the segments of each 
				   file. */
                        fileTextAddr[MAXLOADFILES],
                        fileDataAddr[MAXLOADFILES],
                        fileSDataAddr[MAXLOADFILES],
                        fileBssAddr[MAXLOADFILES],
                        fileSBssAddr[MAXLOADFILES],
                                /* Amount segment relocated, indexed by file 
				   and n_type. */
                        fileDelta[MAXLOADFILES][N_SBSS + 1];
static unsigned  int numFiles;	/* Number of object files */

static int EOtoNmap[EO_SBSS+1]; /* Maps EO_TEXT, ... to N_TEXT, ... */

				/* Command line switches */

static bool
    dSwitch,
    MSwitch,
    pdSwitch,
    ptSwitch,
    rSwitch,
    sSwitch,
    SSwitch,
    tSwitch,
    TtextSwitch,		/* -T or -Ttext present */
    TdataSwitch,
    TsdataSwitch,
    wSwitch,
    xSwitch,
    XSwitch;

static char *baseFileName;	/* From -A option */
static char *outFileName;	/* From -o option */
static int magic;		/* Magic number for file */
static char *entryName;		/* From -e option */

static unsigned int textStart;	/* Text starting address. */
static unsigned int dataStart;	/* Data starting address from -Tdata; defined
				/* only if TdataSwitch. */
static unsigned int sdataStart; /* Shared data starting address from -Tsdata;
				/* defined only if TsdataSwitch */
static char **dirList;		/* Vector of names from -L option (ended by
				 * null pointer) */
static char **tracedSyms;	/* Vector of names from -y options (ended by
				 * null pointer) */
static struct nlist *forcedSyms;
                                /* Vector of names from -u options (ended by
				 * null pointer) */
static unsigned int numForcedSyms, numTracedSyms;

			/* Forward declarations */
extern int
    hash(),
    readOpenObjectFile(),
    exprLength(),
    indirectStrcmp(),
    ranlibCompare();

extern bool
    matchPhrase(),
    evalReloc();

extern unsigned int
    evalConstUnaryExpr(),
    evalConstBinaryExpr(),
    evalSym(),
    segmentOffsetToNewAddr(),
    evalLoadExpr(),
    extractVal();

extern unsigned char
    *relocSegmentToPntr();

extern globalSymType
    **findSymbolEntry();

extern void
    realSym(),
    internSym(),
    convertStringIndex(),
    convertStringIndicies(),
    insertVal(),
    computeRelocations(),
    resolveCommon(),
    setLoc(),
    incrLoc(),
    readOpenLibraryFile(),
    setupForcedSyms(),
    setupBaseFile(),
    relocateAll(),
    relocSizes(),
    relocateSymbols(),
    computeEnds(),
    relocateExprs(),
    countAndMarkSymbols(),
    createNewSymbolsAndStrings(),
    createNewRelocs(),
    writeObjectFile(),
    writeCombinedFile();

extern union reloc_expr
    *simpLoadExpr(),
    *simpCanonLoadExpr(),
    *reduceExpr(),
    *compressExpr(),
    *createNewRelocExpr();

extern char
    *bsearch(),
    *calloc(),
    *malloc();

extern long
    lseek();

		/* Various utilities */

void
printErrorHeader()
{
    (void) fprintf(stderr, "%s: ", programName);
}

#define ErrorMsg    {\
    printErrorHeader(); \
    (void) fprintf(stderr, 

#define EndMsg     \
    );  \
    (void) putc('\n', stderr); \
    errorCount += 1; \
    errorBreak();\
 }

#define FormatError { \
    ErrorMsg "Format error in load file." EndMsg; \
    exit(1); \
    }

#define InternalError { \
    ErrorMsg "Internal loader error." EndMsg; \
    exit(1); \
    }

#define InsufficientSpace  { \
    ErrorMsg  "Insufficient memory to build load file." EndMsg; \
    exit(1); \
    }

void
errorBreak()
{
}

         /* Result of incrementing  pointer by byte offset */
#define BINCR(p, offset, type) ((type *) ((int) (p) + (int) (offset)))

         /* Byte address difference between two pointers */
#define BDIFF(p1, p2) ((int) (p1) - (int) (p2))


		/* Resolving global symbols */

void
realSym(inSym, inFile, outSym, outFile)
     struct nlist *inSym, **outSym;
     unsigned int inFile, *outFile;
     /* For any symbol inSym, returns the "real" definition of sym in *outSym,
      * i.e., either *inSym itself, or, in the case of an external definition,
      * the global defining instance of *inSym. The number of the file 
      * containing inSym must be passed as inFile if inSym is not an 
      * external reference; otherwise it is ignored.  The variable *outFile is
      * set to the number of the file containing *outSym.  */
{
    switch (inSym -> n_type & N_TYPE) {
    case N_UNDF:
    case N_COMM:
    case N_SCOMM: {
	globalSymType *g = globalReferent(inSym);
	*outSym = g -> sym;
	*outFile = g -> file;
	break;
    }
    default:
	*outSym = inSym;
	*outFile = inFile;
	break;
    }
}

char *
symName(sym)
     struct nlist *sym;
     /* Identifier associated with sym. */
{
    switch (sym -> n_type) {
    case N_UNDF | N_EXT:
    case N_UNDF:
    case N_COMM | N_EXT:
    case N_SCOMM | N_EXT:
	return globalReferent(sym) -> name;
    default:
	return sym -> n_un.n_name;
    }
}	

int
hash(s)
     char *s;
     /* Compute a hash value from s in the range 0 .. GLOBALHASHSIZE-1. */
     /* Taken from P. J. Weinberger, as published in Aho, Sethi, Ullman. */
{
    char *p;
    unsigned h = 0, g;

    for (p = s; *p != '\0'; p++) {
	h = (h << 4) + *p;
        g = h & 0xf0000000;
	if (g != 0)
	    h ^= (g >> 24) ^ g;
    }
    return (int) (h % GLOBALHASHSIZE);
}

globalSymType **
findSymbolEntry(str)
     char *str;
     /* Reference to a variable containing the current representative entry, 
      * if any, for str, or NULL if none.   This variable may be set to a
      * new entry to add a symbol.  */
{
    globalSymType **rep;

    for (rep = &globals[hash(str)]; 
	 *rep != NULL && strcmp((*rep) -> name, str) != 0;
	 rep = &((*rep) -> link));
    return rep;
}

void
internSym(sym, file)
     unsigned int file;
     struct nlist *sym;
     /* Where *sym is a symbol definition in object file number file, with
      * the n_un.n_name field valid.
      * If sym is a global definition:
      *     1. Check that sym is not multiply defined and report if it is.
      *	    2. If (1) succeeds, establish sym as the defining instance for the
      *        symbol it contains.  If (1) fails, change sym to an external 
      *	       reference to the previous definition (see (3) below).
      * If sym is an external reference or .comm symbol:
      *     3. Set globalReferent(sym) to a globalSymType unique to that 
      *        symbol.
      *     4. If this is the first encounter of sym, make sym the 
      *	       representative instance.  Also, if the symbol is a .comm symbol,
      *        and all previous references have been external references or 
      *        .comm symbols with smaller space requests, make this 
      *        the representative instance.
      * Assumes that the n_un.n_name field is initially valid.
      */
{
    char *str = sym -> n_un.n_name;
    globalSymType *rep, **prep;
    int type = sym -> n_type;
    int stype = type & N_TYPE;

    if ((type & N_STAB) != 0 || (type & N_EXT) == 0)
	return;

    if (str[0] == '\0') {
	ErrorMsg "Global or external symbol (#%d) with null name in %s.", 
	         sym - fileDefns[file], fileName[file]
	EndMsg;
	return;
    }
    
    prep = findSymbolEntry(str);
    rep = *prep;

    if (rep == NULL) {
	rep = (globalSymType *) malloc(sizeof(globalSymType));
	rep -> name = str;
	rep -> file = file;
	rep -> sym = sym;
	rep -> link = NULL;
	*prep = rep;

	if (stype == N_UNDF) {
	    rep -> nextUnresolved = unresolvedSyms.nextUnresolved;
	    rep -> lastUnresolved = &unresolvedSyms;
	    rep -> nextUnresolved -> lastUnresolved = rep;
	    rep -> lastUnresolved -> nextUnresolved = rep;
	}
	else rep -> nextUnresolved = rep -> lastUnresolved = NULL;
    }
    else {
	int oldstype = (rep -> sym -> n_type) & N_TYPE;
	if ((type & N_EXT) != 0 && 
	    stype != N_UNDF && stype != N_COMM && stype != N_SCOMM &&
	    oldstype != N_UNDF && oldstype != N_COMM && oldstype != N_SCOMM) {
	    ErrorMsg "Global symbol %s defined in %s and %s.", 
	           str, fileName[rep -> file], fileName[file]
	    EndMsg;
	    sym -> n_type = N_EXT | N_UNDF;
	    globalReferent(sym) = rep;
	}
    }

    switch (type & N_TYPE) {

    case N_UNDF:
	globalReferent(sym) = rep;
	break;

    case N_COMM:
    case N_SCOMM:
	globalReferent(sym) = rep;
	switch (rep -> sym -> n_type & N_TYPE) {
	case N_UNDF:
	    rep -> file = file, rep -> sym = sym;
	    break;
	case N_COMM:
	case N_SCOMM:
	    if ((rep -> sym -> n_type & N_TYPE) != (type & N_TYPE)) {
		ErrorMsg "Conflicting definitions of common region %s.", str
		    EndMsg;
		sym -> n_type = rep -> sym -> n_type;
	    }
	    if (sym -> n_value > rep -> sym -> n_value)
		rep -> file = file, rep -> sym = sym;
	    break;
	default:
	    break;
	}
	if (rep -> nextUnresolved != NULL) {
	    rep -> nextUnresolved -> lastUnresolved = rep -> lastUnresolved;
	    rep -> lastUnresolved -> nextUnresolved = rep -> nextUnresolved;
	    rep -> lastUnresolved = rep -> nextUnresolved = NULL;
	}
	break;

    default:
	rep -> file = file;
	rep -> sym = sym;
	if (rep -> nextUnresolved != NULL) {
	    rep -> nextUnresolved -> lastUnresolved = rep -> lastUnresolved;
	    rep -> lastUnresolved -> nextUnresolved = rep -> nextUnresolved;
	    rep -> lastUnresolved = rep -> nextUnresolved = NULL;
	}
	break;
    }
}

void
convertStringIndex(sym,file)
     struct nlist *sym;
     unsigned int file;
{
    if (sym -> n_un.n_strx == 0)
	sym -> n_un.n_name = "";
    else 
	sym -> n_un.n_name = fileStrings[file] + sym -> n_un.n_strx;
}

void
convertStringIndices(file)
     unsigned int file;
     /* Convert n_strx fields for all symbols in file number file to string
      * pointers. */
{
    struct nlist
	*last = (struct nlist *) ((int) fileDefns[file] + 
				  fileHeader[file].a_syms),
	*sym;
    for (sym = fileDefns[file]; sym != last; sym++) {
	convertStringIndex(sym,file);
    }
}

void
internAllSyms(file)
     unsigned int file;
     /* Intern all symbols in object file number file. Assumes that all
      * string indices have been replaced by strings in the symbols for
      * file. */
{
    struct nlist
	*last = (struct nlist *) ((int) fileDefns[file] + 
				  fileHeader[file].a_syms),
	*sym;
    for (sym = fileDefns[file]; sym != last; sym++) {
	if (numTracedSyms > 0 && file != 0 
	    && bsearch((char *) (&sym -> n_un.n_name), (char *) tracedSyms, 
		       numTracedSyms, sizeof(char *),
		       indirectStrcmp) != (char *) NULL ) {

	    int stype = sym -> n_type & N_TYPE;
	    char *name = sym -> n_un.n_name;

	    (void) printf("%s: ", fileName[file]);

	    switch (stype) {
	    case N_UNDF:
		(void) fputs("reference to ", stdout);
		break;
	    default:
		(void) fputs("definition of ", stdout);
		break;
	    }
	    
	    if (stype == N_COMM) 
		(void) printf("common %s size %d\n", name, sym -> n_value);
	    else if (stype == N_SCOMM)
		(void) printf("shared common %s size %d\n", name, 
			      sym -> n_value);
	    else {
		if (sym -> n_type & N_EXT)
		    (void) fputs("external ", stdout);

		(void) printf("%s %s\n",
			      stype == N_UNDF ? "undefined " :
			      stype == N_TEXT ? "text " :
			      stype == N_DATA ? "data " :
			      stype == N_BSS ? "bss " :
			      stype == N_SDATA ? "shared data " :
			      stype == N_SBSS ? "shared bss " :
			      "?", name);
	    }
	}
	internSym(sym, file);
    }
}

			 /* Expression simplification and evaluation */

int
exprLength(e)
     union reloc_expr *e;
     /* The length in words of e.  (Also checks the validity of the 
      * expression.) */
{
    switch (e -> re_syl.re_op) {
    case EO_INT:
    case EO_SYM:
    case EO_TEXT:
    case EO_DATA:
    case EO_BSS:
	return 2;
    case EO_MINUS:
    case EO_COMP:
	return 1 + exprLength(e+1);
    case EO_PLUS:
    case EO_MULT:
    case EO_DIV:
    case EO_SLL:
    case EO_SRL:
    case EO_AND:
    case EO_OR:
    case EO_XOR:
    case EO_SUB:
	{
	    int L1 = exprLength(e+1);
	    return L1 + 1 + exprLength(e+L1+1);
	}
    default:
	FormatError;     /*NOTREACHED*/
    }
}	

unsigned int
evalConstUnaryExpr(op, left)
     unsigned int op, left;
     /* Evaluate expression `op left', where op is an re_op.  */
{
    switch (op) {
    case EO_MINUS:
	return  -left;
    case EO_COMP:
	return ~left;
    default:
	InternalError;      /*NOTREACHED*/
    }
}

unsigned int
evalConstBinaryExpr(op, left, right)
     unsigned int op, left, right;
     /* Evaluate expression `left op right', where op is an re_op. */
{
    switch (op) {
    case EO_PLUS:
	return left + right;
    case EO_MULT:
	return left  * right;
    case EO_DIV:
	if (right != 0) return left / right;
	else {
	    ErrorMsg  "Division by zero in load-time expression." EndMsg;
	    return 0;
	}
    case EO_SLL:
	return left << right;
    case EO_SRL:
	return left >> right;
    case EO_AND:
	return left & right;
    case EO_OR:
	return left | right;
    case EO_XOR:
	return left ^ right;
    case EO_SUB:
	return left - right;
    default:
	FormatError;     /*NOTREACHED*/
    }
}

unsigned int
evalSym(e, file, defined)
     union reloc_expr *e;
     unsigned int file;
     bool *defined;
     /* Given expression e consisting of a symbol, appearing in given file,
      * returns its value, if defined.  Sets *defined to TRUE iff value
      * defined.  If *defined FALSE, return value is undefined. */
{
    struct nlist *sym;
    unsigned int symFile;

    realSym(fileDefns[file] + e -> re_syl.re_arg, file, &sym, &symFile);

    *defined = TRUE;
    switch (sym -> n_type & N_TYPE) {
    case N_ABS:
    case N_EXPR:
	return (sym -> n_value + e[1].re_value);
    case N_TEXT:
    case N_DATA:
    case N_SDATA:
    case N_BSS:
    case N_SBSS:
	return (sym -> n_value + e[1].re_value);
    default:
	*defined = FALSE;
	
	return 0;
    }
}

unsigned int
evalLoadExpr(e, file, defined)
     union reloc_expr *e;
     unsigned int file;
     bool *defined;
     /* Given load expression e appearing in given load file, returns its
      * value if defined, and sets *defined to TRUE.  Otherwise, sets
      * *defined to FALSE, and returns an undefined value. */
{
    unsigned int op1, op2;
    bool defined1;
    *defined = TRUE;
    switch (e -> re_syl.re_op) {
    case EO_INT:
	return e[1].re_value;
    case EO_SYM:
	return evalSym(e, file, defined);
    case EO_TEXT:
	return fileTextAddr[file] + e[1].re_value;
    case EO_DATA:
	return fileDataAddr[file] + e[1].re_value;
    case EO_BSS:
	return fileBssAddr[file] + e[1].re_value;
    case EO_MINUS:
    case EO_COMP:
	op1 = evalLoadExpr(e+1, file, defined);
	if (*defined)
	    return evalConstUnaryExpr(e -> re_syl.re_op, op1);
	else return 0;
    default: 		/* Binary expression */
	op1 = evalLoadExpr(e+1, file, &defined1);
	op2 = evalLoadExpr(e+exprLength(e+1)+1, file, defined);
	if (!defined1) *defined = FALSE;
	if (*defined) {
	    if (*defined)
		return evalConstBinaryExpr(e -> re_syl.re_op, op1, op2);
	}
	return 0;
    }
}

union reloc_expr *
simpLoadSym(e, file)
     union reloc_expr *e;
     unsigned int file;
     /* Given e in file with index file consisting of a symbol reference to s,
      * return expression for definition of s.  May change *e.  */
{
    struct nlist *sym;
    unsigned int realFile;
    unsigned int type;

    realSym(fileDefns[file] + e -> re_syl.re_arg, file, &sym, &realFile);

    type = sym -> n_type & N_TYPE;
    switch (type) {
    case N_UNDF:
    case N_COMM:
    case N_SCOMM:
    case N_EXPR:
	return e;
    case N_ABS:
	EO_SET_SYL2(*e, EO_INT, 0);
	e[1].re_value += sym -> n_value;
	return e;
    case N_TEXT:
    case N_DATA:
    case N_SDATA:
    case N_BSS:
    case N_SBSS:
	EO_SET_SYL2(*e,
		    type == N_TEXT ? EO_TEXT :
		    type == N_DATA ? EO_DATA :
		    type == N_BSS ? EO_BSS :
		    type == N_SDATA ? EO_SDATA :
		    /* else */   EO_SBSS,
		    0);
	e[1].re_value += sym -> n_value;
	return e;
    default:
	FormatError;     /*NOTREACHED*/
    }
}

union reloc_expr *
simpLoadExpr(e, file)
     union reloc_expr *e;
     unsigned int file;
     /* Simplify the expression e from object file with index file in place,
      * returning the address of the result.
      */
{
    union reloc_expr *simp1, *simp2;	/* Simplified operands. */
    int len1;				/* Length of first operand */
    unsigned int op;			/* Operator */
    
         /* Simplify operands, check for base cases */
    op = e -> re_syl.re_op;
    switch(op) {
    case EO_INT:
    case EO_TEXT:
    case EO_DATA:
    case EO_BSS:
    case EO_SDATA:
    case EO_SBSS:
	return e;
    case EO_SYM:
	return simpLoadSym(e, file);
    case EO_MINUS:
    case EO_COMP:
	simp1 = simpLoadExpr(e+1, file);
	if (simp1 -> re_syl.re_op == EO_INT) {
	    simp1[1].re_value = evalConstUnaryExpr(op, simp1[1].re_value);
	    return simp1;
	}
	simp2 = NULL;
	break;
    default:	/* Binary operator */
	len1 = exprLength(e+1);
	simp1 = simpLoadExpr(e+1, file);
	simp2 = simpLoadExpr(e+len1+1, file);
	if (simp1 -> re_syl.re_op == EO_INT && simp2 -> re_syl.re_op == EO_INT) {
	    simp1[1].re_value = 
		evalConstBinaryExpr(op,simp1[1].re_value, simp2[1].re_value);
	    return simp1;
	}
    }

    return simpCanonLoadExpr(e, simp1, simp2);
}

/* Mini-pattern matching facility */

/* Patterns are sequences of words, interpreted as subsets of the set
 * containing the EO_xxx operators, DONT_CARE, EQ_P0, SET_P0, ZERO,
 * ONE, and STOP.  An expression syllable matches such a word if the
 * word contains DONT_CARE, if the operator part of the syllable is
 * one of the EO_xxx operators in the set, if register P0 is equal to
 * the syllable and EQ_P0 is in the set, if the syllable is 0 and ZERO
 * is in the set, or if the syllable is 1 and ONE is in the set.  If
 * SET_P0 is in the set, then P0 is set to the current syllable.  If
 * STOP is in the set, then pattern matching ends with this word.
 */

#define DONT_CARE    0x80000000
#define EQ_P0        0x40000000
#define SET_P0       0x20000000
#define STOP      0x10000000
#define ZERO	     0x08000000
#define ONE	     0x04000000
#define OP	     1 <<

#define SYM_OR_SEG  OP EO_TEXT | OP EO_DATA | OP EO_BSS | OP EO_SYM \
                    | OP EO_SDATA | OP EO_SBSS
#define PLUS_MINUS  OP EO_PLUS | OP EO_MINUS

static unsigned int simpPatterns[][12] =
    {  {OP EO_PLUS | OP EO_MULT | STOP,   STOP,  OP EO_INT | STOP},
	   /* 1: any +* const */
       {OP EO_SUB | STOP,   DONT_CARE | STOP,   OP EO_INT | STOP},
	   /* 2: any - const */
       {OP EO_PLUS | STOP,  OP EO_INT | STOP,  SYM_OR_SEG | STOP},
	   /* 3: const + sym */
       {OP EO_SUB | STOP,  SYM_OR_SEG | SET_P0 | STOP,  EQ_P0 | STOP},
	   /* 4: sym - same */
       {OP EO_MULT | STOP, OP EO_INT, ZERO | STOP, DONT_CARE | STOP},
	   /* 5: 0 * any */
       {OP EO_PLUS | STOP, OP EO_INT, ZERO | STOP, DONT_CARE | STOP},	
	   /* 6: 0 + any */
       {OP EO_MULT | STOP, OP EO_INT, ONE | STOP, DONT_CARE | STOP},
	   /* 7: 1 * any */
       {OP EO_SUB | STOP, OP EO_INT, ZERO | STOP, DONT_CARE | STOP},
	   /* 8: 0 - any */
       {OP EO_MINUS | STOP, OP EO_MINUS | STOP, DONT_CARE | STOP},
	   /* 9: - -any */
       {OP EO_PLUS | STOP, DONT_CARE | STOP, EO_MINUS | STOP},
	   /* 10: x + -y */
       {OP EO_PLUS | STOP, EO_MINUS | STOP, DONT_CARE | STOP},
	   /* 11: -x + y */
       {OP EO_SUB | STOP, DONT_CARE | STOP, EO_MINUS | STOP},
	   /* 12: x - -y */
       {STOP} };
	             
union reloc_expr *
simpCanonLoadExpr(e, opnd1, opnd2)
     union reloc_expr *e, *opnd1, *opnd2;
     /* Given unary or binary expression with operator in e and
      * operands in opnd1 and opnd2, return simplified expression.
      * Assume opnd1 and opnd2 are simplified and compressed
      * expressions (results are also).  Modifies original
      * expressions. It is assumed that the operands are not both
      * constant. */
{
    int npat;
    unsigned int P0;
    
    for (npat = 0; simpPatterns[npat][0] != STOP; npat++) {
	unsigned int *pat;
	pat = simpPatterns[npat];
	if (matchPhrase(e, &pat, &P0) && matchPhrase(opnd1, &pat, &P0) && 
	    matchPhrase(opnd2, &pat, &P0)) {
	    return reduceExpr(npat+1, e, opnd1, opnd2);
	}
    }
    return compressExpr(e, opnd1, opnd2);
}

int
matchPhrase(e, pat, P0)
     union reloc_expr *e;
     unsigned int **pat, *P0;
     /* Return true iff e matches *pat, with *P0 as P register.  Sets *pat
      * to after end of pattern on success.  May set *P0. */
{
    unsigned int cpat;

    for (cpat = **pat; ; (*pat)++, cpat = **pat, e++) {
	if (cpat & SET_P0) *P0 = (*e).re_value;
	if (! (cpat & DONT_CARE ||
	       cpat & EQ_P0 && e -> re_value == *P0 ||
	       cpat & ZERO  && e -> re_value == 0 ||
	       cpat & ONE && e -> re_value == 1 ||
	       e -> re_syl.re_op <= EO_SUB && cpat & 1 << e -> re_syl.re_op)) {
	    return FALSE;
	}
	if (cpat & STOP) {
	    (*pat)++; 
	    return TRUE;
	}
    }
}
	
union reloc_expr *
reduceExpr(n,  e, opnd1, opnd2)
     int n;
     union reloc_expr *e, *opnd1, *opnd2;
     /* Destructively apply reduction n to operation with operator at
      * e, and operands opnd1, opnd2.  Return result. */
{
    switch (n) {
    case 1: 	/* x +* const -> const +* x */
	e = simpCanonLoadExpr(e, opnd2, opnd1);
	return e;
    case 2:     /* x - const -> -const + x */
	EO_SET_SYL2(*e, EO_PLUS, 0);
	opnd2[1].re_value = -opnd2[1].re_value;
	e = simpCanonLoadExpr(e, opnd2, opnd1);
	return e;
    case 3: 	/* const + sym -> sym (offset folded in) */
	opnd2[1].re_value += opnd1[2].re_value;
	return opnd2;
    case 4: 	/* sym - same sym -> difference of offsets */
	EO_SET_SYL2(*opnd2, EO_INT, 0);
	opnd1[1].re_value -= opnd2[1].re_value;
	return opnd2;
    case 5:	/* 0 * any -> 0 */
	return opnd1;
    case 6:	/* 0 + x -> x */
    case 7:	/* 1 * x -> x */
	return opnd2;
    case 8:	/* 0 - x -> -x */
	EO_SET_SYL2(*e, EO_MINUS, 0);
	return simpCanonLoadExpr(e, opnd2, opnd2);
    case 9:	/* - - x -> x */
	return opnd1+1;
    case 10:	/* x + -y -> x-y */
	EO_SET_SYL2(*e, EO_SUB, 0);
	return simpCanonLoadExpr(e, opnd1, opnd2+1);
    case 11:    /* -x + y -> y-x */
	EO_SET_SYL2(*e, EO_SUB, 0);
	return simpCanonLoadExpr(e, opnd2, opnd1+1);
    case 12:    /* x - -y -> x+y */
	EO_SET_SYL2(*e, EO_PLUS, 0);
	return simpCanonLoadExpr(e, opnd1, opnd2+1);
    default:
	InternalError;      /*NOTREACHED*/
    }
}

union reloc_expr *
compressExpr(e, opnd1, opnd2)
     union reloc_expr *e, *opnd1, *opnd2;
     /* Return expression with operator from e and operands opnd1 and
      * opnd2, compressed to contiguous storage in place.  It is
      * assumed that all space from the first byte occupied by the
      * arguments to the last is available, and that e is first. */
{
    if (e -> re_syl.re_op == EO_MINUS || e -> re_syl.re_op == EO_COMP) {
	(void) bcopy((char *) opnd1, (char *) (e + 1), exprLength(opnd1));
	return e;
    }
    else {
	union reloc_expr *temp[MAXEXPRSYLS];
	int len1 = exprLength(opnd1);
	int len2 = exprLength(opnd2);

	if (len2 > MAXEXPRSYLS) {
	    ErrorMsg  "Expression too long." EndMsg;
	    exit(1);
	    /*NOTREACHED*/
	}
	bcopy((char *) opnd2, (char *) temp, len2*sizeof(union reloc_expr));
	bcopy((char *) opnd1, (char *) (e+1), len1*sizeof(union reloc_expr));
	bcopy((char *) temp, (char *) (e+len1+1), 
	      len2*sizeof(union reloc_expr));
	return e;
    }
}

		/* Relocation and symbol evaluation */

unsigned char *
relocSegmentToPntr(segment, file)
     unsigned int segment, file;
     /* Return pointer to given segment (an r_segment, indicated by RP_TEXT0, 
      * etc.) from given file, so that adding a byte offset gives address of 
      * quantity to relocate.  (In the case that segment = RP_SYMBOLS, the 
      * offset of the n_value field in the record is added in.) */
{
    switch (segment) {
    case RP_TEXT0:
	return (unsigned char *) fileText[file];
    case RP_DATA0:
	return (unsigned char *) fileData[file];
    case RP_SDATA0:
	return (unsigned char *) fileSData[file];
    case RP_SYMBOLS:
	return (unsigned char *) &(fileDefns[file] -> n_value);
    default:
	{
	    struct nlist *sym = fileDefns[file] + segment - RP_SEG0;
	    int stype = sym -> n_type & N_TYPE;
	    if (stype == N_TEXT) 
		return  ((unsigned char *) fileText[file]) 
		      + sym -> n_value - (int) fileTextAddr[file];
	    else if (stype == N_DATA)
		return  ((unsigned char *) fileData[file]) 
		      + sym -> n_value - (int) fileDataAddr[file];
	    else if (stype == N_SDATA)
		return ((unsigned char *) fileSData[file])
		      + sym -> n_value - (int) fileSDataAddr[file];
	    else {
		FormatError;     /*NOTREACHED*/
	    }
	}
    }
}

bool
valueFits(V, L)
     unsigned int V,L;
     /* Return true iff V, interpreted as a signed 32-bit quantity, is
      * acceptable for a signed field of size indicated by L (one of
      * RP_LOW8, ...).
      */
{
    /* The following looks like an odd test, but V is an
     * unsigned quantity that will be interpreted as signed.
     */
    return (V <= maxMap[L] || V >= minMap[L]);
}

void
fitError(file, segment, offset, value, length)
     unsigned int file, segment, offset, value, length;
{
    char segmentNameBuffer[20];
    char *segmentId;

    switch (segment) {
    case RP_TEXT0:
	segmentId = "text segment";
	break;
    case RP_DATA0:
	segmentId = "private data segment";
	break;
    case RP_SDATA0:
	segmentId = "shared data segment";
	break;
    default:
	segmentId = segmentNameBuffer;
	(void) strcpy(segmentNameBuffer, "extra segment 0");
	segmentNameBuffer[strlen(segmentNameBuffer)-1] += segment - RP_SEG0;
	break;
    }	

    ErrorMsg "%s: Value 0x%X does not fit in %d-bit field\n\tat offset 0x%X in %s.",
	     fileName[file], value, lengthMap[length], offset, segmentId
    EndMsg;
}

void
insertVal(file, segment, offset, length, val)
     unsigned int file, segment, offset;
     unsigned int length, val;
     /* Insert val in bits indicated by length starting at offset from
      * given segment in given object file.  Length is RP_LOW8, etc.
      * Address to be modified must be divisible by 4, except for
      * RP_LOW8 and RP_LOW16, and must be divisible by 2 for RP_LOW16.
      * Produces warning messages if val is too large for field; val
      * is usually assumed to be signed for this purpose.  However,
      * RP_LOW28 is a special case.  There, val is treated as an
      * intra- segment word address, and is checked for agreement with
      * the eventual load-time address of the modified memory.
      */
{
    unsigned int
	byte = offset & 0x3,
	*word = 
	    (unsigned int *)
	          (relocSegmentToPntr(segment, file) + (offset & ~0x3)),
	bit = byte*8,
        realLoc = segmentOffsetToNewAddr(segment,file,offset);
    
    if (length == RP_LOW28) {
	if (((realLoc>>2 ^ val) & 0xf0000000) != 0) {
	    ErrorMsg "Intersegment call or jump at %X.", realLoc EndMsg;
	}
	val &= 0x0fffffff;
    }
    else
	if (!valueFits(val, length))
	    fitError(file, segment, offset, val, length);
    switch (length) {
    case RP_LOW8:
	*word = *word & (~(0xff << bit)) | (val & 0xff) << bit;
	break;
    case RP_LOW16:
	if (byte & 0x1) {
	    FormatError;     /*NOTREACHED*/
	}
	*word = *word & (~(0xffff << bit)) | (val & 0xffff) << bit;
	break;
    case RP_LOW9:
    case RP_LOW14:
    case RP_LOW28:
	if (byte & 0x3) {
	    FormatError;     /*NOTREACHED*/
	}
	*word =   *word & ~lengthBitMask[length] | val & lengthBitMask[length];
	break;
    case RP_SCONS:
	if (byte & 0x3) {
	    FormatError;     /*NOTREACHED*/
	}
	*word =   *word & ~SCONSMASK | val & SCONSLOWMASK
	        | val << (SCONSHIGHPOSN-SCONSLOWLEN) & SCONSHIGHMASK;
	break;
    case RP_32:
	*word = val;
	break;
    default:
	FormatError;     /*NOTREACHED*/
    }
}

unsigned int
extractVal(file, segment, offset, length)
     unsigned int file, segment, offset, length;
     /* Extract value in bits indicated by length starting at given
      * offset in given segment of given object file. Length is
      * RP_LOW8, etc.  Address must be divisible by 4. */
{
    unsigned int *word = 
	(unsigned int *) (relocSegmentToPntr(segment, file) + (offset & ~0x3));
    unsigned int byte = offset & 0x3;
    unsigned int bit = 8*byte;

    switch (length) {
    case RP_LOW8:
        return (*word & 0xff << bit) >> bit;
    case RP_LOW16:
	if (byte & 0x1) {
	    FormatError;     /*NOTREACHED*/
	}
	return (*word  &  0xffff << bit) >> bit;
    case RP_LOW9:
    case RP_LOW14:
    case RP_LOW28:
	if (byte & 0x3) {
	    FormatError;     /*NOTREACHED*/
	}
	return *word & lengthBitMask[length];
    case RP_SCONS:
	if (byte & 0x3) {
	    FormatError;     /*NOTREACHED*/
	}
	return *word & SCONSLOWMASK |
		(*word & SCONSHIGHMASK) >> (SCONSHIGHPOSN-SCONSLOWLEN);
    case RP_32:
	return *word;
    default:
	FormatError;     /*NOTREACHED*/
    }
}


void
setLoc(segment, offset, file, length, wordp, val)
     unsigned int segment, offset, file, length, wordp, val;
     /* Set data at given offset from given segment (RP_TEXT0, etc.) in given 
      * file to val.  Length (RP_LOW8, etc.) indicates bits to change.  
      * Wordp is 1 if val is to be divided by word length, else 0. 
      * length == RP_LOW28 is a special case: val is first checked to
      * make sure it is in the same segment as the modified address,
      * and modified to make it legal.
      */
{
    if (wordp == 1) 
	val =
	    (length == RP_LOW28) 
	    ? val >> 2 
	    : (unsigned int) (((int) val) >> 2);

    insertVal(file, segment, offset, length, val);
}

void
incrLoc(segment, offset, file, length, wordp, val)
     unsigned int segment, offset, file, length, wordp, val;
     /* As for setLoc above, but increment the location. */
{
    if (wordp == 1) 
	val =
	    (length == RP_LOW28) 
	    ? val >> 2 
	    : (unsigned int) (((int) val) >> 2);

    insertVal(file, segment, offset, length,
	      val + extractVal(file, segment, offset, length));
}

bool
evalReloc(reloc, file, keep)
     struct relocation_info *reloc;
     unsigned int file;
     bool keep;
     /* Check relocation item reloc from file number file.  If possible, 
      * perform indicated relocation, and set r_extra to indicate no further 
      * work needed.  If keep, then set reloc and its attendant expression (if
      * any) to most simplified form possible.  Set r_extra bits to RELOC_DONE
      * if relocation item should be kept and no further work is needed, 
      * RELOC_EXCLUDE if relocation item is no longer needed, and 0 otherwise.
      * Returns FALSE if no changes made to symbol table, otherwise TRUE.
      */
{
    unsigned int targetSeg, targetAddr, targetLength, targetWord;
    int symChanged;

    if (reloc -> r_extra != 0) return FALSE;
    targetSeg =     reloc -> r_segment;
    targetAddr =    reloc -> r_address;
    targetLength =  reloc -> r_length;
    targetWord =    reloc -> r_word;
    symChanged =    targetSeg == RP_SYMBOLS;  /* Initial guess */

    switch (reloc -> r_reltype) {
    case RP_RSEG:
	incrLoc(targetSeg, targetAddr, file, targetLength, targetWord,
		fileDelta[file][reloc -> r_expr]);
	reloc -> r_extra = RELOC_DONE;
	break;
    case RP_RSYM:
	{
	    unsigned int realFile;
	    struct nlist *sym;

	    realSym(fileDefns[file] + reloc -> r_expr, file, &sym, &realFile);

	    switch (sym -> n_type & N_TYPE) {
	    case N_UNDF:
	    case N_COMM:
	    case N_SCOMM:
		symChanged = FALSE;
		break;
	    case N_ABS:
		incrLoc(targetSeg, targetAddr, file, targetLength, targetWord,
			(unsigned int) sym -> n_value);
		reloc -> r_extra = RELOC_EXCLUDE;
		break;
	    case N_TEXT:
	    case N_DATA:
	    case N_BSS:
	    case N_SDATA:
	    case N_SBSS:
		reloc -> r_expr = sym -> n_type & N_TYPE;
		incrLoc(targetSeg, targetAddr, file, targetLength, targetWord,
			(unsigned int) sym -> n_value);
		reloc -> r_reltype = RP_RSEG;
		reloc -> r_extra = RELOC_DONE;
		break;
	    case N_EXPR:
		incrLoc(targetSeg, targetAddr, file, targetLength, targetWord,
			(unsigned int) sym -> n_value);
		reloc -> r_extra = RELOC_DONE;
		break;
	    default:
		FormatError;     /*NOTREACHED*/
	    }
	}
	break;
    case RP_REXP:
	{
	    bool defined;
	    unsigned int v;
	    v = evalLoadExpr(fileRelocExprs[file] + 
			        (reloc -> r_expr / sizeof(union reloc_expr)),
			     file, &defined);
	    if (defined) {
		setLoc(targetSeg, targetAddr, file, targetLength, targetWord,
		       v);
		reloc -> r_extra = RELOC_DONE;
		if (keep) {
		    union reloc_expr *e;
		    int op;

		    e = simpLoadExpr(BINCR(fileRelocExprs[file], 
					   reloc -> r_expr, union reloc_expr),
				     file);
		    reloc -> r_expr = BDIFF(e, fileRelocExprs[file]);

		    op = e -> re_syl.re_op;
		    switch (op) {
		    case EO_INT:
			reloc -> r_extra = RELOC_EXCLUDE;
			break;
		    case EO_TEXT:
		    case EO_DATA:
		    case EO_BSS:
		    case EO_SDATA:
		    case EO_SBSS:
			reloc -> r_reltype = RP_RSEG;
			reloc -> r_expr = 
			    op == EO_TEXT ? N_TEXT :
			    op == EO_DATA ? N_DATA :
			    op == EO_BSS ? N_BSS :
			    op == EO_SDATA ? N_SDATA :
			    /* else */      N_SBSS;
			break;
		    default:
			break;
		    }
		}
	    }
	    else symChanged = FALSE;
	}
	break;
    default:
	FormatError;     /*NOTREACHED*/
    }
    if (symChanged) {
	struct nlist *sym = 
	    (struct nlist *) (((char *) fileDefns[file]) + reloc -> r_address);
	int newtype;
	if (sym -> n_type & N_STAB)
	    symChanged = FALSE;
	if (reloc -> r_extra == RELOC_EXCLUDE) {
	    if (sym -> n_type & N_STAB) {
		newtype = N_UNDF;	/* Is this needed ? */
	    }
	    else {
		newtype = N_ABS;
	    }
	}
	else if (reloc -> r_reltype == RP_RSEG) {
	    reloc -> r_extra = RELOC_EXCLUDE;
	    newtype = reloc -> r_expr;
	}
	else {
	    newtype = N_EXPR;
	}
	sym -> n_type = (sym -> n_type & ~N_TYPE) | newtype;
    }
    if (reloc -> r_extra == RELOC_DONE && !keep) 
	reloc -> r_extra = RELOC_EXCLUDE;
    return symChanged;
}
	
void
relocateAll()
     /* Process all relocation information, and simplify all expressions as 
      * much as possible if keep.  Assumes all symbols are interned. 
      * Resolved relocations are marked RELOC_EXCLUDE if they are 
      * unnecessary. Common is allocated, if switches indicate. */
{
    int numUndefExprs;		/* Expression-defined symbols of type N_UNDF 
				 * in current pass. */
    int moreToDo;		/* False as long as no changes are made to 
				 * symbol table in current pass. */
    unsigned int file;

    relocateSymbols();
    if (!rSwitch || dSwitch) 
	resolveCommon();
    if (!rSwitch && baseFileName == NULL) 
	computeEnds();
    relocateExprs();

    do {
	moreToDo = FALSE;
	numUndefExprs = 0;

	for (file = 0; file < numFiles; file++) {

	    struct relocation_info
		*reloc,
		*last =  (struct relocation_info *) 
		              ((char *) fileRelocs[file] + 
			       fileHeader[file].a_rsize);

	    for (reloc = fileRelocs[file]; reloc != last; reloc++) {
		if (evalReloc(reloc, file, rSwitch) && numUndefExprs > 0)
		    moreToDo = TRUE;
		if (reloc -> r_extra != 0) numUndefExprs++;
	    }
	}
    } while (moreToDo);
}    

			/* Reading object files */

int
readOpenObjectFile(fd, size, name)
     int fd,size;
     char *name;
     /* Read object file of size bytes from file with descriptor fd as 
      * next file.  Return 0 if successful, and non-zero otherwise. 
      * Name is the file name for error messages. */
{
    char *contents;
    
    if (read(fd, (char *) &fileHeader[numFiles], sizeof(struct exec)) != 
	    sizeof(struct exec)) {
	ErrorMsg "Unexpected EOF on file %s.", name EndMsg;
	return 1;
    }
    if (N_BADMAG(fileHeader[numFiles])) {
	ErrorMsg "Bad magic number on file %s.", name EndMsg;
	return 1;
    }
    else if (fileHeader[numFiles].a_magic != OMAGIC) {
	ErrorMsg "File %s cannot be relinked.", name 
		EndMsg;
	return 1;
    }
    
    contents = malloc((unsigned int) size - sizeof(struct exec));
    if (contents == NULL) {
	InsufficientSpace;
    }
    if (read(fd, contents, size - sizeof(struct exec)) != 
	    size - sizeof(struct exec)) {
	ErrorMsg "Could not read file %s.", name EndMsg;
	return 1;
    }

    contents -= sizeof(struct exec);    /* Compensate for missing header. */

    fileText[numFiles] =	/* Works only for OMAGIC input. */
	(unsigned int *) (contents + N_TXTOFF(fileHeader[numFiles]));
    fileData[numFiles] =
	(unsigned int *) (contents + N_DATAOFF(fileHeader[numFiles]));
    fileSData[numFiles] = 
	(unsigned int *) (contents + N_SDATAOFF(fileHeader[numFiles]));
    fileDefns[numFiles] = 
	(struct nlist *) (contents + N_SYMOFF(fileHeader[numFiles]));
    fileRelocs[numFiles] = 
	(struct relocation_info *) 
	    (contents + N_RELOCOFF(fileHeader[numFiles]));
    fileRelocExprs[numFiles] = 
	(union reloc_expr *) (contents + N_RELOCOFF(fileHeader[numFiles]) + 
			       fileHeader[numFiles].a_rsize);
    fileStrings[numFiles] = contents + N_STROFF(fileHeader[numFiles]);

    return 0;
}
			/* Library Support */

bool
isLibraryFile(fd)
     int fd;
     /* Given that fd is a file descriptor positioned at the beginning of
      * a file, returns TRUE iff fd appears to be the descriptor of a
      * library file.  Positions fd to beginning.
      */
{
    char arMagic[SARMAG+1];
    int len;

    arMagic[SARMAG] = '\0';
    len = read(fd, arMagic, SARMAG);
    if (lseek(fd, (long) 0, L_SET) == -1) return FALSE;
    return (len == SARMAG && strcmp(arMagic, ARMAG) == 0);
}
    
int
searchLibList(name, dirList)
     char *name;
     char *dirList[];
     /* Search for library with given name in directories dirList
      * (which is terminated by a NULL).  Return file descriptor for open 
      * library, or -1.  */
{
    char **dir;
    int fd;
    
    for (dir = dirList, fd = -1; *dir != NULL && fd == -1; dir++) {
	char fileName[MAXFILENAMESIZE+1];
	int len;

	fileName[MAXFILENAMESIZE] = '\0';
	(void) strncpy(fileName, *dir, MAXFILENAMESIZE+1);
	len = strlen(fileName);
	if (fileName[len-1] != '/') {
	    (void) strncpy(fileName+len, "/", MAXFILENAMESIZE-len+1);
	    len ++;
	}
	(void) strncpy(fileName+len, "lib", MAXFILENAMESIZE-len+1);
	len += 3;
	(void) strncpy(fileName + len, name, MAXFILENAMESIZE-len+1);
	len += strlen(name);
	(void) strncpy(fileName + len, ".a", MAXFILENAMESIZE-len+1);

	fd = open(fileName, O_RDONLY);
    }

    return fd;
}
	    
int
openLibrary(name)
     char *name;
     /* Search for library file with given name.  Search for it in 
      * directories listed in -L options in dirList and in the standard 
      * library directories. Return fd of opened library file, or -1 if no 
      * openable file found. */
{
    int fd;

    fd = searchLibList(name, dirList);
    if (fd == -1)
	return searchLibList(name, standardLibs);
    else return fd;
}

void
resolveStrings(symTab, n, strings)
     struct ranlib symTab[];
     int n;
     char *strings;
     /* Change all ran_un entries in symTab[0..n-1] from indices into
      * strings to pointers into strings. */
{
    int i;

    for (i = 0; i < n; i++)
	symTab[i].ran_un.ran_name = strings + symTab[i].ran_un.ran_strx;
}

int
ranlibCompare(x,y)
     struct ranlib *x,*y;
     /* Returns 0 if name of *x == that of *y, -1 if *x's is less, 1 otherwise.
      */
{
    return strcmp(x -> ran_un.ran_name, y -> ran_un.ran_name);
}

int
indirectStrcmp(x,y)
     char **x,**y;
{
    return strcmp(*x, *y);
}

int
findResolvingFile(symTab, numSyms, name, file)
     struct ranlib symTab[];
     int numSyms;
     char **name;
     int *file;
     /* Find a symbol in symTab[0..numSyms-1] that is unresolved and return the
      * offset of the defining object file recorded for that symbol in symTab.
      * The entries in symTab must be sorted alphabetically, and must use 
      * the ran_name fields to point directly at strings. Return 0 
      * if no file.   If file found, set *name to point to the name of 
      * a symbol that caused the returned file to be found, and *file to
      * the index of an object file that references that name. */
{
    globalSymType *ref;
    struct ranlib *fileRef;

    for (ref = unresolvedSyms.nextUnresolved, fileRef = NULL; 
	 fileRef == NULL && ref != &unresolvedSyms;
	 ref = ref -> nextUnresolved) {

	    struct ranlib target;
	    target.ran_un.ran_name = *name = ref -> name;
	    *file = ref -> file;
	    fileRef = 
		(struct ranlib *) bsearch((char *) &target, (char *) symTab, 
					  (unsigned) numSyms, 
					  sizeof(struct ranlib),
					  ranlibCompare);
	}
    if (fileRef == NULL) return 0;
    else return fileRef -> ran_off;
}

void
readOpenLibraryFile(fd, name)
     int fd;
     char *name;
     /* Read library from file with descriptor fd as object files.
      * Name is the name of the library for error messages.  */
{
    struct ar_hdr SYMDEFHead;
    int symBytes, numObjs;
    struct ranlib *extSyms = NULL;
    int stringSpaceSize;
    char *stringSpace = NULL;
    
    if (lseek(fd, (long) SARMAG, L_SET) == -1 ||
	read(fd, (char *) &SYMDEFHead, sizeof(struct ar_hdr))
          != sizeof(struct ar_hdr) ||
	strncmp(SYMDEFHead.ar_fmag, ARFMAG, sizeof(SYMDEFHead.ar_fmag)) != 0 ||
	strncmp(SYMDEFHead.ar_name, "__.SYMDEF ", 10) != 0 ||
	read(fd, (char *) &symBytes, sizeof(symBytes)) != sizeof(symBytes))
	    goto BadFormat;

    extSyms = (struct ranlib *) malloc((unsigned int) symBytes);
    if (extSyms == NULL) {
	InsufficientSpace
    }
    if (read(fd, (char *) extSyms, symBytes) != symBytes)
	goto BadFormat;
    numObjs = symBytes / sizeof(struct ranlib);

    if (read(fd, (char *) &stringSpaceSize, sizeof(stringSpaceSize)) != 
	sizeof(stringSpaceSize))
	    goto BadFormat;
    stringSpace = malloc((unsigned int) stringSpaceSize);
    if (stringSpace == NULL) {
	InsufficientSpace;
    }
    if (read(fd, stringSpace, stringSpaceSize) != stringSpaceSize) 
	goto BadFormat;

    resolveStrings(extSyms, numObjs, stringSpace);
    qsort((char *) extSyms, numObjs, sizeof(struct ranlib), ranlibCompare);

    do {
	char *symName;
	int symFile;
	int fileOffset = 
	    findResolvingFile(extSyms, numObjs, &symName, &symFile);
	struct ar_hdr objHeader;
	int size;

	if (fileOffset == 0) goto Finish;

	if ((int) lseek(fd, (long) fileOffset, L_SET) == -1 ||
	    read(fd, (char *) &objHeader, sizeof(objHeader)) != 
	        sizeof(objHeader) ||
	    strncmp(objHeader.ar_fmag, ARFMAG, sizeof(objHeader.ar_fmag)) != 0)
	        goto BadFormat;
	(void) sscanf(objHeader.ar_size, "%d", &size);

	fileName[numFiles] = malloc(sizeof(objHeader.ar_name)+1);
	(void) strncpy(fileName[numFiles], objHeader.ar_name, 
		       sizeof(objHeader.ar_name));
	{
	    char *s = fileName[numFiles] + sizeof(objHeader.ar_name) - 1;
	    while (s != fileName[numFiles] && *s == ' ')
		s -= 1;
	    s[1] = '\0';
	}

	if (wSwitch) {
	    (void) printf("Library file %s loaded for %s referenced in %s.\n",
		   fileName[numFiles], symName, fileName[symFile]);
	}

	if (readOpenObjectFile(fd, size, name) != 0)
	    goto BadFormat;
	convertStringIndices(numFiles);
	internAllSyms(numFiles);
	numFiles += 1;
    } while (TRUE);

 BadFormat:
    ErrorMsg "Bad format on library file %s.", name EndMsg;

 Finish:
    (void) close(fd);
    if (extSyms != NULL) (void) free((char *) extSyms);
    if (stringSpace != NULL) (void) free(stringSpace);
    return;
}

void
setupForcedSyms()
     /* Set up a dummy file for symbols from -u options */
{
    (void) bzero((char *) &fileHeader[numFiles], sizeof (struct exec));
    fileHeader[numFiles].a_magic = OMAGIC;
    fileHeader[numFiles].a_syms = sizeof(struct nlist) * numForcedSyms;

    fileText[numFiles] = fileData[numFiles] = fileSData[numFiles] = NULL;
    fileRelocs[numFiles] = NULL;
    fileStrings[numFiles] = NULL;
    fileRelocExprs[numFiles] = NULL;
    fileName[numFiles] = NULL;
    fileDefns[numFiles] = forcedSyms;

    internAllSyms(numFiles);

    numFiles += 1;
}

void
setupBaseFile()
     /* "Load" a dummy file corresponding to the base file named in -A
      * option. */
{
    int fd = open(baseFileName, O_RDONLY);
    unsigned int size;	/* Size of base file */

    if (fd == -1) {
	ErrorMsg "Failed to open base file %s.", baseFileName EndMsg;
	exit(1);
    }

    { 
	struct stat stats;
	(void) fstat(fd, &stats);
	size = stats.st_size;
    }

    if (read(fd, (char *) &fileHeader[numFiles], sizeof(struct exec)) !=
	sizeof(struct exec)) {
	ErrorMsg "Unexpected EOF on file %s.", baseFileName EndMsg;
	exit(1);
    }
    if (fileHeader[numFiles].a_magic != ZMAGIC) {
	ErrorMsg "Base file has improper magic number." EndMsg;
	exit(1);
    }
    fileName[numFiles] = baseFileName;
    fileDefns[numFiles] = 
	(struct nlist *) malloc(size - (int) N_SYMOFF(fileHeader[numFiles]));
    if (fileDefns[numFiles] == NULL) {
	InsufficientSpace;
    }
    if (lseek(fd, (long) N_SYMOFF(fileHeader[numFiles]), L_SET) == -1
	|| read(fd, (char *) fileDefns[numFiles], 
		size - N_SYMOFF(fileHeader[numFiles]) 
		   != size - N_SYMOFF(fileHeader[numFiles]))) {

	ErrorMsg "Could not read symbols and strings of file %s.", baseFileName
	    EndMsg;
	exit(1);
    }
    (void) close(fd);

    fileStrings[numFiles] = 
	(char *) fileDefns[numFiles]
	    + N_STROFF(fileHeader[numFiles]) - N_SYMOFF(fileHeader[numFiles]);
    convertStringIndices(numFiles);
    internAllSyms(numFiles);

    if (!TtextSwitch) {
	globalSymType **rep = findSymbolEntry("_end");
	
	if (*rep != NULL && (*rep) -> sym -> n_type == (N_BSS | N_EXT)) 
	    textStart = (*rep) -> sym -> n_value;
	else
	    textStart = 
		N_BSSADDR(fileHeader[numFiles]) + 
		    fileHeader[numFiles].a_bss;
    }
    
    if (!TsdataSwitch) {
	globalSymType **rep = findSymbolEntry("_end_s");
	
	TsdataSwitch = TRUE;	/* Implicit setting */
	if (*rep != NULL && (*rep) -> sym -> n_type == (N_SBSS | N_EXT))
	    sdataStart = (*rep) -> sym -> n_value;
	else 
	    sdataStart =
		N_SBSSADDR(fileHeader[numFiles]) + 
		    fileHeader[numFiles].a_sbss;
    }

    fileText[numFiles] = fileData[numFiles] = fileSData[numFiles] = NULL;
    fileRelocs[numFiles] = NULL;
    fileRelocExprs[numFiles] = NULL;
    (void) bzero((char *) &fileHeader[numFiles], sizeof(struct exec));

    numFiles += 1;
}

			/* Computing relocated addresses */

void
computeRelocations()
     /* Set fileTextAddr, fileDataAddr, fileBssAddr, fileSDataAddr,
      * fileSBssAddr, and fileDelta from fileHeader, given the
      * output file's desired magic number and the desired starting address
      * of the first text segment.  */
{
    unsigned int nextAddr;
    int i;
    
    if (numFiles == 0) return;

    nextAddr = textStart;
    if (!ptSwitch) {
	for (i = 0; i < numFiles; i++) {
	    fileTextAddr[i] = nextAddr;
	    fileDelta[i][N_TEXT] = nextAddr - N_DEFENT(fileHeader[i]);
	    nextAddr += REAL_TEXT_LENGTH(fileHeader[i]);
	}
    }

    if (!pdSwitch) {
	if (TdataSwitch) {
	    nextAddr = dataStart;
	}
	else {
	    nextAddr = DATA_ADDR(nextAddr, magic);
	}
	
	for (i = 0; i < numFiles; i++) {
	    fileDataAddr[i] = nextAddr;
	    fileDelta[i][N_DATA] = nextAddr - N_DATADDR(fileHeader[i]);
	    nextAddr += fileHeader[i].a_data;
	}
	
	for (i = 0; i < numFiles; i++) {
	    fileBssAddr[i] = nextAddr;
	    fileDelta[i][N_BSS] = nextAddr - N_BSSADDR(fileHeader[i]);
	    nextAddr += fileHeader[i].a_bss;
	}
    }

    if (TsdataSwitch) {
	nextAddr = sdataStart;
    }
    else
	nextAddr = N_SDATADDR(0);

    if (ptSwitch) {
	for (i = 0; i < numFiles; i++) {
	    fileTextAddr[i] = nextAddr;
	    fileDelta[i][N_TEXT] = nextAddr - N_DEFENT(fileHeader[i]);
	    nextAddr += REAL_TEXT_LENGTH(fileHeader[i]);
	}
    }
    for (i = 0; i < numFiles; i++) {
	if (pdSwitch) {
	    fileDataAddr[i] = nextAddr;
	    fileDelta[i][N_DATA] = nextAddr - N_DATADDR(fileHeader[i]);
	    nextAddr += fileHeader[i].a_data;
	}
	fileSDataAddr[i] = nextAddr;
	fileDelta[i][N_SDATA] = nextAddr - N_SDATADDR(fileHeader[i]);
	nextAddr += fileHeader[i].a_sdata;
    }

    for (i = 0; i < numFiles; i++) {
	if (pdSwitch) {
	    fileBssAddr[i] = nextAddr;
	    fileDelta[i][N_BSS] = nextAddr - N_BSSADDR(fileHeader[i]);
	    nextAddr += fileHeader[i].a_bss;
	}
	fileSBssAddr[i] = nextAddr;
	fileDelta[i][N_SBSS] = nextAddr - N_SBSSADDR(fileHeader[i]);
	nextAddr += fileHeader[i].a_sbss;
    }
}

void
resolveCommon()
     /* Allocate uninitialized common allocated by .comm directives.
      * Change all outstanding N_COMM symbol entries to N_BSS or
      * N_SBSS global definitions or to external references to the
      * first such definition and expand bss regions of last file as
      * needed.  */
{
    unsigned int file;

    for (file = 0; file < numFiles; file++) {
	struct nlist *sym,
	             *last = BINCR(fileDefns[file],fileHeader[file].a_syms,
				   struct nlist);

	for (sym = fileDefns[file]; sym != last; sym++) {
	    int type = sym -> n_type;
	    int stype = type & N_TYPE;
	    if ((type & N_STAB) == 0
		&& (stype == N_COMM || stype == N_SCOMM)) {

		struct nlist *nSym;
		unsigned int outFile;

		realSym(sym, file, &nSym, &outFile);
		if (sym != nSym) {
		    sym -> n_type = N_UNDF | N_EXT;
		    sym -> n_value = 0;
		}
		else {
		    int len = sym -> n_value;

		    sym -> n_un.n_name = globalReferent(sym) -> name;
		    if (stype == N_COMM && !pdSwitch) {
			sym -> n_type = N_BSS | N_EXT;
			sym -> n_value = 
			    fileBssAddr[numFiles-1] + 
				fileHeader[numFiles-1].a_bss;
			fileHeader[numFiles-1].a_bss += len;
		    }
		    else {
			sym -> n_type = N_SBSS | N_EXT;
			sym -> n_value = 
			    fileSBssAddr[numFiles-1] + 
				fileHeader[numFiles-1].a_sbss;
			fileHeader[numFiles-1].a_sbss += len;
		    }			
		}
	    }
	}
    }
}
		
void
relocateSymbols()
     /* Relocate all symbol definitions using fileDelta. */
{
    unsigned int file;

    for (file = 0; file < numFiles; file++) {
	struct nlist *sym,
	             *last = BINCR(fileDefns[file],fileHeader[file].a_syms,
				   struct nlist);

	for (sym = fileDefns[file]; sym != last; sym++) {
	    switch (sym -> n_type & N_TYPE) {
	    case N_TEXT:
	    case N_DATA:
	    case N_BSS:
	    case N_SDATA:
	    case N_SBSS:
		sym -> n_value += fileDelta[file][sym -> n_type & N_TYPE];
		break;
	    default:
		break;
	    }
	}
    }
}

void
defineSpecialSymbol(name, V, T)
     char *name;
     unsigned long V;
     unsigned int T;
     /* If the global symbol name is referenced, define its value to be 
      * V and its n_type to be T.  Report an error if it is already defined. */
{
    globalSymType **rep = findSymbolEntry(name);
    struct nlist *sym;

    if (*rep == NULL) return;

    sym = (*rep) -> sym;
    if ((sym -> n_type & N_TYPE) == N_UNDF) {
	sym -> n_un.n_name = name;
	sym -> n_type = T;
	sym -> n_value = V;
	if ((*rep) -> nextUnresolved != NULL) {
	    (*rep) -> nextUnresolved -> lastUnresolved =
		(*rep) -> lastUnresolved;
	    (*rep) -> lastUnresolved -> nextUnresolved =
		(*rep) -> nextUnresolved;
	    (*rep) -> lastUnresolved = (*rep) -> nextUnresolved = NULL;
	}
    }
    else {
	ErrorMsg "Symbol %s should not be defined explicitly.", name EndMsg;
    }
}

void
computeEnds()
     /* Set values for whichever of _etext, _edata, _end, _edata_s, 
      * and _end_s are present. */
{
    int last = numFiles - 1;

    defineSpecialSymbol("_etext", 
			fileTextAddr[last]+REAL_TEXT_LENGTH(fileHeader[last]),
			N_TEXT | N_EXT);

    if (pdSwitch) {
	defineSpecialSymbol("_edata", 
			    fileSDataAddr[last] + fileHeader[last].a_sdata,
			    N_SDATA | N_EXT);
	defineSpecialSymbol("_end", 
			    fileSBssAddr[last] + fileHeader[last].a_sbss,
			    N_SBSS | N_EXT);
    }
    else {
	defineSpecialSymbol("_edata", 
			    fileDataAddr[last] + fileHeader[last].a_data,
			    N_DATA | N_EXT);
	defineSpecialSymbol("_end", 
			    fileBssAddr[last] + fileHeader[last].a_bss,
			    N_BSS | N_EXT);
    }

    defineSpecialSymbol("_edata_s", 
			fileSDataAddr[last] + fileHeader[last].a_sdata,
			N_SDATA | N_EXT);
    defineSpecialSymbol("_end_s", fileSBssAddr[last] + fileHeader[last].a_sbss,
			N_SBSS | N_EXT);
}    

void
relocateExprs()
     /* Relocate all reloc_expr syllables of types EO_TEXT, EO_DATA, and EO_BSS
      * according to fileDelta. */
{
    unsigned int file;

    for (file = 0; file < numFiles; file++) {
	struct relocation_info 
	    *reloc,
	    *last = BINCR(fileRelocs[file], fileHeader[file].a_rsize,
			  struct relocation_info);

	for (reloc = fileRelocs[file]; reloc != last; reloc++) {
	    if (reloc -> r_reltype == RP_REXP) {
		union reloc_expr *e, *laste;
		
		for (e = BINCR(fileRelocExprs[file], reloc -> r_expr, 
			       union reloc_expr),
		     laste = e + exprLength(e);
		     e != laste;
		     e++) {
		    switch (e -> re_syl.re_op) {
		    case EO_TEXT:
		    case EO_DATA:
		    case EO_BSS:
		    case EO_SDATA:
		    case EO_SBSS:
			e[1].re_value += 
			    fileDelta[file][EOtoNmap[e -> re_syl.re_op]];
			e++;
			break;
		    case EO_INT:
		    case EO_SYM:
			e++;
			break;
		    default:
			break;
		    }
		}
	    }
	}
    }
}

unsigned int
findEntry(name, textStart)
     char *name;
     unsigned int textStart;
     /* Return relocated address of symbol name, or textStart if it does not
      * exist.  
      */
{
    struct nlist sym;
    union reloc_expr e[2];
    bool defined;
    unsigned int val;

    if (name == NULL) return textStart;
    /* Dummy up a symbol and an expression to use with evalSym */
    fileDefns[numFiles] = &sym;
    sym.n_un.n_name = name;
    sym.n_type = N_UNDF | N_EXT;
    sym.n_other = sym.n_desc = sym.n_value = 0;
    internSym(&sym, numFiles);
    EO_SET_SYL2(e[0], EO_SYM, 0);
    e[1].re_value = 0;

    val = evalSym(e, numFiles, &defined);
    if (!defined) {
	ErrorMsg "Entry name %s not defined.", name EndMsg;
	return textStart;
    }
    else return val;
}

			/* Building combined object file */

void
markUsedSymbols()
     /* Set SYM_USED bits in n_others for all symbols that are used in
      * relocation data. Used after relocations are processed and
      * excluded. */
{
    unsigned int file;

    for (file = 0; file < numFiles; file++) {
	struct relocation_info 
	    *reloc,
	    *last = BINCR(fileRelocs[file], fileHeader[file].a_rsize, 
			  struct relocation_info);
	for (reloc = fileRelocs[file]; reloc != last; reloc++) {
	    if (reloc -> r_extra != RELOC_EXCLUDE) {
		if (reloc -> r_reltype == RP_RSYM) {
		    unsigned int realFile;
		    struct nlist *sym;
		    realSym(fileDefns[file] + reloc -> r_expr, file, &sym,
			    &realFile);
		    sym -> n_other |= SYM_USED;
		}
		else if (reloc -> r_reltype == RP_REXP) {
		    union reloc_expr
			*e = BINCR(fileRelocExprs[file],reloc -> 
				        r_expr,union reloc_expr),
			*last = e + exprLength(e);
		    while (e != last) {
			switch (e -> re_syl.re_op) {
			case EO_SYM:
			    {
				unsigned int realFile;
				struct nlist *sym;

				realSym(fileDefns[file] + e -> re_syl.re_arg, 
					file, &sym, &realFile);
				sym -> n_other |= SYM_USED;
				e++;
				break;
			    }
			case EO_INT:
			case EO_TEXT:
			case EO_DATA:
			case EO_BSS:
			case EO_SDATA:
			case EO_SBSS:
			    e++;
			    break;
			default:
			    break;
			}
			e++;
		    }
		}
	    }
	}
    }
}

void
countAndMarkSymbols(numSyms, stringSize)
     unsigned int *numSyms, *stringSize;
     /* Mark all symbols that that are to be excluded from the final symbol
      * table by setting the SYM_EXCLUDE flag in their n_other fields.  
      * It is assumed that symbols are interned, relocation expressions 
      * simplified or resolved, and SYM_USED bits in n_other set.   Set 
      * *numSyms to number of symbols retained in final table, and *stringSize
      * to total space needed for the string table for these symbols.
      */
{
    unsigned int file;
    
    *stringSize = *numSyms = 0;
    for (file = 0; file < numFiles; file++) {
	struct nlist *sym,
	             *last = (struct nlist *) 
			          ((char *) fileDefns[file] 
				   + fileHeader[file].a_syms);

	for (sym = fileDefns[file]; sym != last; sym++) {
	    unsigned int fullType = sym -> n_type,
	                 type = fullType & (N_TYPE | N_STAB);
	    if (type == N_UNDF || type == N_COMM || type == N_SCOMM) {
		struct nlist *Rsym;
		unsigned int temp;
		realSym(sym, file, &Rsym, &temp);
		if (sym != Rsym) {
		    sym -> n_other |= SYM_EXCLUDE;
		    continue;
		}
	    }
	    if (   !sSwitch && fullType & N_EXT && (fullType & N_STAB) == 0
		|| !sSwitch && !xSwitch && !SSwitch && fullType & N_STAB
		|| rSwitch && (fullType & N_STAB) == 0 
		   && sym -> n_other & SYM_USED
		   && (type == N_UNDF || type == N_COMM
		       || type == N_SCOMM || type == N_EXPR)
		|| !xSwitch && !sSwitch && (fullType & N_STAB) == 0
		   && (sym -> n_un.n_name)[0] != '\0' 
		   && (!XSwitch || (sym -> n_un.n_name)[0] != 'L')) {
		char *s = symName(sym);
		(*numSyms)++;
		if (s[0] != '\0')
		    *stringSize += strlen(s)+1;
	    }
	    else 
		sym -> n_other |= SYM_EXCLUDE;
	}
    }
    if (*stringSize > 0) *stringSize += 1;  /* Account for initial "" */
}

void
createNewSymbolsAndStrings(symbols, strings, symbolsSize, stringsSize, 
			   allResolved)
     struct nlist **symbols;
     char **strings;
     unsigned int *symbolsSize, *stringsSize;
     bool *allResolved;
     /* Create new symbol and string tables for output file. Set n_value field
      * of non-excluded symbols to their ordinal numbers in the new table. Set
      * *symbols to new symbol table, *strings to new string table. Put their
      * sizes in bytes in *symbolsSize and *stringsSize.  Sets *allResolved to
      * TRUE iff all used symbols' values have been resolved.  Produces error
      * messages on stderr for undefined symbols, unless wantReloc is TRUE. */
{
    unsigned int nsyms;
    unsigned int file;
    struct nlist *newSym;
    char *newStr;

    *allResolved = TRUE;

    markUsedSymbols();
    countAndMarkSymbols(&nsyms, stringsSize);

    if (!xSwitch && !rSwitch && !sSwitch && numFiles > 1) {
	for (file = 0; file < numFiles; file++) {
	    if (fileName[file] != NULL) {
		(*stringsSize) += strlen(fileName[file]) + 1;
		nsyms++;
	    }
	}
    }

    *symbolsSize = nsyms * sizeof(struct nlist);

    if (nsyms == 0) {
	*symbols = NULL;
	*strings = NULL;
	return;
    }

    *stringsSize += sizeof(int);
    *symbols = (struct nlist *) malloc(nsyms * sizeof(struct nlist));
    *strings = (char *) malloc(*stringsSize);
    if (*symbols == NULL || *strings == NULL) {
	InsufficientSpace;
    }

    * ((int *) *strings) = *stringsSize;
    newSym = *symbols;
    newStr = *strings + sizeof(int);

    for (file = 0; file < numFiles; file++) {
	struct nlist *sym, 
	             *last = (struct nlist *) 
			          ((char *) fileDefns[file]
				   + fileHeader[file].a_syms);

	if (!sSwitch && !xSwitch && !rSwitch && numFiles>1 
	    && fileName[file] != NULL) {
	    newSym -> n_un.n_strx = newStr - *strings;
	    newSym -> n_type = N_TEXT;
	    newSym -> n_value = fileTextAddr[file];
	    newSym -> n_other = newSym -> n_desc = 0;
	    (void) strcpy(newStr, fileName[file]);
	    newStr += strlen(newStr)+1;
	    newSym++;
	}
	
        for (sym = fileDefns[file]; sym != last; sym++) {
	    char *name;
	    int symSimpType = sym -> n_type & N_TYPE;
	    int symRestType = sym -> n_type ^ symSimpType;

	    if ((sym -> n_other & SYM_EXCLUDE) == 0) {
		*newSym = *sym;	/* Initialize */
		sym -> n_value = newSym - *symbols;   /* Forwarding index */
	    }

	    name = symName(sym);

	    switch (symSimpType) {
	    case N_UNDF:
		if (sym -> n_other & SYM_USED) {
		    *allResolved = FALSE;
		    if (!rSwitch) {
			ErrorMsg "%s undefined.", name EndMsg;
		    }
		}
		break;
	    case N_COMM:
	    case N_SCOMM:
		if (sym -> n_other & SYM_USED) {
		    *allResolved = FALSE;
		}
		if (pdSwitch) {
		    newSym -> n_type = N_SCOMM | symRestType;
		}
		break;
	    case N_TEXT:
		if (ptSwitch) {
		    newSym -> n_type = N_SDATA | symRestType;
		}
		break;
	    case N_DATA:
		if (pdSwitch) {
		    newSym -> n_type = N_SDATA | symRestType;
		}
		break;
	    case N_BSS:
		if (pdSwitch) {
		    newSym -> n_type = N_SBSS | symRestType;
		}
		break;
	    case N_EXPR:
		newSym -> n_type = N_UNDF | symRestType;
		break;
	    default:
		break;
	    }

	    if (sym -> n_other & SYM_EXCLUDE) /* REJECT */
		continue;
	    
	    if (name[0] != '\0') {
		newSym -> n_un.n_strx = newStr - *strings;
		(void) strcpy(newStr, name);
		newStr += strlen(name)+1;
	    }
	    else newSym -> n_un.n_strx = 0;
	    newSym -> n_other = 0;	/* Must be 0 in file */

	    newSym++;
	}
    }
}

unsigned int
segmentOffsetToNewAddr(segment, file, offset)
     unsigned int segment, file, offset;
     /* Return relocated address of [given segment (an r_segment, indicated by 
      * RP_TEXT0,  etc.) from given file + offset].  */
{
    switch (segment) {
    case RP_TEXT0:
	return fileTextAddr[file] + offset;
    case RP_DATA0:
	return fileDataAddr[file] + offset;
    case RP_SDATA0:
	return fileSDataAddr[file] + offset;
    case RP_SYMBOLS:
	{
	    struct nlist *sym = BINCR(fileDefns[file], offset, struct nlist);
	    return sym -> n_value * sizeof(struct nlist);
	}
    default:
	{
	    struct nlist *sym = fileDefns[file] + segment - RP_SEG0;
	    return offset + sym -> n_value;
	}
    }
}

unsigned int
newSegmentAddr(segment)
     unsigned int segment;
     /* Return the address of given segment in new file, where segment must be
      * RP_TEXT0, RP_DATA0, RP_SDATA0. */
{
    switch (segment) {
    case RP_TEXT0:
	return fileTextAddr[0];
    case RP_DATA0:
	return fileDataAddr[0];
    case RP_SDATA0:
	if (ptSwitch) {
	    return fileTextAddr[0];
	}
	else if (pdSwitch) {
	    return fileDataAddr[0];
	}
	else {
	    return fileSDataAddr[0];
	}
    case RP_BSS:
	return fileBssAddr[0];
    case RP_SBSS:
	if (pdSwitch) {
	    return fileBssAddr[0];
	}
	else {
	    return fileSBssAddr[0];
	}
    default:
	ErrorMsg "Internal error in newSegmentAddr." EndMsg;
	exit(1);
    }
}

void
relocSizes(relocsSize, relocExprsSize)
     unsigned int *relocsSize, *relocExprsSize;
     /* Set *relocsSize to total number of bytes required for relocation items
      * and *relocExprsSize to the total number of bytes required for load-time
      * expressions in final file. */
{
    unsigned int file;

    *relocsSize = 0; 
    *relocExprsSize = 0;
    for (file = 0; file < numFiles; file++) {
	struct relocation_info 
	    *reloc,
	    *last = (struct relocation_info *) 
		       ((int) fileRelocs[file] + fileHeader[file].a_rsize);

	for (reloc = fileRelocs[file]; reloc != last; reloc++) {
	    if (reloc -> r_extra != RELOC_EXCLUDE) {
		*relocsSize += 1;
		if (reloc -> r_reltype == RP_REXP)
		    *relocExprsSize += 
			exprLength(BINCR(fileRelocExprs[file], reloc -> r_expr,
					 union reloc_expr));
	    }
	}
    }
    *relocsSize *= sizeof(struct relocation_info);
    *relocExprsSize *= sizeof(union reloc_expr);
}

void
createNewRelocs(relocs, relocExprs, relocsSize, relocExprsSize)
     struct relocation_info **relocs;
     union reloc_expr **relocExprs;
     unsigned int *relocsSize, *relocExprsSize;
     /* Create new relocation data, putting address of relocation item list
      * in *relocs and its size in bytes in *relocsSize, and address of block
      * of relocation expressions in *relocExprs and its size in 
      * *relocExprsSize.  Assumes new symbols have already been built. 
      */ 
{
    unsigned int file;
    struct relocation_info *newReloc;
    union reloc_expr *newRelocExpr;
    
    relocSizes(relocsSize, relocExprsSize);

    if (*relocsSize > 0) {
	newReloc = *relocs = (struct relocation_info *) malloc(*relocsSize);
	if (*relocs == NULL) {
	    InsufficientSpace;
	    /*NOTREACHED*/
	}
    }
    if (*relocExprsSize > 0) {
	newRelocExpr = *relocExprs = 
	    (union reloc_expr *) malloc(*relocExprsSize);
	if (*relocExprs == NULL) {
	    InsufficientSpace;
	    /*NOTREACHED*/
	}
    }

    for (file = 0; file < numFiles; file++) {
	struct relocation_info 
	    *reloc,
	    *last = BINCR(fileRelocs[file], fileHeader[file].a_rsize, 
			  struct relocation_info);

	for (reloc = fileRelocs[file]; reloc != last; reloc++) {
	    if (reloc -> r_extra != RELOC_EXCLUDE) {
		switch (reloc -> r_segment) {
		case RP_TEXT0:
		case RP_DATA0:
		case RP_SDATA0:
		case RP_SYMBOLS:
		    newReloc -> r_segment = reloc -> r_segment;
		    break;
		default:
		    { 
			int stype = 
			   fileDefns[file][reloc -> r_segment - RP_SEG0].n_type
			       & N_TYPE;

			newReloc -> r_segment = 
			    stype == N_TEXT ? RP_TEXT0:
			    stype == N_DATA ? RP_DATA0:
			    RP_SDATA0;
			break;
		    }
		}

		if (ptSwitch && newReloc -> r_segment == RP_TEXT0
		    || pdSwitch && newReloc -> r_segment == RP_DATA0) {
		    newReloc -> r_segment = RP_SDATA0;
		}

		newReloc -> r_address = 
		    segmentOffsetToNewAddr(reloc -> r_segment, file, 
					   reloc -> r_address)
		    - (reloc -> r_segment == RP_SYMBOLS ? 0
		       : newSegmentAddr(newReloc -> r_segment));
		
		switch (reloc -> r_reltype) {
		case RP_REXP:
		    newReloc -> r_expr = 
			BDIFF(createNewRelocExpr(BINCR(fileRelocExprs[file], 
						       reloc -> r_expr,
						       union reloc_expr),
						 file, &newRelocExpr),
			      *relocExprs);
		    break;
		case RP_RSYM:
		    {
			struct nlist *sym;
			unsigned int realFile;

			realSym(fileDefns[file] + reloc -> r_expr, file,
				&sym, &realFile);
			newReloc -> r_expr = sym -> n_value;
		    }
		    break;
		case RP_RSEG:
		    if (ptSwitch && reloc -> r_expr == N_TEXT
			|| pdSwitch && reloc -> r_expr == N_DATA) {
			newReloc -> r_expr = N_SDATA;
		    }
		    else if (pdSwitch && reloc -> r_expr == N_BSS) {
			newReloc -> r_expr = N_SBSS;
		    }
		    else {
			newReloc -> r_expr = reloc -> r_expr;
		    }
		    break;
		default:
		    InternalError;
		    /*NOTREACHED*/
		}			
		    
		newReloc -> r_word = reloc -> r_word;
		newReloc -> r_reltype = reloc -> r_reltype;
		newReloc -> r_length = reloc -> r_length;
		newReloc -> r_extra = 0;
		newReloc++;
	    }
	}
    }
}

union reloc_expr *
createNewRelocExpr(e, file, newExpr)
     union reloc_expr *e, **newExpr;
     unsigned int file;
     /* Move and convert e to *newExpr, incrementing *newExpr beyond end of
      * new expression.  Return address of new expression. */
{
    int len = exprLength(e);
    union reloc_expr *ret = *newExpr;
    int i;
    
    i = 0;
    while (i < len) {
	int re_op = e -> re_syl.re_op;

				/* Adjust for publicized segments.  */
	if (re_op == EO_TEXT && ptSwitch
	    || re_op == EO_DATA && pdSwitch 
	    || re_op == EO_BSS && pdSwitch) {
	    bool dummy;
	    unsigned int newOffset =
		evalLoadExpr(e, file, &dummy) 
		    - newSegmentAddr(re_op == EO_BSS ? RP_SBSS : RP_SDATA0);
	    EO_SET_SYL(*e, re_op == EO_BSS ? EO_SBSS : EO_SDATA);
	    e[1].re_value = newOffset;
	}
    
	switch (re_op) {
	case EO_TEXT:
	case EO_DATA:
	case EO_BSS:
	case EO_INT:
	case EO_SDATA:
	case EO_SBSS:
	    (*newExpr)[0] = e[0];
	    (*newExpr)[1] = e[1];
	    *newExpr += 2;
	    e += 2;
	    i++;
	    break;
	case EO_SYM: {
	    struct nlist *sym = &fileDefns[file][e -> re_syl.re_arg];
	    int outFile;

	    if ((sym -> n_type) & N_EXT) 
		realSym(sym,file,&sym,&outFile);
	    EO_SET_SYL2(**newExpr, EO_SYM, sym -> n_value);
	    (*newExpr)[1] = e[1];
	    *newExpr += 2;
	    e += 2;
	    i++;
	    break;
	}
	default:
	    *(*newExpr)++ = *e++;
	    break;
	}
	i++;
    }
    return ret;
}

void
writeObjectFile(fd, entry,
		relocs, relocsSize, relocExprs, relocExprsSize,
		symbols, symbolsSize, strings, stringsSize)
     int fd;
     unsigned int entry;
     struct nlist *symbols;
     struct relocation_info *relocs;
     union reloc_expr *relocExprs;
     char *strings;
     unsigned int symbolsSize, relocsSize, relocExprsSize, stringsSize;
     /* Write the specified portions of the object file to file fd. */
{
    int i;
    struct exec header;
    unsigned int textPad, dataPad, sdataPad;
    char ZEROS[PAGSIZ];

    (void) bzero(ZEROS, PAGSIZ);

    (void) bzero((char *) &header, sizeof header);
    header.a_magic = magic;
    header.a_bytord = 0x01020304;   /* This isn't quite right yet. */
    header.a_syms = symbolsSize;
    header.a_entry = entry;
    header.a_rsize = relocsSize;
    header.a_expsize = relocExprsSize;

    for (i = 0; i < numFiles; i++) {
	if (ptSwitch)
	    header.a_sdata += REAL_TEXT_LENGTH(fileHeader[i]);
	else
	    header.a_text += REAL_TEXT_LENGTH(fileHeader[i]);
    }

    for (i = 0; i < numFiles; i++) {
	if (pdSwitch) {
	    header.a_sdata += fileHeader[i].a_data;
	    header.a_sbss  += fileHeader[i].a_bss;
	}
	else {
	    header.a_data += fileHeader[i].a_data;
	    header.a_bss  += fileHeader[i].a_bss;
	}
	header.a_sdata += fileHeader[i].a_sdata;
	header.a_sbss  += fileHeader[i].a_sbss;
    }

    if (magic == ZMAGIC) { /* Round up segment sizes and put part of 
			      bss segments in data segments */
	header.a_text += sizeof(struct exec);
	textPad = 
	    ( - (header.a_text & (PAGSIZ - 1))) % PAGSIZ;
	header.a_text += textPad;

	dataPad = ( - (header.a_data & (PAGSIZ - 1))) % PAGSIZ;
	if (header.a_bss < dataPad) 
	    header.a_bss = 0;
	else
	    header.a_bss -= dataPad;
	header.a_data += dataPad;

	sdataPad = ( - (header.a_sdata & (PAGSIZ - 1))) % PAGSIZ;
	if (header.a_sbss < sdataPad) 
	    header.a_sbss = 0;
	else
	    header.a_sbss -= sdataPad;
	header.a_sdata += sdataPad;
	
    }
    else 
	textPad = dataPad = sdataPad = 0;

#   define writeSeg(addr, num) \
    { \
        if (num != 0) {\
	    int bytes = (num); \
	    if (bytes != write(fd, (char *) (addr), bytes)) { \
                ErrorMsg "Write to output file failed." EndMsg; \
		exit(1); \
	    } \
	} \
    }

    writeSeg(&header, sizeof(struct exec));

    if (!ptSwitch) {
	for (i = 0; i < numFiles; i++) {
	    writeSeg(fileText[i], REAL_TEXT_LENGTH(fileHeader[i]));
	}
    }
    writeSeg(ZEROS, textPad);

    if (!pdSwitch) {
	for (i = 0; i < numFiles; i++) {
	    writeSeg(fileData[i], fileHeader[i].a_data);
	}
	writeSeg(ZEROS, dataPad);
    }

    if (ptSwitch) {
	for (i = 0; i < numFiles; i++) {
	    writeSeg(fileText[i], REAL_TEXT_LENGTH(fileHeader[i]));
	}
    }
    for (i = 0; i < numFiles; i++) {
	if (pdSwitch) {
	    writeSeg(fileData[i], fileHeader[i].a_data);
	}
	writeSeg(fileSData[i], fileHeader[i].a_sdata);
    }
    writeSeg(ZEROS, sdataPad);

    writeSeg(relocs, header.a_rsize);
    writeSeg(relocExprs, header.a_expsize);
    writeSeg(symbols, header.a_syms);
    writeSeg(strings, stringsSize);

#   undef writeSeg
}

void
writeCombinedFile(fd)
     int fd;
     /* Assuming all files read in and symbol table interned, create the
      * new object file on file fd.  */
{
    int allResolved;
    struct nlist *symbols;
    char *strings;
    struct relocation_info *relocs;
    union reloc_expr *relocExprs;
    unsigned int symbolsSize, stringsSize, relocsSize, relocExprsSize;
    unsigned int entryPoint;

    computeRelocations();
    
    relocateAll();
    
    entryPoint = findEntry(entryName, fileTextAddr[0]);

    createNewSymbolsAndStrings(&symbols, &strings, &symbolsSize, &stringsSize,
			       &allResolved);

    if (sSwitch) {
	symbols = NULL;
	strings = NULL;
	symbolsSize = stringsSize = 0;
    }

    if (rSwitch) {
	createNewRelocs(&relocs, &relocExprs, &relocsSize, &relocExprsSize);
    }
    else {
	relocs = NULL;
	relocExprs = NULL;
	relocsSize = relocExprsSize = 0;
    }

    writeObjectFile(fd, entryPoint, relocs, relocsSize,
		    relocExprs, relocExprsSize, symbols, symbolsSize,
		    strings, stringsSize);

    if (allResolved && errorCount == 0) {
	struct stat stats;
	(void) fstat(fd, &stats);
	(void) fchmod(fd, (int) (stats.st_mode
		          | (stats.st_mode >> 2 & 
			     (S_IEXEC | S_IEXEC>>3 | S_IEXEC>>6))));
    }
}

			/* Main routine */

void
initTables()
{
    EOtoNmap[EO_TEXT] = N_TEXT; 
    EOtoNmap[EO_DATA] = N_DATA;
    EOtoNmap[EO_BSS] = N_BSS;
    EOtoNmap[EO_SDATA] = N_SDATA;
    EOtoNmap[EO_BSS] = N_SBSS;
}

int
processCommandLineArgs(n, argv)
     int n;
     char *argv[];
     /* Set command line arguments from the command line in argv with n
      * entries.  Does not process files, -l options.  Remaining entries
      * are compressed; their total number is returned. */
{
    int i;
    unsigned int numLSwitches = 0;

    numForcedSyms = numTracedSyms = 0;
    dSwitch = MSwitch = pdSwitch = ptSwitch = rSwitch = sSwitch = SSwitch = 
	tSwitch = wSwitch = xSwitch = XSwitch = FALSE;
    TtextSwitch = TdataSwitch = TsdataSwitch = FALSE;
    outFileName = NULL;
    magic = 0;
    entryName = NULL;
    baseFileName = NULL;

    for (i = 1; i < n; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'A':
		if (magic != ZMAGIC && i+1 < n && baseFileName == NULL) {
		    baseFileName = argv[i+1];
		    argv[i] = argv[i+1] = NULL;
		    magic == OMAGIC;
		    i += 1;
		}
		else {
		    ErrorMsg "Invalid -A option." EndMsg;
		    exit(1);
		}
		break;
	    case 'd':
		dSwitch = TRUE, argv[i] = NULL;
		break;
	    case 'e':
		if (i+1 < n && entryName == NULL) {
		    numForcedSyms += 1;
		    entryName = argv[i+1];
		    i += 1;
		}
		else {
		    ErrorMsg "Invalid entry name."  EndMsg;
		    exit(1);
		}
		break;
	    case 'l':
		break;
	    case 'L':
		numLSwitches += 1;
		break;
	    case 'N':
		if (magic == 0) magic = OMAGIC;
		argv[i] = NULL;
		break;
	    case 'o':
		if (i+1 < n && outFileName == NULL) {
		    outFileName = argv[++i];
		    argv[i-1] = argv[i] = NULL;
		}
		else {
		    ErrorMsg "Invalid output file name."  EndMsg;
		    exit(1);
		}
		break;
	    case 'p':
		if (argv[i][2] == 't' || argv[i][2] == 'T')
		    ptSwitch = TRUE;
		else
		    pdSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 'r':
		rSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 's':
		sSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 'S':
		SSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 'T':
		{
		  enum { TEXT, DATA, SDATA } region;
		  char *hex;

		  if (strncmp(argv[i], "-Ttext", 6) == 0) 
		      region = TEXT, hex = argv[i] + 6, TtextSwitch = TRUE;
		  else if (strncmp(argv[i], "-Tdata", 6) == 0) 
		      region = DATA, hex = argv[i] + 6, TdataSwitch = TRUE;
		  else if (strncmp(argv[i], "-Tsdata", 7) == 0) 
		      region = SDATA, hex = argv[i] + 7, TsdataSwitch = TRUE;
		  else region = TEXT, hex = argv[i] + 2, TtextSwitch = TRUE;
		    
		  if (*hex == '\0') {
		      argv[i] = NULL;
		      if (i+1 < n) 
			  hex = argv[++i];
		      else {
			  ErrorMsg "Invalid -T argument." EndMsg;
			  exit(1);
		      }
		  }
 
		  if (hex[0] == '0' && hex[1] == 'x') hex += 2;
		  (void) sscanf(hex, "%x", 
				region == TEXT ? &textStart :
				region == DATA ? &dataStart :
				&sdataStart);
		  argv[i] = NULL;
		}
		break;
	    case 'u':
		numForcedSyms += 1;
		i += 1;
		break;
	    case 'w':
		wSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 'x':
		xSwitch = TRUE;
		argv[i] = NULL;
		break;
	    case 'X':
		XSwitch = TRUE;	
		argv[i] = NULL;
		break;
	    case 'y':
		numTracedSyms += 1;
		break;
	    case 'z':
		if (magic == 0) magic = ZMAGIC;
		argv[i] = NULL;
		break;
	    default:
		ErrorMsg "Unknown switch: %s.", argv[i] EndMsg;
		exit(1);
		break;
	    }
	}
    }

    {
       char **nextLOption, **nextYOption; 
       struct nlist *nextuOption;
       nextLOption = dirList = 
	   (char **) calloc(numLSwitches+1, sizeof(char *));
       nextYOption = tracedSyms = 
	   (char **) calloc(numTracedSyms, sizeof(char *));
       nextuOption = forcedSyms = 
	   (struct nlist *) calloc(numForcedSyms, sizeof(struct nlist));
       for (i = 1; i < n; i++) {
	   if (argv[i] != NULL && argv[i][0] == '-') {
	       switch (argv[i][1]) {
	       default:
		   break;
	       case 'L':
		   *nextLOption++ = argv[i]+2;
		   argv[i] = NULL;
		   break;
	       case 'y':
		   *nextYOption++ = argv[i]+2;
		   argv[i] = NULL;
		   break;
	       case 'u':
	       case 'e':
		   i += 1;
		   if (i >= n) {
		       ErrorMsg "Invalid -u option." EndMsg;
		       exit(1);
		   }
		   nextuOption -> n_un.n_name = argv[i];
		   nextuOption -> n_type = N_EXT | N_UNDF;
		   nextuOption -> n_value = 0;
		   nextuOption -> n_desc = nextuOption -> n_other = 0;
		   nextuOption++;
		   argv[i] = argv[i-1] = NULL;
		   break;
	       }
	   }
       }
       *nextLOption = NULL;
       qsort((char *) tracedSyms, (int) numTracedSyms, sizeof(char *),
	     indirectStrcmp);
    }

    if (sSwitch) XSwitch = xSwitch = SSwitch = rSwitch = FALSE;
    if (SSwitch) XSwitch = xSwitch = FALSE;
    if (xSwitch) XSwitch = FALSE;
    if (rSwitch) magic = OMAGIC;

    if (magic == 0) 
	magic = (TtextSwitch || TdataSwitch || TsdataSwitch) ? OMAGIC : ZMAGIC;
    if (outFileName == NULL) outFileName = "a.out";
    if (! TtextSwitch) {
	aHeader.a_magic = magic;
	textStart = N_DEFENT(aHeader);
    }

    {
      int t,f;

      for (t = f = 1; f < n; f++)
	  if (argv[f] != NULL) argv[t++] = argv[f];
      return t;
    }
}


main(argc, argv)
     int argc;
     char *argv[];
{
    int outFd;
    int i;

    unresolvedSyms.link = &unresolvedSyms;
    unresolvedSyms.nextUnresolved = &unresolvedSyms;
    
    errorCount = 0;
    programName = argv[0];
    initTables();
    argc = processCommandLineArgs(argc,argv);

    numFiles = 0;
    setupForcedSyms();
    if (baseFileName != NULL) 
	setupBaseFile();
	
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-' && argv[i][1] == 'l') {
	    int fd = openLibrary(argv[i]+2);
	    if (fd == -1) {
		ErrorMsg
		    "Failed to open library file %s.", argv[i]+2
			EndMsg;
		break;
	    }
	    readOpenLibraryFile(fd, argv[i]+2);
	    (void) close(fd);
	}
	else if (argv[i][0] != '-') {
	    int fd = open(argv[i], O_RDONLY);
	    if (fd == -1) {
		ErrorMsg "Failed to open load file %s.", argv[i] EndMsg;
		exit(1);
		/*NOTREACHED*/
	    }
	    
	    if (isLibraryFile(fd)) {
		readOpenLibraryFile(fd, argv[i]);
	    }
	    else {
		struct stat stats;
		(void) fstat(fd, &stats);

		fileName[numFiles] = argv[i];

		if (readOpenObjectFile(fd, stats.st_size, argv[i])
		    != 0) 
		        exit(1);
		convertStringIndices(numFiles);
		internAllSyms(numFiles);
		numFiles++;
	    }
	    (void) close(fd);
	}
    }
    
    outFd = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (outFd == -1) {
	ErrorMsg "Could not open output file." EndMsg;
	exit(1);
    }
    
    writeCombinedFile(outFd);    
    (void) close(outFd);

    if (errorCount) 
	exit(1);
    else
	exit(0);
}
