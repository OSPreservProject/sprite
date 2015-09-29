/* $Header: lexer.c,v 3.6 88/10/18 16:34:32 hilfingr Exp $ */

/* Lexical analysis for SPUR assembler (sas) */

/* Copyright (c) 1987 by the Regents of the University of California.  All 
 * rights reserved.  
 *
 * Author: P. N. Hilfinger
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "a.out.h"
#include "sas.h"
#include "parser.h"

#define USERSYMTABLESIZE 1031
#define PREDEFINEDSYMTABLESIZE 211
#define BLOCKSIZE 4096

    /* Table of symbols defined in input */
static symbolType *userSymTable[USERSYMTABLESIZE];

   /* Table of predefined symbols */
static symbolType *predefinedSymTable[PREDEFINEDSYMTABLESIZE];

extern YYSTYPE yylval;	/* Global used to communicate semantic information to parser. */

struct genericInitTable {  /* General form of initialization structures */
    char  *string;
    int   type;
    int   id;
};

static struct genericInitTable predefinedSymbolInitTable[] = {
#   include "predefines.i"
#   include "opcodes.i"
    { ".globl", _globl, 0 },
    { ".extern", _extern, 0 },
    { ".long", _long, 0 },
    { ".word", _word, 0},
    { ".byte", _byte, 0},
    { ".single", _single, 0},
    { ".double", _double, 0 },
    { ".ascii", _ascii, 0 },
    { ".asciz", _asciz, 0 },
    { ".align", _align, 0},
    { ".comm", _comm, 0},
    { ".lcomm", _lcomm, 0},
    { ".scomm", _scomm, 0},
    { ".slcomm", _slcomm, 0},
    { ".text", _text0, 0},
    { ".text0", _text0, 0},
    { ".text1", _text1, 0}, 
    { ".text2", _text2, 0},
    { ".data", _data0, 0},
    { ".data0", _data0, 0},
    { ".data1", _data1, 0},
    { ".data2", _data2, 0},
    { ".sdata", _sdata0, 0},
    { ".sdata0", _sdata0, 0},
    { ".sdata1", _sdata1, 0},
    { ".set", _set, 0},
    { ".org", _org, 0},
    { ".space", _space, 0},
    { ".stabs", _stabs, 0},
    { ".stabn", _stabn, 0},
    { ".stabd", _stabd, 0},
    { NULL, 0, 0 }
};

static bool commandSet = FALSE;		/* TRUE iff module initialized */
static bool atLineStart;		/* TRUE iff at start of line in input */

static bool blank[128];			/* blank[c] iff c is a blank character. */
static bool idChar[128];		/* idChar[c] iff c can appear after first char 
					 * of an identifier. */
static char class[128];			/* Classes of the charcters:
					   'A':  Letters
					   ',':  Various single-character punctuation.
					   '0':  Digits
					   '+':  + or -
					   other characters are their own class
					 */
		/* Symbol table manipulation */

int
hash(s, tableSize)
     char *s;
     int tableSize;
     /* Produce a hashed value for s into a table of size tableSize.  Algorithm due to
      * P. J. Weinberger, as published in Aho, Sethi, Ullman. */
{
    char *p;
    unsigned h = 0, g;

    for (p = s; *p != '\0'; p++) {
	h = (h << 4) + *p;
        g = h & 0xf0000000;
	if (g != 0)
	    h ^= (g >> 24) ^ g;
    }
    return (int) (h % tableSize);
}

int 		nextId;		/* Next user symbol id. */
symbolType 	*lastSym;   /* Last created symbol */
symbolType	*firstSym;

void
assignId(s)
     symbolType *s;
     /* Assign next ordinal in sequence to s and link into list of user 
      * symbols, if s is new. */
{
    if (s->id != -1) return;
    s -> id = nextId++;
    if (firstSym == NULL)
	firstSym = lastSym = s;
    else lastSym = lastSym -> next = s;
    s -> next = NULL;
}

symbolType *
newSymbol(s, type, other, desc)
     char *s;
     unsigned int type, other, desc;
     /* Create a new, uninterned symbol with given string, n_type,
      * n_other, and n_desc fields.  */
{
    symbolType *e = 
	(symbolType *) malloc((unsigned) 
			      (sizeof(symbolType) + strlen((char *) s)));

    (void) strcpy(e -> string, (char *) s);
    e -> type = type, e -> other = other, e -> desc = desc;
    e -> id = -1, e -> value = 0, e -> link = NULL, e -> segment = -1;
    return e;
}

symbolType *
intern(s,hashTable,tableSize,addp)
     char *s;
     symbolType *hashTable[];
     int tableSize, addp;

     /* Return table entry for symbol s in given hashTable of given
      * tableSize.  If addp is false, return null if entry not
      * initially present. */
{
    int h;
    symbolType *e = (symbolType *) NULL;

    if (s[0] != '\0') {
	h = hash(s, tableSize);
	
	for (e = hashTable[h]; e != NULL && strcmp(s, e->string) != 0;
	     e = e->link);
    }
    if (e == NULL && addp) {
	e = newSymbol((char *) s, N_UNDF, 0, 0);
	e->link = hashTable[h], hashTable[h] = e;
    }

    return e;
}

void
initTable(table, tableSize, initTable)
     symbolType *table[];
     int tableSize;
     struct genericInitTable initTable[];
     /* Initialize table (with tableSize entries) from the list of 
      * symbols and associated data in initTable.
      */
{
    struct genericInitTable *c;

    for (c = initTable; c->string != NULL; c++) {
	symbolType *e = intern(c->string, table, tableSize, TRUE);
	e->id = c->id;
	e->type = (int) c->type;
    }
}

		/* Input */

/* It is assumed that no line will be longer than BLOCKSIZE.  If a line is 
 * longer than BLOCKSIZE, or if the last line of a file is improperly 
 * terminated, the lexer may either report an error and insert a newline or 
 * do the right thing. */

static char inputBuffer[BLOCKSIZE*2+1];	/* Space for two blocks of input. */
static char *nextin;			/* Position of next unprocessed 
					   input char. */
static char *lastin;			/* Address of character beyond the
					   end of the current input buffer. */

int
read2(where, len)
     char *where;
     int len;
     /* Read len characters into where, returning number actually read, 
      * or -1 in case of error. The number returned is always len unless 
      * the end of file is reached or there is an error. */
{
    int n = len, m;

    while (n > 0) {
	m = read(0, where, n);
	if (m == -1) return -1;
	if (m == 0) return len-n;
	n -= m;
	where += m;
    }
    return len;
}

void
initInput()
     /* Called at beginning of each source file to initialize input */
{
    int len;

    nextin = inputBuffer;
    len = read2(inputBuffer, BLOCKSIZE*2);
    if (len == -1) {
	ErrorMsg  "Unable to read input." EndMsg;
	exit2(1);
    }
    lastin = nextin + len;
    *lastin = '\n';
}

bool
checkInput()
     /* If necessary, read more input from standard input.  Returns TRUE iff 
      * there is more input. */
{
    if (nextin - inputBuffer >= BLOCKSIZE) {
	int len;

	(void) bcopy(inputBuffer + BLOCKSIZE, inputBuffer, BLOCKSIZE);
	nextin -= BLOCKSIZE;
	lastin -= BLOCKSIZE;
	len = read2(inputBuffer + BLOCKSIZE, BLOCKSIZE);
	if (len == -1) {
	    ErrorMsg "Read failed." EndMsg;
	    exit2(1);
	}
	lastin += len;
	*lastin = '\n';
    }
    return (nextin < lastin);
}
			/* Initialization */

void
initLexer()
{
    int i;

    initInput();
    linecount = 1;
    nextId = 0;
    firstSym = lastSym = NULL;
    atLineStart = TRUE;

    for (i = 0; i < USERSYMTABLESIZE; i++) {
	symbolType *e = userSymTable[i];

	while ((e = userSymTable[i]) != NULL) {
	    userSymTable[i] = e->link;
	    (void) free((char *) e);
	}
    }

    if (! commandSet) {
	int c;
	commandSet = TRUE;

	initTable(predefinedSymTable, PREDEFINEDSYMTABLESIZE, 
		  (struct genericInitTable *) predefinedSymbolInitTable);

	for (c = 0; c < 128; c++) {
	    blank[c] = (c == ' ' || c == '\t' || c == '\f');
	    class[c] = 
		isalpha(c) || c == '_' ? 'A' :
		isdigit(c)             ? '0' :
		c == '+' || c == '-'   ? '+' :
		c;
	    idChar[c] = isalpha(c) || isdigit(c) || c == '_' || c == '.';
	}
	class['='] = class['^'] = class[':'] = class['~'] = class['&'] =
	     class[';'] = class['*'] = class['/'] = class['('] = class[')'] =
	     class['|'] = class['$'] = class['%'] = ',';
    }
}
			/* Main lexical analysis routines */

void
readString()
     /* Read string starting at nextin, setting yylval appropriately.  Advances
      * nextin. */
{
    char *inputStart = nextin+1;
    char *str;
    /* *stringBuffer and stringBufferLength hold a string constant until the 
     * next string literal.  stringBufferLength is the size of the
     * area pointed to by stringBuffer.  It is expanded as needed to hold
     * the largest string read by readString. */
    static char *stringBuffer = NULL;
    static int stringBufferLength = 0;

    str = inputStart;
    for (nextin += 1; *nextin != '"' && *nextin != '\n'; nextin++) {
	if (*nextin == '\\') {
	    nextin++;
	    switch (*nextin) {
	    default:
		*str++ = *nextin; break;
	    case 'b':
		*str++ = '\b'; break;
	    case 'f':
		*str++ = '\f'; break;
	    case 'n': 
		*str++ = '\n'; break;
	    case 'r': 
		*str++ = '\r'; break;
	    case 't':
		*str++ = '\t'; break;
	    case 'v':
		*str++ = '\v'; break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
		{
		    int i,n;
		    for (i = 1, n = 0; 
			 i <= 3 && *nextin >= '0' && *nextin <= '7'; 
			 n = n*8 + *nextin - '0', i++, nextin++);
		    *str++ = (char) n;
		    nextin--;
		}
		break;
	    }
	}
	else *str++ = *nextin;
    }

    if (*nextin != '"') {
	ErrorMsg "Runaway string." EndMsg;
    }
    else nextin++;

    *str = '\0';
    yylval.string.len = str - inputStart;
    if (yylval.string.len >= stringBufferLength) {
	if (stringBuffer != NULL) 
	    (void) free(stringBuffer);
	stringBufferLength = yylval.string.len+1;
	stringBuffer = (char *) malloc((unsigned) stringBufferLength);
    }
    (void) strcpy(stringBuffer, inputStart);
    yylval.string.str = (char *) stringBuffer;
}

int
readNum()
     /* Read longest legal number or additive operator (+ or -) from nextin.
      * Put value in yylval and return syntactic type. Advances nextin. */
{
    char *p = nextin,
         t;

    /* The following FSR recognizes decimal, octal, and hexadecimal integers,
     * signed and unsigned real numbers, + and - operators, and numeric 
     * temporary labels. */

#define STATE(label) label:
#define TRANS(c, state) if (*p == (c)) { p++; goto state; }
#define DTRANS(state)	goto state;
#define CTRANS(func, state) if (func(*p)) { p++; goto state; }
#define RETFIRST  yylval.num = *nextin; return (*nextin++);
#define RETINT(format, type) \
    t = *p; \
    *p = '\0'; \
    (void) sscanf(nextin, format, &yylval.num); \
    nextin = p; \
    *p = t; \
    return type;
#define RETREAL \
    yylval.string.len = p - nextin; \
    yylval.string.str = nextin; \
    nextin = p; \
    return FLOATCONST;

/* STATE(start) */
    TRANS('+', _signed)
    TRANS('-', _signed)
    TRANS('0', leadingZero)
    DTRANS(unsignedInt)

STATE(unsignedInt)
    CTRANS(isdigit, unsignedInt)
    TRANS('f', fwdRef)
    TRANS('b', bckwdRef)
    TRANS('.', okReal)
    RETINT("%d", INTCONST);

STATE(fwdRef)
    RETINT("%d", FWDNUMTEMPREF)

STATE(bckwdRef)
    RETINT("%d", BCKWDNUMTEMPREF)

STATE(_signed)
    CTRANS(isdigit, signedInt)
    RETFIRST

STATE(signedInt)
    CTRANS(isdigit, signedInt)
    TRANS('.', okReal)
    RETFIRST

STATE(okReal)
    CTRANS(isdigit, okReal)
    TRANS('e', exponent)
    TRANS('E', exponent)
    RETREAL

STATE(exponent)
    TRANS('+', exponent2)
    TRANS('-', exponent2)
    CTRANS(isdigit, exponent3)
    RETFIRST

STATE(exponent2)
    CTRANS(isdigit, exponent3)
    RETFIRST

STATE(exponent3)
    CTRANS(isdigit, exponent3)
    RETREAL

STATE(leadingZero)
    CTRANS(isdigit, octalOrReal)
    TRANS('x', hex1)
    TRANS('X', hex1)
    TRANS('f', fwdRef)
    TRANS('b', bckwdRef)
    TRANS('.', okReal)
    RETINT("%d",INTCONST)

STATE(octalOrReal)
    CTRANS(isdigit, octalOrReal)
    TRANS('.', okReal)
    RETINT("%o",INTCONST)

STATE(hex1)
    CTRANS(isxdigit, hex2)
    RETFIRST

STATE(hex2)
    CTRANS(isxdigit, hex2)
    nextin += 2;
    RETINT("%x",INTCONST)

#undef STATE
#undef TRANS
#undef DTRANS
#undef CTRANS
#undef RETFIRST
#undef RETINT
#undef RETREAL
}

int
yylex()
     /* Read next lexeme from input.  Return its lexical type, and, if its
      * type designates a class of more than one lexeme, return additional data
      * in yylval.
      */
{
    while (TRUE) {
	if (!checkInput()) return 0;

	if (blank[*nextin]) atLineStart = FALSE;
	while (blank[*nextin]) nextin++;

	switch (class[*nextin]) {
	case '\n':
	    linecount++; nextin++;
	    atLineStart = TRUE;
	    return EOLN;
	case '/':
	    atLineStart = FALSE;
	    if (*(++nextin) == '*') {
		while (TRUE) {
		    while (*nextin != '\n' && *nextin != '*') nextin++;
		    if (*nextin == '\n') {
			linecount++;
			if (nextin >= lastin) {
			    ErrorMsg "Line too long; newline inserted." EndMsg;
			    (void) checkInput();
			}
			nextin++;
			if (!checkInput()) {
			    ErrorMsg "Unterminated comment." EndMsg;
			    return 0;
			}
		    }
		    else /* *nextin == '*' */ {
			if (*(++nextin) == '/') {
			    nextin++;
			    break;
			}
		    }
		}
	    }
	    else {
		yylval.num = (unsigned int) '/';
		return '/';
	    }
	    continue;
	case '#':
	    if (atLineStart) {
		char *nextnewline;
		char dummy;
		int num;

		for (nextnewline = nextin; *nextnewline != '\n'; nextnewline++)
		    ;
		*nextnewline = '\0';
		num = sscanf(nextin, "# %*d \"%*[^\"]%c", &dummy);
		if (num != 1 || dummy != '\"') {
		    ErrorMsg "Invalid preprocessor command or comment." EndMsg;
		}
		else {
		    (void) sscanf(nextin, "# %d \"%[^\"]", &linecount, 
				  filename);
		    linecount--;
		}
		*nextnewline = '\n';
	    }
	    atLineStart = FALSE;
	    while (*(++nextin) != '\n');
	    if (lastin == nextin) {
		ErrorMsg "Line too long; newline inserted." EndMsg;
		(void) checkInput();
	    }
	    continue;
	case ',':	/* Random punctuation */
	    atLineStart = FALSE;
	    yylval.num = (unsigned int) *nextin;
	    nextin++;
	    return yylval.num;
	case '<':
	    atLineStart = FALSE;
	    if (nextin[1] != '<') break;
	    nextin += 2;
	    yylval.num = LSHIFT;
	    return LSHIFT;
	case '>':
	    atLineStart = FALSE;
	    if (nextin[1] != '>') break;
	    nextin += 2;
	    yylval.num = RSHIFT;
	    return RSHIFT;
	case '0':
	case '+':
	    atLineStart = FALSE;
	    return readNum();
	case '"':
	    atLineStart = FALSE;
	    readString();
	    return STRINGCONST;
	case '@':
	    {
		char *p, *end;
		char t;
		int lexType;

		atLineStart = FALSE;
		if (class[nextin[1]] != 'A') {
		    nextin++;
		    ErrorMsg "Ill-formed temporary label." EndMsg;
		    continue;
		}

		for (p = nextin+1; idChar[*p]; p++);
		end = p;
		
		if (p[0] == '$' && p[1] == 'w') 
		    p += 2;
		else {
		    for ( ; blank[*p]; p++);
		}

	        if (*end == '$' || *p != ':') {
		    end--;
		    if (end[0] == 'f') lexType = FWDTEMPREF;
		    else if (end[0] == 'b') lexType = BCKWDTEMPREF;
		    else {
			ErrorMsg
			    "Temporary label assumed to be forward." 
				EndMsg;
			end++;
			lexType = FWDTEMPREF;
		    }
		}
		else lexType = TEMPLABEL;

		t = end[0]; end[0] = '\0';
		yylval.sym = intern(nextin, userSymTable, 
				    USERSYMTABLESIZE, TRUE);
		end[0] = t;

		nextin = p;
		return lexType;
	    }
	case '.':
	    atLineStart = FALSE;
	    if (!idChar[nextin[1]]) {
		nextin++;
		yylval.num = '.';
		return '.';
	    }
	    /* Else, fall through */
	case 'A':
	    {
		symbolType *sym;
		int tokenType;
		char *p,t;
		
		atLineStart = FALSE;
		for (p = nextin; idChar[*p]; p++);
		if (p[0] == '$' && p[1] == 'w') p += 2;
		t = *p;
		*p = '\0';

		sym = intern(nextin, predefinedSymTable, 
			     PREDEFINEDSYMTABLESIZE, FALSE);

		if (sym != NULL) {
		    yylval.num = sym -> id;
		    nextin = p; *p = t;
		    return sym -> type;
		}
		if (p[-2] == '$' && p[-1] == 'w') {
		    tokenType = WORDID;
		    p[-2] = '\0';
		}
		else tokenType = ID;
		yylval.sym = intern(nextin,userSymTable,USERSYMTABLESIZE,TRUE);
		*p = t; nextin = p;
		
		assignId(yylval.sym);
		return(tokenType);
	    }
	default:
	    atLineStart = FALSE;
	    break;
	}

	ErrorMsg 
	    "Illegal character %c (%x) encountered.", *nextin, (unsigned) *nextin
	EndMsg;
	nextin++;
    }
}
