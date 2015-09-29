/*  $Header: a.out.h,v 2.2 88/09/28 00:04:58 hilfingr Exp $ */

/* Format of a.out files, as seen by as, ld, and the kernel.  The kernel's
 * knowledge is restricted to the contents of sys/exec.h. 
 */

#include "sys/exec.h"

/*
 * memory management parameters
 */

#define PAGSIZ		0x1000

/*
 * returns 1 if an object file type is invalid, i.e., if the other macros
 * defined below will not yield the correct offsets.  Note that a file may
 * have N_BADMAG(x) = 0 and may be fully linked, but still may not be
 * executable.
 */

#define	N_BADMAG(x) \
	((x).a_magic!=ZMAGIC && (x).a_magic!=OMAGIC)

/*
 * relocation parameters. These are architecture-dependent
 * and can be deduced from the machine type.  They are used
 * to calculate offsets of segments within the object file;
 * See N_TXTOFF(x), etc. below.
 */

#define N_PAGSIZ(x) PAGSIZ

/*
 * offsets of various sections of an object file.
 */


#define	N_TXTOFF(x) \
	((x).a_magic == ZMAGIC ? 0 : sizeof (struct exec))

#define N_DATAOFF(x) \
        (N_TXTOFF(x)+(x).a_text)

#define N_SDATAOFF(x) \
        (N_DATAOFF(x)+(x).a_data)

#define N_RELOCOFF(x) \
        (N_SDATAOFF(x)+(x).a_sdata)

#define N_EXPOFF(x) \
	(N_RELOCOFF(x)+(x).a_rsize)

#define	N_SYMOFF(x) \
	/* symbol table */ \
	(N_EXPOFF(x)+(x).a_expsize)

#define	N_STROFF(x) \
	/* string table */ \
	(N_SYMOFF(x) + (x).a_syms)

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */

#define N_TXTADDR(x) \
    ((x).a_magic == ZMAGIC ? 0x40000000 : 0)

#define N_DEFENT(x) \
    (N_TXTADDR(x) + ((x).a_magic == ZMAGIC ? sizeof(struct exec) : 0))

#define N_DATADDR(x) \
    ((x).a_magic == ZMAGIC ? 0xc0000000 : N_TXTADDR(x) + (x).a_text)

#define N_BSSADDR(x)  (N_DATADDR(x)+(x).a_data)

#define N_SDATADDR(x) 0x80000000

#define N_SBSSADDR(x) (N_SDATADDR(x) + (x).a_sdata)


/*
 * Format of a relocation datum.
 */

struct relocation_info {
unsigned int r_address:27,  /* offset in segment of quantity to be */
                            /* relocated. */
             r_segment:4,   /* segment to be used as the base */
                            /* for r_address. */
             r_dummy:1;     /* padding. */
unsigned int r_expr:24,	    /* offset in expression data if r_reltype 
			     * is RP_REXP; 
			     * symbol ordinal, if r_reltype is RP_RSYM;
			     * or one of the values N_TEXT, N_DATA, N_SDATA,
			     * N_BSS, or N_SBSS, if r_reltype is RP_RSEG.
			     */
             r_word:1,      /* 1 iff location requires word address */
             r_reltype:2,   /* operation to perform */
             r_length:3,    /* bits to be modified */
             r_extra:2;     /* Must be 0. */
};

/* Values for r_reltype */
#define RP_RSEG  0      /* add relocation increment of segment r_expr. */
#define RP_RSYM  1      /* add value of symbol r_expr. */
#define RP_REXP  2      /* set to expression indexed by r_expr. */

/* Values for r_length */
/* Note: the address of the modified data must be a multiple of 4 */
/* except when r_length is RP_LOW16, in which case is must be even, */
/* or when r_length is RP_LOW8, when there is no restriction. */
#define RP_LOW8  0      /* low-order 8 bits */
#define RP_LOW9  1      /* low-order 9 bits */
#define RP_LOW14 2      /* low-order 14 bits */
#define RP_LOW16 3      /* low-order 16 bits */
#define RP_SCONS 4      /* store constant (bits 0-8 and 20-24) */
#define RP_LOW28 5      /* low-order 28 bits */
#define RP_32    6      /* full long word */

/* Values for r_segment */
#define RP_TEXT0   0    /* Text segment */
#define RP_DATA0   1    /* Private data segment */
#define RP_SDATA0  2    /* Shared data segment */
#define RP_SYMBOLS 3    /* Symbol table */
#define RP_BSS	   4    /* Unitialized private data segment */
#define RP_SBSS    5    /* Uninitialized shared data segment */
#define RP_SEG0    6    /* Symbol 0. */
#define RP_SEGLAST 15   /* Last available segment number. */

/*
 * Definitions of expressions.
 *
 * Expressions are stored in prefix form as a sequence of reloc_exprs.  The
 * operators are described in the #defines below.  
 */

union reloc_expr {
    unsigned int 	re_value;	/* Unsigned integer value, following an
					 * EO_INT, EO_SYM, EO_TEXT, EO_DATA, or
					 * EO_BSS syllable. */
    struct {
	unsigned int	re_op:8,	/* Operator (see EO_xxx below). */
                        re_arg:24;	/* Argument modifying EO_SYM. */
    } re_syl;
};
    
    /* Constructor */
#define EO_SET_SYL2(x,op,arg) { (x).re_syl.re_op = (op); (x).re_syl.re_arg =(arg); }
#define EO_SET_SYL(x,op) { (x).re_syl.re_op = (op); }

/* Values of re_op.  In the following, x is the re_value of the */
/* following syllable. */
#define EO_INT   0      /* x */
#define EO_SYM   1      /* x + symbol at index re_arg in symbol table. */
#define EO_TEXT  4      /* x + base of text segment. */
#define EO_DATA  5      /* x + base of private data segment. */
#define EO_BSS   6      /* x + base of uninitialized data. */
#define EO_SDATA 7      /* x + base of shared data segment. */
#define EO_SBSS  8      /* x + base of shared uninitialized data. */

/* Unary operators: x is value of the following expression. */
#define EO_MINUS 9       /* -x */
#define EO_COMP  10      /* !x */

/* Binary operators: x and y are the values of the following two */
/* expressions. */
#define EO_PLUS 11      /* x+y */
#define EO_MULT 12      /* x*y */
#define EO_DIV  13      /* x/y (integer division) */
#define EO_SLL  14      /* logical left shift. */
#define EO_SRL  15      /* logical right shift */
#define EO_AND  16      /* logical bitwise and */
#define EO_OR   17      /* logical bitwise or */
#define EO_XOR  18      /* logical bitwise exclusive or */
#define EO_SUB  19      /* x-y */

/*
 * Format of a symbol table entry
 */
struct  nlist {
    union {
            char    *n_name;        /* convenience only --  */
                                    /* unused in object file */
            long    n_strx;         /* index into string table */
    } n_un;
    unsigned char   n_type;         /* type flag, see below  */
    char    n_other;                /* Must be 0 */
    short   n_desc;                 /* debugging data, q.v. */
    unsigned long   n_value;        /* value (or offset) of symbol,
                                     * length of common area. */
};


/* Values for n_type field */
#define N_UNDF  0x0             /* undefined */
#define N_ABS   0x2             /* absolute */
#define N_TEXT  0x4             /* text-relative */
#define N_DATA  0x6             /* data-relative */
#define N_BSS   0x8             /* bss-relative */
#define N_SDATA 0xa             /* shared data-relative */
#define N_SBSS  0xe             /* shared bss-relative */
#define N_EXPR  0x10            /* value to be filled in by load-time */
                                /* expression */
#define N_COMM  0x12            /* common region: n_value gives length */
#define N_SCOMM 0x14		/* shared common region: n_value gives
				 * length */

#define N_EXT   01              /* external bit, or'ed in to indicate */
                                /* imported or exported definitions */
#define N_TYPE  0x1e            /* mask for all the type bits */

/*
 * Dbx entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */
#define	N_STAB	0xe0		/* if any of these bits set, a dbx symbol */

/*
 * Format for namelist values.
 */
#define	N_FORMAT	"%08x"
