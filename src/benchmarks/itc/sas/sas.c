/* $Header: sas.c,v 3.15 88/10/18 16:34:39 hilfingr Exp $ */

/* Sas assembler for SPUR */

/* Copyright (c) 1987 by the Regents of the University of California.  
 * All rights reserved.  
 *
 * Author: P. N. Hilfinger
 */

static char *rcsid = "$Header: sas.c,v 3.15 88/10/18 16:34:39 hilfingr Exp $";

#include <stdio.h>
#include <sys/file.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include "sas.h"
#include "parser.h"
#include "a.out.h"

#define MAXEXPRTERMS  1000	/* Maximum number of expression nodes for 
				 * single stmt. */
#define SEGMENTINCR   (1<<14)	/* Size by which to step up buffer for 
				 * assembler text or data segment */
#define MAXFILENAMELENGTH 100   /* Maximum length of file name handled. */
#define MAXPREPROCESSARGS 100   /* Maximum number of arguments to /lib/cpp */
#define MAXTEMPLABELS     100   /* Maximum temporary label number + 1 */

#define IEEE_ARITH 1
#define VAX_ARITH 2
#define OTHER_ARITH 3

#ifdef SPUR
#define FLOAT_ARITH IEEE_ARITH
#endif

#ifdef SUN
#define FLOAT_ARITH IEEE_ARITH
#define BYTES_BIG_ENDIAN
#endif

#ifdef VAX
#define FLOAT_ARITH VAX_ARITH
#endif

#define FIRST_TEXT_SEG RP_SEG0
#define FIRST_DATA_SEG RP_SEG0+3
#define FIRST_SHARED_DATA_SEG RP_SEG0+7
#define BSS_SEG    RP_SEG0+6
#define SBSS_SEG   RP_SEG0+9

struct _segmentDescType {
    char *mem;			/* Start of memory */
    int size;			/* Maximum size */
};

typedef struct _segmentDescType segmentDescType; 

static unsigned int
           LC[RP_SEGLAST+1], 		/* Location counters for each 
					 * segment. */
           segmentSize[RP_SEGLAST+1],	/* Current size for each segment. */
           segmentStart[RP_SEGLAST+1];	/* Starting place in file for each 
					 * segment */

#define NUM_ASM_SEGS  10	/* Number of segments seen by programmer */

static segmentDescType segment[RP_SEGLAST+1];
                                /* Region pointers for each segment. */

static symbolType *segmentSym[RP_SEGLAST+1]; 
                                /* Symbols for the starts of the TEXTx, DATAx, 
				 * SDATAx, and BSS regions. */

static segmentDescType
               reloc,        /* Output relocation data */
               relocExpr,    /* Relocation expression data */
               strings;	     /* Output string table */

static struct nlist *symbols;  /* Output symbol area */

static unsigned int disallowedOpcodeMask;
                               /* Opcodes that yield non-zero when &'ed with 
				* this mask are to be disallowed. */
static bool nativeFloatSwitch;
				/* True iff floating-point constants are to */
				/* be in host native mode for simulations. */

/* Segment sizes in bytes */
static unsigned int
    relocSize,
    relocExprSize,
    symbolsSize,
    stringsSize;

extern void
    initTempLabels(),
    resetTempLabels();

		/* Various utilities */
#ifndef BYTES_BIG_ENDIAN

#define bcopy2(s1, s2, offset, n) \
    (void) bcopy((char *) (s1), ((char *) (s2)) + (offset), (int) (n))

#define numcopy(s1, s2, offset, n) \
{\
    int _numcopy_ = (s1); \
    (void) bcopy((char *) &_numcopy_, ((char *) (s2)) + (offset), (int) (n)); \
    }

#else

#define bcopy2(s1, s2, offset, n) \
{\
     int __n = (n); \
     int __offset = (offset); \
     char *__s1 = (char *)(s1), *__s2 = (char *)(s2); \
     while (__n -- > 0) { \
         __s2[(__offset & ~3) + 3 - (__offset & 3)] = *__s1++; \
	 __offset++; \
     } \
}
 
#define numcopy(x, s2, offset, n) \
{\
     int __n = (n); \
     int __offset = (offset); \
     unsigned int __x = (x); \
     char *__s2 = (char *)(s2); \
     while (__n -- > 0) { \
         __s2[(__offset & ~3) + 3 - (__offset & 3)] = __x & 0xff; \
	 __offset++; \
	 __x >>= 8;\
	 }\
}

#endif

int errorCount;			/* Cumulative error count. */

void
errorHeader()
     /* Print prefix of error message. */
{
    if (linecount == 0) {
	(void) fprintf(stderr, "\"%s\", line 0: ", filename);
    }
    else {
	(void) fprintf(stderr, "\"%s\", line %d-%d: ", filename, linecount-1, 
		       linecount);
    }
}

		/* Expandable segment utilities */

void
initSegment(s, noSpace)
     segmentDescType *s;
     bool noSpace;
     /* Initialize segment s.  Allocate no space if noSpace. */
{
    if (noSpace) {
	s -> size = -1;
	s -> mem = NULL;
    }
    else {
	s->size =  SEGMENTINCR;
	s->mem  =  (char *) malloc(SEGMENTINCR);
	(void) bzero((char *) (s -> mem), SEGMENTINCR);
	if (s->mem == NULL) {
	    ErrorMsg
		"Fatal error: out of memory."
	    EndMsg;
	    exit2(1);
	}
    }
}

void
expandSegment(s,n)
     segmentDescType *s;
     unsigned int n;
     /* Expand segment s to at least size n. If the segment is a dummy segment
      * (containing no space), this operation has no effect. */
{
    char *new;

    if (s->size == -1 || s->size >= n) return;
    n = ((n+SEGMENTINCR-1) / SEGMENTINCR) * SEGMENTINCR;
    new = (char *) malloc(n);
    if (new == NULL) {
	ErrorMsg
	    "Fatal error: out of memory."
        EndMsg;
	exit2(1);
    }
    (void) bcopy( (char *) s -> mem, (char *) new, (int) (s -> size));
    (void) bzero( (int) (s -> size) + (char *) new, (int) (n - s -> size));
    (void) free((char *) s->mem);
    s->mem = new;
    s->size = n;
}    

#define checkSeg(s,n)   \
    /* Check that segment s has at least n bytes */  \
    { if ((s).size < n) expandSegment(&(s), n); }

#define checkThisSeg(n) \
    /* Check that current segment has at least n bytes at current location */ \
    {  checkSeg(segment[currentSegment], LC[currentSegment] + n); }

     		/* Expressions */

static exprType exprSpace[MAXEXPRTERMS];
static exprType *nextExpr;	/* Pointer to next free expression node. */

exprType *
newExpr()
     /* Allocate new expression node. */
{
    if (nextExpr == exprSpace) {
	ErrorMsg  "Fatal assembler error: statement too complex." EndMsg;
	exit2(1);
    }
    return nextExpr--;
}

void
initExprs()
     /* Initialize expression allocator, freeing all space. */
{
    nextExpr = &exprSpace[MAXEXPRTERMS-1];
}
    
exprType *
numToExpr(n)
     unsigned int n;
     /* Convert integer n to an expression */
{
    exprType *e;

    e = newExpr();
    e -> class = MANIFEST_INT;
    e -> v.value = n;
    e -> left = e -> right = NULL;
    return e;
}

exprType *
symToExpr(s)
     symbolType *s;
     /* Convert symbol s to an expression. */
{
    exprType *e;

    switch (s -> type & N_TYPE) {
    case N_ABS:
	e = numToExpr(s -> value);
	break;
    case N_TEXT:
    case N_DATA:
    case N_BSS:
    case N_SDATA:
    case N_SBSS:
	e = newExpr();
	e -> class = SYM_EXPR;
	e -> v.sym = segmentSym[s -> segment];
	e -> offset = s -> value;
	e -> left = e -> right = NULL;
	break;
    default:
	e = newExpr();
	e -> class = SYM_EXPR;
	e -> v.sym = s;
	e -> offset = 0;
	e -> left = e -> right = NULL;
	break;
    }
    return e;
}

exprType *
consUnaryExpr(op, op1)
     unsigned int op;
     exprType *op1;
     /* Create an expression representing op applied to op1. */
{
    if (op1 -> class == MANIFEST_INT)
	switch (op) {
	case '-':
	    return numToExpr( -(op1 -> v.value) );
	case '~':
	    return numToExpr(~op1 -> v.value);
	default:
	    ErrorMsg  "Internal assembler error: bad operator." EndMsg;
	    exit2(1);  /* Should not get here. */
	    return nullExpr;
	}
    else {
	exprType *e = newExpr();
	e -> class = UNARY_EXPR;
	e -> v.opcode = op;
	e -> left = op1;
	e -> right = NULL;
	return e;
    }
}

exprType *
consBinaryExpr(op, op1, op2)
     unsigned int op;
     exprType *op1, *op2;
     /* Return expression representing binary operation op applied to op1 and 
	op2. */
{
    exprClassType op1Class = op1 -> class,
                  op2Class = op2 -> class;

    if (op1Class == MANIFEST_INT && op2Class == MANIFEST_INT)
	switch (op) {
	case '+':
	    return numToExpr(op1->v.value + op2->v.value);
	case '-':
	    return numToExpr(op1->v.value - op2->v.value);
	case '*':
	    return numToExpr(op1->v.value * op2->v.value);
	case '/':
	    return numToExpr(op1-> v.value / op2->v.value);
	case LSHIFT:
	    return numToExpr(op1->v.value << op2->v.value);
	case RSHIFT:
	    return numToExpr(op1->v.value >> op2->v.value);
	case '&':
	    return numToExpr(op1->v.value & op2->v.value);
	case '|':
	    return numToExpr(op1->v.value | op2->v.value);
	case '^':
	    return numToExpr(op1->v.value ^ op2->v.value);
	default:
	    ErrorMsg "Internal assembler error: bad operator." EndMsg;
	    exit2(1);
	    return nullExpr;
	}
    else if ((op == '+' || op == '-') && 
	     op1Class == SYM_EXPR && op2Class == MANIFEST_INT) {
	if (op == '+') op1 -> offset += op2 -> v.value;
	else           op1 -> offset -= op2 -> v.value;
	return op1;
    }
    else if (op == '+' && op2Class == SYM_EXPR && op1Class == MANIFEST_INT) {
	op2 -> offset += op1 -> v.value;
	return op2;
    }
    else if (   op == '-'
	     && op1Class == SYM_EXPR && op2Class == SYM_EXPR 
	     && op1 -> v.sym == op2 -> v.sym) {
	op1 -> class = MANIFEST_INT;
	op1 -> v.value = op1 -> offset - op2 -> offset;
	return op1;
    }
    else {
	exprType *e = newExpr();

	e -> class = BINARY_EXPR;
	e -> v.opcode = op;
	e -> left = op1; e -> right = op2;
	return e;
    }
}

exprType *
pointExpr()
     /* Return expression for `point' */
{
    exprType *e = newExpr();
    e -> class = SYM_EXPR;
    e -> v.sym = segmentSym[currentSegment];
    e -> offset = LC[currentSegment];
    e -> left = e -> right = NULL;
    return e;
}

			/* Temporary labels */

static struct {
    int number;		/* Number for numeric temp, or -1 for non-numeric. */
    symbolType *sym;	/* Symbol for non-numeric temp. */
    int segment;	/* Segment number of defined label. */
    int offset;		/* Offset from start of segment of defined label. */
} backTemps[MAXTEMPLABELS];  /* Data on backwards numeric temps. */

static struct {
    int number;		/* Number for numeric temp, or -1 for non-numeric. */
    symbolType *sym;	/* Symbol for non-numeric temp. */
    symbolType *label;  /* Label inserted in program for current occurrence of
			 * this this temporary label. */
} forwardTemps[MAXTEMPLABELS];
                        /* Symbols assigned to used, but as yet undefined
			 * temporary labels.  Each entry is NULL if no 
			 * outstanding forward reference to that label. */

int numBckwdTemps, numFwdTemps;   /* Number of active temporary labels in each
				     direction */

void
initTempLabels()
{
    numBckwdTemps = numFwdTemps = 0;
}

void
resetTempLabels()
{
    int i;
    for (i = 0; i < numFwdTemps; i++) {
	if (forwardTemps[i].number != -1) {
	    ErrorMsg 
		"Forward-referenced temporary label %d undefined.", i 
		    EndMsg;
	}
	else { 
	    ErrorMsg 
		"Forward-referenced temporary label %s undefined.", 
	             forwardTemps[i].sym->string
			 EndMsg;
	}
    }
    numFwdTemps = numBckwdTemps = 0;
}

void
DefineTempLabel(m, sym)
     int m;
     symbolType *sym;
     /* Create definition for temporary label m if sym is NULL,
      * or symbol sym, if m is -1. */
{
    int n;

    if (numBckwdTemps >= MAXTEMPLABELS) {
	ErrorMsg "Too many temporary labels." EndMsg;	    
	return;
    }

    backTemps[numBckwdTemps].segment = currentSegment;
    backTemps[numBckwdTemps].offset = LC[currentSegment];
    backTemps[numBckwdTemps].number = m;
    backTemps[numBckwdTemps].sym    = sym;
    for (n = 0; backTemps[n].number != m || backTemps[n].sym != sym; n++);
    backTemps[n] = backTemps[numBckwdTemps];
    if (n == numBckwdTemps) numBckwdTemps++;

    for (n = 0; n < numFwdTemps; n++) {
	if (forwardTemps[n].number == m && forwardTemps[n].sym == sym) {
	    forwardTemps[n].label -> segment = currentSegment;
	    forwardTemps[n].label -> value = LC[currentSegment];
	    forwardTemps[n].label -> type = 
		segmentToType(currentSegment);
	    forwardTemps[n] = forwardTemps[--numFwdTemps];
	    break;
	}
    }
}

exprType *
fwdTempLabelToExpr(m, sym)
     int m;
     symbolType *sym;
     /* Return expression corresponding to temp label n next defined */
{
    int n;

    if (numFwdTemps >= MAXTEMPLABELS) {
	ErrorMsg "Too many temporary labels." EndMsg;
	return numToExpr(0);
    }
    forwardTemps[numFwdTemps].number = m; forwardTemps[numFwdTemps].sym = sym;
    forwardTemps[numFwdTemps].label = NULL;
    for (n = 0; forwardTemps[n].number != m || forwardTemps[n].sym != sym; n++)
	;
    if (n == numFwdTemps) {
	symbolType *label = newSymbol("", N_UNDF, 0, 0);
	forwardTemps[n].label = label;
	assignId(label);
	numFwdTemps++;
    }
    return symToExpr(forwardTemps[n].label);
}

exprType *
bckwdTempLabelToExpr(m,sym)
     int m;
     symbolType *sym;
     /* Return expression corresponding to previously defined temporary 
	label n. */
{
    int n;

    for (n = 0; 
	 n < numBckwdTemps && (backTemps[n].number != m 
			       || backTemps[n].sym != sym);
	 n++);

    if (n == numBckwdTemps) {
	ErrorMsg "Temporary label not previously defined." EndMsg;
	return numToExpr(0);
    }
    else {
	exprType *e = newExpr();

	e -> class = SYM_EXPR;
	e -> v.sym = segmentSym[backTemps[n].segment];
	e -> offset = backTemps[n].offset;
	e -> left = e -> right = nullExpr;
	return e;
    }
}

			/* Instruction emission */

#define checkOpcode(opcode)  \
    if ((opcode) & disallowedOpcodeMask) { \
        WarningMsg "Warning: operation only allowed on simulator." EndMsg; \
    } \
    (opcode) &= ~instModifierBits;

void
emitInst(inst) 
    unsigned int inst;
    /* Output instruction inst in current segment. 
       LC[currentSegment] must be divisible by 4 */
{
    if (segment[currentSegment].size == -1) {
	ErrorMsg "Attempt to generate code in uninitialized segment." EndMsg;
    }
    else {
	checkThisSeg(sizeof(unsigned int));
	* (unsigned int *) (segment[currentSegment].mem + LC[currentSegment]) 
	    = inst;
	LC[currentSegment] += sizeof(unsigned int);
    }
}

bool
isValidConstant(n)
     unsigned int n;
     /* True iff n is a valid rc or sc immediate. */
{
    return ( (n & ~immedMask) + ((n & immedSign) << 1) == 0);
}

void
formConstant(n, r)
     unsigned int n;
     int r;
     /* Form the constant n in register r. */
{
    if (n > maxComputedLiteral && n < minComputedLiteral) {

	emitInst( (unsigned int) rd_specialOpcode << opCodePosn | 
		  pc_sreg << src1Posn | r<<destPosn );
	    /* rd_special  r,pc */
	emitInst( (unsigned int) jump_regOpcode << opCodePosn | r << src1Posn 
		  | immedFlag | 0x10 );
	    /* jump_reg    r,16 */
	emitInst( (unsigned int) ld_32Opcode << opCodePosn
		 | r << destPosn | r << src1Posn | immedFlag | 0x0c );
	    /* ld_32       r,r,$12 */
	emitInst( n );
	    /* .long    n */
	emitInst( (unsigned int) add_ntOpcode << opCodePosn );
	    /* nop */
    }
    else {
	unsigned int top;
	int shift, lowzeros, i;
	
	lowzeros = 0, shift = 0;
	if (n >= 0x80000000) {
	    top = ~n;
	    while(top > maxUnsignedRcLit) {
		if (lowzeros == shift && (top & 1) == 1) 
		    lowzeros++;
		top >>= 1;
		shift++;
	    }
	    top = ~top;
	}
	else {
	    top = n;
	    while (top > maxUnsignedRcLit) {
		if (lowzeros == shift && (top & 1) == 0)
		    lowzeros++;
		top >>= 1;
		shift++;
	    }
	}

	emitInst((unsigned int) add_ntOpcode << opCodePosn | r << destPosn 
		 | immedFlag | top & immedMask);
	     /* Put top part in r */
	for (i = shift; i > 0; i -= 3)
	    emitInst((unsigned int) sllOpcode << opCodePosn | r << destPosn
		     | r << src1Posn | immedFlag | (i >= 3 ? 3 : i));
	        /* Shift left */
	if (lowzeros != shift) {
	    emitInst(orOpcode << opCodePosn | r << destPosn | r << src1Posn |
		     immedFlag | (n ^ (top << shift)));
	}
    }
}

void
SetOrg(e)
     exprType *e;
     /* Set origin to value of e. */
{
    unsigned int offset;

    if (LC[currentSegment] > segmentSize[currentSegment])
	segmentSize[currentSegment] = LC[currentSegment];
    if (e -> class == MANIFEST_INT)
	offset = e -> v.value;
    else if (e -> class == SYM_EXPR && e -> v.sym -> id == currentSegment)
	offset = e -> offset;
    else {
	ErrorMsg
	    "Improper change of origin."
        EndMsg;
	return;
    }
    if (offset > segmentSize[currentSegment]) {
	segmentSize[currentSegment] = offset;
	checkSeg(segment[currentSegment], offset);
    }
    LC[currentSegment] = offset;
}

void
leaveSpace(e)
     exprType *e;
     /* Increase the current LC by the value of e >= 0, thus allocating space.
      */
{
    unsigned int newLC;
    if (e -> class != MANIFEST_INT) {
	ErrorMsg
	    "Improper allocation size."
	EndMsg;
	return;
    }
    newLC = e -> v.value + LC[currentSegment];
    if (newLC > segmentSize[currentSegment]) {
	segmentSize[currentSegment] = newLC;
	checkSeg(segment[currentSegment], newLC);
    }
    LC[currentSegment] = newLC;
}

void
emitStab(string, ntype, nother, ndesc, nvalue)
     char *string;
     unsigned int ntype, nother, ndesc;
     exprType *nvalue;
     /* Enter debugging symbol into symbol table.  String is optional string
      * data for n_un field; ntype, nother, ndesc, and nvalue are n_type,
      * n_other, n_desc, and n_value fields in symbol.  In the case that
      * nvalue is null, this assumes that ntype is appropriate to the current
      * segment. */
{
    symbolType *s = newSymbol(string, N_UNDF | ntype & ~N_TYPE, nother, ndesc);

    assignId(s);
    if (nvalue == nullExpr) {
	struct relocation_info *item;

	checkSeg(reloc, relocSize+sizeof(struct relocation_info));
	item = (struct relocation_info *) (reloc.mem + relocSize);
	item->r_address = (unsigned int) ((s -> id) * sizeof(struct nlist));
	item->r_segment = RP_SYMBOLS;
	item->r_word = 0;
	item->r_length = RP_32;
	item->r_extra = 0;
	item->r_reltype = RP_RSYM;
	item->r_expr = currentSegment - RP_SEG0;
	relocSize += sizeof(struct relocation_info);

	s -> value = LC[currentSegment];
    }
    else 
	emitReloc(RP_SYMBOLS,
		  (unsigned int) ((s -> id) * sizeof(struct nlist)),
		  RP_32, 0, nvalue);
}

void
emitBytes(str, len)
     char *str;
     int len;
     /* Place len bytes from str in current segment at current LC.  Move up LC.
      */
{
    checkSeg(segment[currentSegment], LC[currentSegment]+len);
    if (segment[currentSegment].size == -1) {
	ErrorMsg 
	    "Attempt to generate code or data in uninitialized segment." 
		EndMsg;
    }
    else {
	bcopy2(str, segment[currentSegment].mem, LC[currentSegment], len);
	LC[currentSegment] += len;
    }
}

void
emitExpr(e, size)
     exprType *e;
     int size;
     /* Insert constant with value of expression e in current segment at 
      * current LC.  Constant is size bytes long. */
{
    checkSeg(segment[currentSegment], LC[currentSegment]+size);
    if (segment[currentSegment].size == -1) {
	ErrorMsg 
	    "Attempt to generate code or data in uninitialized segment." 
		EndMsg;
	return;
    }
    if (e -> class == MANIFEST_INT) {
        numcopy(e -> v.value, segment[currentSegment].mem, LC[currentSegment],
		size);
    }
    else {
	numcopy(0, segment[currentSegment].mem, LC[currentSegment], size);
	emitReloc(currentSegment, LC[currentSegment], 
		  (size == 8 ? RP_LOW8 : size == 16 ? RP_LOW16 : RP_32),
		  0, e);
    }
    LC[currentSegment] += size;
}

#ifdef VAX
typedef struct  {
    unsigned int
	frac : 23,
	exponent : 8,
	sign : 1;
} SpurSingleFloat;

typedef struct  {
    unsigned int
	frac1 : 20,
	exponent : 11,
	sign : 1;
    unsigned int
	frac0;
} SpurDoubleFloat;
#else
typedef float SpurSingleFloat;
typedef double SpurDoubleFloat;
#endif

#ifdef VAX
struct SINGLEFLOAT {
    unsigned int
	frac1 : 7,
	exponent : 8,
	sign : 1,
        frac0 : 16;
};

struct DOUBLEFLOAT {
    unsigned int
	frac3 : 4,
	exponent : 11,
	sign : 1,
	frac2 : 16;
    unsigned int
	frac1 : 16,
	frac0 : 16;
};
#endif        

SpurSingleFloat
toSingleIEEE(x, len)
     char *x;
     int len;
     /* Convert x (len bytes long)  to IEEE single-precision form for SPUR, 
      * with host's byte order. */
{
    char buffer[100];
    int sign;
    float y;

    sign = 0;
    if (x[0] == '-') 
	sign = 1, x++;
    else if (x[0] == '+')
	x++;

    (void) strncpy(buffer, x, len);
    buffer[len] = '\0';
    (void) sscanf(x, "%e", &y);

#if FLOAT_ARITH == IEEE_ARITH
    return sign ? -y : y;
#endif

#if FLOAT_ARITH == VAX_ARITH
    {
	SpurSingleFloat z;
	struct SINGLEFLOAT *p = (struct SINGLEFLOAT *) &y;
	
	if (p -> exponent == 0)
	    z.exponent = 0, z.frac = 0;
	else if (p -> exponent == 1)
	    z.exponent = 0, 
	    z.frac = p -> frac0 >> 2 | p -> frac1 << 14 | 1 << 21;
	else if (p -> exponent == 2)
	    z.exponent = 0,
	    z.frac = p -> frac0 >> 1 | p -> frac1 << 15 | 1 << 22;
	else
	    z.exponent = p -> exponent - 1, 
	    z.frac = p -> frac0 | p -> frac1 << 16;
	z.sign = sign;
	return z;
    }    
#endif

#if FLOAT_ARITH == OTHER_ARITH
    ErrorMsg "Floating point operands not yet supported." EndMsg;
    return 0.0;
#endif
}

SpurDoubleFloat
toDoubleIEEE(x, len)
     char *x;
     int len;
     /* Convert x (len bytes long)  to IEEE single-precision form for SPUR, 
      * in the SPUR byte order.  */
{
    char buffer[100];
    int sign;
    double y;

    sign = 0;
    if (x[0] == '-') 
	sign = 1, x++;
    else if (x[0] == '+')
	x++;

    (void) strncpy(buffer, x, len);
    buffer[len] = '\0';
    (void) sscanf(x, "%le", &y);

#if FLOAT_ARITH == IEEE_ARITH
    return sign ? -y : y;
#endif

#if FLOAT_ARITH == VAX_ARITH
    {
	struct DOUBLEFLOAT *p = (struct DOUBLEFLOAT *) &y;
	SpurDoubleFloat z;
	
	if (p -> exponent == 0)
	    z.exponent = 0, z.frac1 = 0, z.frac0 = 0;
	else if (p -> exponent == 1)
	    z.exponent = 0, 
	    z.frac1 = p -> frac3 << 14 | p -> frac2 >> 2 | 1 << 18,
	    z.frac0 = (p -> frac2 & 3) << 30
		      | p -> frac1 << 13| p -> frac0 >> 2;
	else if (p -> exponent == 2)
	    z.exponent = 0, 
	    z.frac1 = p -> frac3 << 15 | p -> frac2 >> 1 | 1 << 19,
	    z.frac0 = (p -> frac2 & 1) << 31
		      | p -> frac1 << 15 | p -> frac0 >> 1;
	else
	    z.exponent = p -> exponent - 1,
	    z.frac1 = p -> frac3 << 16 | p -> frac2,
	    z.frac0 = p -> frac1 << 16 | p -> frac0;
	z.sign = sign;
	return z;
    }    
#endif

#if FLOAT_ARITH == OTHER_ARITH
    ErrorMsg "Floating point operands not yet supported." EndMsg;
#endif
}

void
emitFloat(v, l, size)
     char *v;
     int l;
     int size;
     /* Emit l-byte floating point literal v in current segment as a
      * floating point number of given size. */
{
    if (segment[currentSegment].size == -1) {
	ErrorMsg 
	    "Attempt to generate code or data in uninitialized segment." 
		EndMsg;
	return;
    }

    checkSeg(segment[currentSegment], LC[currentSegment]+size);

    if (size == SINGLE) {
	if (nativeFloatSwitch) {
	    float x;
	    (void) sscanf(v, "%e", &x);
	    bcopy((char *) &x, 
		  (char *) (segment[currentSegment].mem + LC[currentSegment]),
		  size);
	}
	else {
	    SpurSingleFloat x;
	    x = toSingleIEEE(v,l);
	    bcopy((char *) &x, 
		  (char *) (segment[currentSegment].mem + LC[currentSegment]),
		  size);
	}
    }
    else {
	if (nativeFloatSwitch) {
	    double x;
	    (void) sscanf(v, "%le", &x);
	    bcopy((char *) &x, 
		  (char *) (segment[currentSegment].mem + LC[currentSegment]),
		  size);
	}
	else {
	    SpurDoubleFloat x;
	    x = toDoubleIEEE(v,l);
	    bcopy((char *) &x, 
		  (char *) (segment[currentSegment].mem + LC[currentSegment]),
		  size);
	}
    }

    LC[currentSegment] += size;
}

int
segmentToType(segment)
     int segment;
     /* Return symbol type (see a.out.h) for given segment number */
{
    switch (segment) {
    case RP_SEG0:
    case RP_SEG0+1:
    case RP_SEG0+2:
	return N_TEXT;
    case RP_SEG0+3:
    case RP_SEG0+4:
    case RP_SEG0+5:
	return N_DATA;
    case RP_SEG0+6:
	return N_BSS;
    case RP_SEG0+7:
    case RP_SEG0+8:
	return N_SDATA;
    case RP_SEG0+9:
	return N_SBSS;
    default:
	ErrorMsg "Internal assembler error: bad segment type." EndMsg;
	exit2(1);
	return 0;
    }
}

void
setSymDefn(sym, e)
     symbolType *sym;
     exprType *e;
     /* Define symbol sym to have value given by e */
{
    int stype = sym -> type & N_TYPE;
    if (stype != N_UNDF && stype != N_COMM && stype != N_SCOMM) {
	ErrorMsg
	    "Symbol %s previously defined.", sym -> string
        EndMsg;
	return;
    }
    if (e -> class == MANIFEST_INT) {
	sym -> type = N_ABS | (sym -> type & ~N_TYPE);
	sym -> value = e -> v.value;
    }
    else if (e -> class == SYM_EXPR && e -> v.sym -> id < NUM_ASM_SEGS) {
	sym -> type = segmentToType(e -> v.sym -> id) | (sym->type & ~N_TYPE);
	sym -> value = e -> offset;
	sym -> segment = e -> v.sym -> id;
    }
    else {
	sym -> type = N_UNDF | (sym-> type & ~N_TYPE);
	sym -> value = 0;
	emitReloc(RP_SYMBOLS,
		  (unsigned int) ((sym -> id) * sizeof(struct nlist)), 
		  RP_32, 0, e);
    }
}

void
setSymCommDefn(sym, e, isShared)
     symbolType *sym;
     exprType *e;
     bool isShared;
     /* Define symbol sym to be a global bss symbol of size given by e. 
      * Place in shared area if isShared, and otherwise make private. */
{
    int type = sym -> type & N_TYPE;
    unsigned int val;
    int commType = N_EXT | (isShared ? N_SCOMM : N_COMM);

    if (e -> class != MANIFEST_INT) {
	ErrorMsg "Absolute expression required for (s)comm." EndMsg;
    }
    val = (e -> v.value + 7) & ~0x7;
    switch (type) {
    case N_UNDF:
	sym -> type = commType;
	sym -> value = val;
	break;
    case N_COMM:
    case N_SCOMM:
	if (type != commType) 
	ErrorMsg "Common symbol redefined for different kind of segment." 
	    EndMsg;
	if (val > sym -> value) 
	    sym -> value = val;
	break;
    default:
	break;
    }
}
    

void
setSymLcommDefn(sym, e, isShared)
     symbolType *sym;
     exprType *e;
     bool isShared;
     /* Define symbol sym to be in the bss region of size given by e. Use
      * shared bss if isShared, and otherwise private bss. */
{
    int oldSegment = currentSegment;
    
    currentSegment = isShared ? SBSS_SEG : BSS_SEG;
    if (e -> class != MANIFEST_INT) {
	ErrorMsg "Absolute expression required for (s)lcomm." EndMsg;
    }
    else {
	setAlignment(3);
	DefineLabel(sym);
	LC[currentSegment] += e -> v.value;
	if (LC[currentSegment] > segmentSize[currentSegment])
	    segmentSize[currentSegment] = LC[currentSegment];
    }
    currentSegment = oldSegment;
}

void
DefineLabel(sym)
     symbolType *sym;
     /* Define sym as label at current position in current region */
{
    int stype = sym -> type & N_TYPE;
    if (stype != N_UNDF && stype != N_COMM && stype != N_SCOMM) {
	ErrorMsg
	    "Label %s previously defined.", sym->string
	EndMsg;
	return;
    }
    sym -> type = segmentToType(currentSegment) | (sym -> type & ~N_TYPE);
    sym -> value = LC[currentSegment];
    sym -> segment = currentSegment;
}

void
setGlobalSym(sym)
     symbolType *sym;
     /* Make sym an exported (or imported) symbol. */
{
    sym -> type |= N_EXT;
}

void
setAlignment(n)
     unsigned int n;
     /* Align current LC to 2**n boundary. */
{
    if (n > 3) {
	WarningMsg "Warning: .align argument greater than 3." EndMsg;
    }
    if (n > 31) n = 31;
    n = 1 << n;
    LC[currentSegment] = (LC[currentSegment] + n - 1 ) & ~(n-1);
    checkSeg(segment[currentSegment], LC[currentSegment]);
}

void
emitRRxInst(op, rd, rs1, rc)
     unsigned int op;
     operandType rd,rs1,rc;
     /* Emit an RRR or RRI format instruction with destination rd, first source
      * register rs1, second source register or immediate rc, and opcode op.
      */
{
    unsigned int inst;

    if (op == nopOpcode && op & disallowedOpcodeMask)
	    op = add_ntOpcode;
    checkOpcode(op);

    inst = op<<opCodePosn | rd.number<<destPosn | rs1.number<<src1Posn;

    if (rc.type == REG) {
	inst |= rc.number<<src2Posn;
    }
    else if (rc.expr -> class == MANIFEST_INT && 
	     !isValidConstant(rc.expr -> v.value)) {
	ErrorMsg
	    "Immediate constant out of range."
	EndMsg;
    }
    else {	
	inst |= immedFlag;
	if (rc.expr -> class == MANIFEST_INT)
	    inst |= (rc.expr -> v.value & immedMask);
	else
	    emitReloc(currentSegment, LC[currentSegment], RP_LOW14,
		      0, rc.expr);
    }
    
    emitInst(inst);
}

void
emitStoreInst(op, from, to, sc)
     unsigned int op;
     operandType from, to;
     exprType *sc;
     /* Emit store-format instruction op with given operands. */
{
    unsigned int inst;
    unsigned int lowPart, highPart;

    checkOpcode(op);

    inst = op<<opCodePosn | from.number<<src2Posn | immedFlag 
	   | to.number << src1Posn;

    if (sc -> class == MANIFEST_INT) {
	lowPart = (sc -> v.value) & immedMask;
	highPart = (sc -> v.value) & (immedSign | ~immedMask);

	inst |=  lowPart & lowStoreMask 
	       | lowPart << (storeHighPosn - src2Posn) & highStoreMask;

	if (isValidConstant(sc -> v.value)) {
	    inst |= to.number << src1Posn
		  | (sc -> v.value) & immedSign << (storeHighPosn - src2Posn);
	}
	else {
	    ErrorMsg
		"Immediate constant out of range."
	    EndMsg;
	}
    }	    
    else {
	emitReloc(currentSegment, LC[currentSegment], RP_SCONS,
		  0, 
		  sc);
    }
    emitInst(inst);
}

void
emitRxCmpInst(op, cond, rs1, cc, instAddr, trap)
     unsigned int op,cond;
     operandType rs1, cc;
     exprType *instAddr;
     bool trap;
     /* Generate compare instruction, with jump target instAddr.  If instAddr 
      * is NULL, the offset field of the instruction is set to 0.  Otherwise, 
      * if trap is TRUE, the offset field of the instruction is set to the 
      * value of expression instAddr.  Otherwise, the offset is set to a 
      * pc-relative word offset from instAddr. */
{ 
    unsigned int inst;

    checkOpcode(op);

    inst = op<<opCodePosn | cond<<condPosn | rs1.number<<src1Posn;

    if (cc.type == REG) 
	inst |= cc.number<<src2Posn;
    else if (cond == eq_tag_immed_code || cond == ne_tag_immed_code) {
	unsigned int val = cc.expr -> v.value;

	if (val > (1<<6)-1) {
	    ErrorMsg "Tag immediate constant value %d out of range.", val 
		EndMsg;
	}
	else 
	    inst |= val << tagImmedPosn;
    }
    else {
	unsigned int val = cc.expr -> v.value;

	inst |= immedFlag;
	if (val > (1<<5)-1) {
	    ErrorMsg "Compare immediate operand value %d out of range.", val
		EndMsg;
	}
	else
	    inst |= val << shortImmedPosn;
    }

    if (instAddr != nullExpr) {
	if (trap) {
	    emitReloc(currentSegment, LC[currentSegment], RP_LOW9, 
		      0, instAddr);
	}
	else
	    emitReloc(currentSegment, LC[currentSegment], RP_LOW9,
		      1,
		      consBinaryExpr('-', instAddr, pointExpr()));
    }

    emitInst(inst);
}

void
emitCmpTagInst(op, cond, rs1, tc, instAddr, trap)
     unsigned int op, cond;
     operandType rs1;
     exprType *tc, *instAddr;
     bool trap;
     /* Emit tag compare instruction. InstAddr is treated as for 
	emitRxCmpInst. */
{
    unsigned int inst;
    unsigned int val = tc -> v.value;

    checkOpcode(op);

    inst = op<<opCodePosn | cond<<condPosn | rs1.number<<src1Posn;

    if (val > (1<<6)-1) {
	ErrorMsg "Tag immediate constant value %d out of range.", val EndMsg;
    }
    else
	inst |= val << tagImmedPosn;

    if (instAddr != nullExpr) {
	if (trap) {
	    emitReloc(currentSegment, LC[currentSegment], RP_LOW9, 
		      0,
		      instAddr);
	}
	else
	    emitReloc(currentSegment, LC[currentSegment], RP_LOW9,
		      1,
		      consBinaryExpr('-', instAddr, pointExpr()));
    }

    emitInst(inst);
}

void
emitJumpInst(op, instAddr)
     unsigned int op;
     exprType *instAddr;
     /* Emit jump-style op instruction with given word address as target. */
{
    unsigned int inst;

    checkOpcode(op);

    inst = op<<jumpOpCodePosn;
    emitReloc(currentSegment, LC[currentSegment], RP_LOW28, 1, 
	      instAddr);

    emitInst(inst);
}

void
emitReloc(segNum, offset, bits, wordp, addr)
     int segNum, bits, wordp;
     unsigned int  offset;
     exprType *addr;
     /* Emit relocation directive for word at given offset from segment 
      * designated by segNum.  Bits indicates bits to be affected, as indicated
      * in r_length field for the relocation_info structure.  Wordp is either 
      * 1 or 0. Addr is the expression whose value is 
      * to be placed in the indicated word.  It is assumed that the value of
      * the location to be modified is 0 in the file.
      */
{
    struct relocation_info *item;
    checkSeg(reloc, relocSize+sizeof(struct relocation_info));
    item = (struct relocation_info *) (reloc.mem + relocSize);
    item->r_address = offset;
    item->r_segment = segNum;
    item->r_word = wordp;
    item->r_length = bits;
    item->r_extra = 0;
    if (addr -> class == SYM_EXPR && addr -> offset == 0) {
	item->r_expr = addr -> v.sym -> id;
	item->r_reltype = RP_RSYM;
    }
    else {
        item->r_expr = emitConvertedExpr(addr);
	item->r_reltype = RP_REXP;
    }

    relocSize += sizeof(struct relocation_info);
}


#define emitExprSyl(x,y) { \
	    checkSeg(relocExpr,  relocExprSize+sizeof(union reloc_expr)); \
	    EO_SET_SYL2(*(union reloc_expr *) (relocExpr.mem + relocExprSize),\
			x, y); \
	    relocExprSize += sizeof(union reloc_expr); \
	    }
#define emitExprVal(x) { \
	    checkSeg(relocExpr,  relocExprSize+sizeof(union reloc_expr)); \
	    ((union reloc_expr *) (relocExpr.mem + relocExprSize)) -> re_value = (x); \
	    relocExprSize += sizeof(union reloc_expr); \
	    }

unsigned int
emitConvertedExpr(e)
     exprType *e;
     /* Convert e to object file format and put into expression area. Return
      * offset into expression area. */
{
    unsigned int result = relocExprSize;

    switch (e -> class) {
    case MANIFEST_INT:
	emitExprSyl(EO_INT, 0);
	emitExprVal(e -> v.value);
	break;
    case SYM_EXPR:
	emitExprSyl(EO_SYM, e -> v.sym -> id);
	emitExprVal(e -> offset);
	break;
    case BINARY_EXPR:
	{
	    int cmnd;
	    switch (e -> v.opcode) {
	    case '+': cmnd = EO_PLUS;
		break;
	    case '-': cmnd = EO_SUB;
		break;
	    case '*': cmnd = EO_MULT;
		break;
	    case '/': cmnd = EO_DIV;
		break;
	    case LSHIFT: cmnd = EO_SLL;
		break;
	    case RSHIFT: cmnd = EO_SRL;
		break;
	    case '&': cmnd = EO_AND;
		break;
	    case '|': cmnd = EO_OR;
		break;
	    case '^': cmnd = EO_XOR;
		break;
	    default:
		ErrorMsg "Internal assembler error: bad operator." EndMsg;
		exit2(1);
	    }
	    emitExprSyl(cmnd, 0);
	    (void) emitConvertedExpr(e -> left);
	    (void) emitConvertedExpr(e -> right);
	}
	break;
    case UNARY_EXPR:
	{
	    int cmnd;

	    switch (e -> v.opcode) {
	    case '-': cmnd = EO_MINUS;
		break;
	    case '~': cmnd = EO_COMP;
		break;
	    default:
		ErrorMsg "Internal assembler error: bad operator." EndMsg;
		exit2(1);
	    }
	    emitExprSyl(cmnd, 0);
	    (void) emitConvertedExpr(e -> left);
	}
	break;
    }

    return result;
}

unsigned int
emitString(s)
     char *s;
     /* Place string s in string area and return offset. */
{
    unsigned int start = stringsSize;
    int len;

    if (s == NULL || s[0] == '\0') return 0;

    len = strlen(s);
    checkSeg(strings, stringsSize+len+1);
    (void) strcpy((char *) (strings.mem + stringsSize), s);
    stringsSize += len+1;
    return start;
}

void
emitSymbol(sym, dest)
     symbolType *sym;
     struct nlist *dest;
     /* Convert sym into object file form and put in dest. */
{
    int stype = sym -> type & N_TYPE;

    dest -> n_un.n_strx = emitString(sym -> string);
    dest -> n_type = 
	sym -> type == N_UNDF ? N_UNDF | N_EXT : sym -> type;
    dest -> n_other = sym -> other, dest -> n_desc = sym -> desc;
    dest -> n_value = sym -> value;
    if (stype == N_TEXT || stype == N_DATA || stype == N_BSS ||
	stype == N_SDATA || stype == N_SBSS)
	    dest -> n_value += segmentStart[sym -> segment];
}

void
emitAllSymbols()
     /* Create object file symbol table in symbols. Set symbolSize. */
{
    symbolType *sym;
    struct nlist *objSym;

    symbolsSize = nextId * sizeof(struct nlist);
    symbols = (struct nlist *) malloc((unsigned) symbolsSize);
    if (symbols == NULL) {
	ErrorMsg "Fatal error.  Insufficient space." EndMsg;
    }
    
    for (sym = firstSym; sym != NULL; sym = sym -> next) {
	objSym = symbols + sym -> id;
	emitSymbol(sym, objSym);
    }
}

void
joinSegments()
     /* Compute positions of start of TEXTn, DATAn, BSS, SDATAn, SBSS
      * in segmentStart, rounded up to multiples of 8. */
{
    unsigned int place = 0;
    int i;

    for (i = 0; i < NUM_ASM_SEGS; i++) {
	int seg = RP_SEG0 + i;
	if (LC[seg] > segmentSize[seg]) segmentSize[seg] = LC[seg];
	if (seg == FIRST_SHARED_DATA_SEG)
	    place = N_SDATADDR(0);
	segmentStart[seg] = place;
	segmentSize[seg] = 
	    (segmentSize[seg] + SEGMENT_ALIGN - 1) & ~(SEGMENT_ALIGN-1);
	place += segmentSize[seg];
    }
}

void
createHeader(head)
     struct exec *head;
     /* Set *head to the appropriate header, assuming segmentSize is set 
      * correctly. */
{
    head -> a_magic = OMAGIC;
    head -> a_bytord = 0x01020304;
    head -> a_text = segmentStart[FIRST_DATA_SEG];
    head -> a_data = segmentStart[BSS_SEG] - segmentStart[FIRST_DATA_SEG];
    head -> a_sdata = 
	segmentStart[SBSS_SEG] - segmentStart[FIRST_SHARED_DATA_SEG];
    head -> a_bss =  segmentSize[BSS_SEG];
    head -> a_sbss = segmentSize[SBSS_SEG];
    head -> a_syms = symbolsSize;
    head -> a_entry = 0;
    head -> a_rsize = relocSize;
    head -> a_expsize = relocExprSize;
    head -> a_padding = 0;
}

void
writeRegion(fd, p, size)
     int fd;
     unsigned int size;
     char *p;
     /* Write region of storage of size bytes starting at p to fd. */
{
    if (size == 0) return;
    if (size != write(fd, (char *) p, (int) size)) {
	ErrorMsg "Write failed." EndMsg;
	exit2(1);
    }
}

void
writeObject(fd)
     int fd;
     /* Write object file on file fd.  Assumes that segments, segmentSize, 
      * reloc, relocExpr, relocSize, relocExprSize, and firstSym are set.  Sets
      * segmentStart, strings, symbols, symbolsSize, stringsSize. Destructively
      * modifies all of these data. */
{
    struct exec header;
    int i;

    joinSegments();
    emitAllSymbols();
    createHeader(&header);
    
    *((int *) strings.mem) = stringsSize;
    
    writeRegion(fd, (char *) &header, sizeof(header));
    for (i = 0; i < NUM_ASM_SEGS; i++) {
	int seg = RP_SEG0 + i;
	if (seg != BSS_SEG && seg != SBSS_SEG)
	    writeRegion(fd, segment[seg].mem, segmentSize[seg]);
    }
    writeRegion(fd, reloc.mem, relocSize);
    writeRegion(fd, relocExpr.mem, relocExprSize);
    writeRegion(fd, (char *) symbols, symbolsSize);
    writeRegion(fd, strings.mem, stringsSize);
}

		/* Initialization */
void
initSas()
{
    int i;

    for (i = 0; i < NUM_ASM_SEGS; i++) {
	int seg = RP_SEG0 + i;
	symbolType *sym =
	    segmentSym[seg] = (symbolType *) malloc(sizeof(symbolType));

	initSegment(&segment[seg], seg == BSS_SEG || seg == SBSS_SEG);
	segmentSize[seg] = LC[seg] = 0;
	sym -> id = -1;
	assignId(sym);
	sym -> type = segmentToType(seg);
	sym -> string[0] = '\0';
	sym -> segment = seg;
	sym -> value = 0;
    }
    
    initSegment(&reloc, FALSE);
    relocSize = 0;
    initSegment(&relocExpr, FALSE);
    relocExprSize = 0;
    initSegment(&strings, FALSE);
    stringsSize = sizeof(int);

    currentSegment = FIRST_TEXT_SEG;

    initTempLabels();
}
			/* Command line processing, etc. */

static int waitPid = -1;		/* Process id for preprocessor. */
int linecount = 1;		/* Current line number */
char filename[MAXFILENAMELENGTH+1];
                                /* Name of current input file */

int
waitForChild()
     /* Wait for child process, if any, and return its status, or 0 if none. */
{
    int pid;
    union wait status;
    
    if (waitPid == -1) return 0;

    for (pid = -1; pid != waitPid; pid = wait(&status));
    waitPid = -1;
    return (status.w_T.w_Retcode);
}

void
exit2(status)
     int status;
     /* Wait for waitPid to finish and then exit with given status. */
{
    (void) fclose(stdin);

    (void) waitForChild();
    exit(status);
}

void
setInput(f, preprocessp,args,numArgs)
     char *f;
     int preprocessp, numArgs;
     char *args[];
     /* Set stdin to input file f.  If preprocessp, then this input consists of
	the output of cpp given file name f (stdin if f is ""). Otherwise, f or
	stdin is used as the input file directly.  Args[1 .. numArgs] supply
	additional arguments to preprocessor.  The array args may be modified.
	Sets waitPid to pid of process or -1. */
{
    int filedes[2];

    waitPid = -1;
    if (preprocessp) {
	if (pipe(filedes) == -1) {
	    ErrorMsg "Failed to invoke preprocessor." EndMsg;
	    exit(1);
	}
	if (f[0] != '\0' && access(f, F_OK | R_OK)) {
	    ErrorMsg "Can't open file %s.", f EndMsg;
	    exit(1);
	}
	args[numArgs+1] = f;
	args[numArgs+2] = NULL;
	args[0] = PREPROCESSOR;
	waitPid = vfork();
	if (waitPid == 0) {
	    (void) close(1);
	    (void) dup(filedes[1]);
	    (void) close(filedes[0]);
	    (void) close(filedes[1]);
	    (void) execv(PREPROCESSOR, args);
	    exit(1);	/* Shouldn't get here. */
	}
	if (waitPid == -1) {
	    ErrorMsg "Failed to invoke preprocessor." EndMsg;
	    exit(1);
	}
	(void) close(0);
	(void) dup(filedes[0]);
	(void) close(filedes[0]);
	(void) close(filedes[1]);
    }
    else {
	if (f[0] != '\0' && freopen(f, "r", stdin) == NULL) {
	    ErrorMsg "Open failed for file %s.", f EndMsg;
	    exit(1);
	}
    }
}

main(argc, argv)
     int argc;
     char *argv[];
{
    int i;
    char outFileName[MAXFILENAMELENGTH+1];
    char *preprocessArgs[MAXPREPROCESSARGS];	    /* Arguments to /lib/cpp */
    int numPreprocessArgs = 0;
    int outFd;
    bool preprocess = FALSE;	/* TRUE indicates preprocessing needed. */
    bool load = TRUE;		/* TRUE indicates post-processing by sld 
				   needed. */
    bool preserveL = FALSE;	/* TRUE indicates local labels beginning
				 * with L should be preserved. */

    errorCount = 0;
    disallowedOpcodeMask = simulatorOpMask;
    nativeFloatSwitch = FALSE;
    filename[0] = '\0';
    outFileName[0] = '\0';

    for (i = 1; i < argc; i++) {
	char *s;

	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'o':
		if (argc > i+1)
		    (void) strcpy(outFileName, argv[++i]);
		else {
		    ErrorMsg "Invalid -o switch." EndMsg;
		    exit(1);
		}
		break;
	    case 'f':		/* Allow "fake" simulator operations */
		disallowedOpcodeMask &= ~simulatorOpMask;
		break;		
	    case 'F':
		nativeFloatSwitch = TRUE;
		break;
	    case 'L':
		preserveL = TRUE;
		break;
	    case 'p':
		preprocess = TRUE;
		break;
	    case 'I':
	    case 'D':
	    case 'U':
		preprocessArgs[1+numPreprocessArgs++] = argv[i];
		preprocess = TRUE;
		break;
	    case 'a':
		load = FALSE;
		break;
	    default:
		ErrorMsg "Unknown command line option %s.", argv[i] EndMsg;
		exit(1);
	    }
	}
	else {
	    if (filename[0] != '\0') {
		ErrorMsg "Only one file allowed." EndMsg;
		exit(1);
	    }
	    (void) strncpy(filename, argv[i], MAXFILENAMELENGTH);
	    if (outFileName[0] == '\0') {
		for (s = filename + strlen(filename); 
		     s != filename && s[-1] != '/'; s--);
		(void) strcpy(outFileName, s);
		if (outFileName[strlen(outFileName) - 2] == '.')
		    outFileName[strlen(outFileName)-1] = 'o';
		else (void) strcat(outFileName, ".o");
	    }
	}
    }
    if (outFileName[0] == '\0') 
	(void) strcpy(outFileName, "a.out");

    setInput(filename,preprocess,preprocessArgs, numPreprocessArgs);
    initLexer();
    initParser();
    initSas();
    (void) yyparse();
    resetTempLabels();
    (void) fclose(stdin);
    (void) waitForChild();

    if (errorCount != 0) 
	exit(1);
    
    outFd = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (outFd == -1) {
	ErrorMsg "Failed to open file %s.", outFileName EndMsg;
	exit(1);
    }
    writeObject(outFd);
    
    if (errorCount != 0) {
	(void) ftruncate(outFd,0);
	exit(1);
    }
    
    (void) close(outFd);
    
    if (load) {
	char *loadArgs[8];
	int i = 0;
	
	loadArgs[i++] = LOADER;
	loadArgs[i++] = "-r";
	loadArgs[i++] = "-o"; loadArgs[i++] = outFileName;
	if (!preserveL) loadArgs[i++] = "-X";
	loadArgs[i++] = outFileName;
	loadArgs[i++] = NULL;
	
	(void) fflush(stdout);
	(void) fflush(stderr);
	(void) execv(LOADER, loadArgs);
	ErrorMsg "Sld post-processing failed." EndMsg;
	exit(1);
    }
    
    exit(0);
}
