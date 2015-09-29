/* $Header: sas.h,v 2.10 88/06/30 16:03:15 hilfingr Exp $ */

/* Data shared by lexer and parser. */


#define TRUE  -1
#define FALSE 0

typedef int bool;

#define LASTINTREG   31
#define LASTFLOATREG 15
#define FIRSTUSERID   4

#define SEGMENT_ALIGN 8		/* Segments aligned to multiple of this. */

extern int currentSegment;	/* Current region number into which 
				 * instructions are being assembled.  Possible
				 * values in #defines just above. */

#define LONG   4
#define WORD   2
#define BYTE   1
#define SINGLE 4
#define DOUBLE 8


/* Offsets and masks defining SPUR instructions. */

#define opCodePosn	25
#define destPosn	20
#define src1Posn	15
#define src2Posn	 9
#define condPosn	20
#define jumpOpCodePosn	28
#define storeHighPosn	20
#define shortImmedPosn	 9
#define tagImmedPosn	 9

#define immedFlag	0x00004000
#define immedMask	0x00003fff
#define immedSign	0x00002000
#define lowStoreMask	0x000001ff
#define highStoreMask	0x01f00000
#define shortImmedMask	0x00003e00
#define tagImmedMask	0x00007e00
#define maxUnsignedRcLit 0x00001fff
#define maxComputedLiteral 0x0007ffff
#define minComputedLiteral 0xfff80000

#define eq_tag_immed_code 0x19
#define ne_tag_immed_code 0x1d

/* Opcode modifier bits */

#define instModifierBits 0xffffff00	/* Bits that may modify an opcode, without being
					 * part of it. */
#define simulatorOpMask 0x100		/* Indicates simulator instruction. */

/* Opcodes for special-case handling of Nop */
#define nopOpcode     (0x4b | simulatorOpMask)

#define add_ntOpcode	  0x48
#define rd_specialOpcode  0x55
#define jump_regOpcode	  0x5a
#define ld_32Opcode	  0x04
#define sllOpcode	  0x45
#define xorOpcode	  0x44
#define orOpcode	  0x43

#define pc_sreg		  3

/* Symbols */

struct _symbolType {
    int id;		/* Numeric id for this symbol (ordinal symbol
			 * number for user symbols) */
    int type;		/* Symbol's type (see a.out.h for user symbols; for 
			 * special symbols, see parser.h) */
    int other, desc;	/* Values for n_other and n_desc fields. */
    struct _symbolType *link;  /* Hash table chain used for interning. */
    struct _symbolType *next;  /* symbol entry with next higher id. */
    int 	       segment; /* Region number if relative to segment. */
    unsigned int       value;  /* Numeric value if absolute, or offset */
    char string[1];     /* Symbol (variable width) */
};

typedef struct _symbolType	symbolType;

/* Expressions */
typedef enum { MANIFEST_INT, SYM_EXPR, BINARY_EXPR, UNARY_EXPR }
					exprClassType;

struct _exprType {
    exprClassType	class;
    union {
	int 		opcode;
	unsigned int	value;
	symbolType	*sym;
    } v;
    unsigned int offset;
    struct _exprType	*left, *right;
};

typedef struct _exprType	exprType;

#define nullExpr ((exprType *) NULL)

typedef enum { REG, IMMED }	operandStyleType;

struct _operandType {
    operandStyleType	type;
    exprType 		*expr;
    int 		number;
};
typedef struct _operandType	operandType;

#define ErrorMsg \
       {   \
	   errorHeader(); errorCount += 1; (void) fprintf(stderr,
   
#define WarningMsg \
       { \
	   errorHeader(); (void) fprintf(stderr,

#define EndMsg   \
       ); (void) putc('\n',stderr); }

extern int errorCount;		/* Count of number of errors. */
extern int linecount;		/* Line number in current source file */
extern char filename[];		/* Current source file name */
extern int nextId;		/* Next available symbol ordinal--number of symbols
				 * in symbol table. */
extern symbolType *firstSym; /* First user-defined symbol (i.e., one with lowest
				  * ordinal. */

extern void
    exit2();

extern exprType 
    *numToExpr(),
    *symToExpr(),
    *consUnaryExpr(),
    *consBinaryExpr(),
    *pointExpr(),
    *fwdTempLabelToExpr(),
    *bckwdTempLabelToExpr();

extern unsigned int
    emitConvertedExpr();

extern symbolType *
    newSymbol();

extern void
    errorHeader(),
    initLexer(),
    initParser(),
    initExprs(),
    assignId(),
    setGlobalSym(),
    setExternSym(),
    emitStab(),
    setAssemblerReg(),
    emitBytes(),
    emitExpr(),
    setAlignment(),
    setSymDefn(),
    SetOrg(),
    leaveSpace(),
    emitFloat(),
    DefineLabel(),
    DefineTempLabel(),
    emitReloc(),
    emitRRxInst(),
    emitStoreInst(),
    emitRxCmpInst(),
    emitCmpTagInst(),
    emitJumpInst(),
    writeObject(),
    setSymCommDefn(),
    setSymLcommDefn(),
    initSas();

/* Various C library things. */

extern char
    *malloc();
