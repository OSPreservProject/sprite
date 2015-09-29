/* program Wsltex */

    /*
    ********************************************************************
    Copy an 8-bit file  to a 7-bit file,  turning characters with bit  8
    set into 4-character octal escape sequences \nnn.  This is useful in
    analyzing  WordStar  microcomputer   word  processor  files.    Many
    sequences which  can be  recognized as  doing something  useful  are
    turned into LaTeX command sequences.   Comments  below  describe the
    sequences which are recognized and translated.

    Usage:

    @WSLTEX WordStar_inputfile >LaTeX_outputfile

    Remarks:
    This program was converted automatically  from Pascal to C by  James
    Peterson's Pascal-to-C translator  from Carnegie-Mellon  University.
    Hand editing  was required  on  all I/O  references, which  are  not
    converted satisfactorily (e.g.  the  Pascal file variable points  to
    the current input character returned by get(), while in C, one  must
    explicitly save the result of getc()).  Comment and type declaration
    formatting where also  rearranged by hand.   Defined constants  were
    changed to upper-case following the usual convenient C convention of
    visible differences between variables  and constants, and  functions
    and macros.

    The conversion of a test  188K Ph.D. thesis file produced  identical
    results with both versions.  Pascal compilation was much faster than
    the PCC compiler.  The conversion  time with the Pascal version  was
    15.5 sec.  In the C version using putc(), it was 25.1 sec;  changing
    putc() to use of write() with large buffers further reduced the time
    to 21.2 sec.

    [21-Dec-85]		conversion from Pascal
    [11-Oct-85]		original version
    ********************************************************************
    */


#define	UNIX

#include <stdio.h>
#ifdef	TOPS20
#include <file.h>		/* for TOPS-20 open magic */
#endif

#ifdef UNIX
#define WRITELN(fp) putc('\n',fp)
#else
#define WRITELN(fp) {putc('\r',fp);putc('\n',fp);}
#endif

#ifdef TOPS20
#define EMPTY_BUFFER {if (the_next > 0) \
	_write(fileno(LaTeX_file),the_buffer,the_next);\
	the_next = 0;}
#else
#define EMPTY_BUFFER {if (the_next > 0) \
	write(fileno(LaTeX_file),the_buffer,the_next);\
	the_next = 0;}
#endif

typedef int BOOLEAN;

#define FALSE 0

#define TRUE 1

#define NUL '\0'

#define BEL '\07'

#define CR '\r'

#define ESC '\033'

#define FF '\f'

#define HT '\t'

#define LF '\n'

#define MAXRING 256		/* limit for character ring */

#define MAXSPECIAL 100		/* limit for warning when special
				strings (underline, bold, etc) exceed
				this length */

#define MAXBUF 16384		/* large buffer for I/O efficiency */

#define UNSET 0x8000		/* use most-negative 16-bit integer */

FILE *WordStar_file;		/* 8-bit input file */
FILE *LaTeX_file;		/* 7-bit or 8-bit output file */

char *the_buffer;		/* the output buffer (malloc()'ed) */
int  the_next;			/* index of next empty slot in
				the_buffer[]*/

int ring_index;			/* index into ring[] */
char ring[MAXRING+1];		/* ring buffer for current output
				context */

BOOLEAN hyphen_break;		/* hyphen break flag */
BOOLEAN eol;			/* end-of-line flag */
BOOLEAN is_underline;		/* underline et al flags */
BOOLEAN is_subscript;
BOOLEAN is_superscript;
BOOLEAN is_bold;
BOOLEAN is_alternate;

int bi_eol;			/* input byte offset at end-of-line */
int bi_underline;
int bi_subscript;
int bi_superscript;
int bi_bold;
int bi_alternate;

int bo_eol;			/* output byte offset at end-of-line */
int bo_underline;
int bo_subscript;
int bo_superscript;
int bo_bold;
int bo_alternate;

int input_byte_offset;
int output_byte_offset;
int last_c;			/* previous input character */

/* global functions */

#ifdef TOPS20
#define Begin_alternate Beg_alternate	/* unique to 6 characters */
#define Begin_boldface  Beg_boldface
#endif

void Begin_alternate ();
void Begin_boldface ();
void End_alternate ();
void End_boldface ();
void Line_feed ();
void Superscript ();
void Subscript ();
void Underline ();
void Warning ();
void W_byte ();
void W_char ();
void W_octal ();
void W_string ();

char *strchr();


/**********************************************************************/
/*-->main */
main(argc,argv)			/* Main -- WSLTEX */
int argc;
char *argv[];
{
    int fp;
    register int c;		/* current input character */
    register int c7;		/* low-order 7 bits of c */

    if (argc < 2)
    {
        fprintf(stderr,
        "%%Usage: %s WordStar_infile >LaTeX_outfile",argv[0]);
        WRITELN(stderr);
        exit(1);
    }

    /* only 7-bit bytes required, so stdout is okay to use provided
    it does not do CRLF <--> LF translation.  On TOPS-20, this is
    suppressed by loading this program with a special version of the I/O
    library. */

    LaTeX_file = stdout;

#ifdef TOPS20
    /* Read file in bytesize 8 with binary flag set to prevent
       CRLF <--> LF mapping; the "rb" or "wb" flag is not sufficient for
       this because PCC-20 maintains two places internally
       where the binary flag is set, and both are used!  */

    if ((fp =
        open(argv[1], FATT_RDONLY | FATT_SETSIZE | FATT_BINARY,8)) >= 0)
        WordStar_file = fdopen(fp,"rb");
    else
        WordStar_file = (FILE *)NULL;
#else
    WordStar_file = fopen(argv[1],"rb");
#endif

    if (WordStar_file == (FILE *)NULL)
    {

        fprintf(stderr,
        "?Open failure on WordStar input file [%s]",argv[1]);
        WRITELN(stderr);
        exit(1);
    }

    /* Initializations -- in alphabetical order */

    bi_alternate = -1;
    bi_bold = -1;
    bi_subscript = -1;
    bi_superscript = -1;
    bi_underline = -1;
    bo_alternate = -1;
    bo_bold = -1;
    bo_subscript = -1;
    bo_superscript = -1;
    bo_underline = -1;
    eol = TRUE;
    hyphen_break = FALSE;
    input_byte_offset = -1;
    is_bold = FALSE;
    is_subscript = FALSE;
    is_superscript = FALSE;
    is_underline = FALSE;
    last_c = -1;
    output_byte_offset = -1;

    for (ring_index=0; ring_index <= MAXRING; ring_index++)
        ring[ring_index] = ' ';
    ring_index = 0;

    if ((the_buffer = (char *)malloc(MAXBUF)) == (char *)NULL)
    {
        fprintf(stderr,
	    "?Unable to allocate output buffer of size %d",MAXBUF);
	WRITELN(stderr);
	exit(1);
    }
    the_next = 0;

   /*
   =====================================================================
   WordStar makes heavy use of the 8-th (high-order) bit of 8-bit ASCII
   characters.  In general, it turns on that bit for
   -- any character (0..127) at end-of-word
   -- any character (0..127) at end-of-line
   -- "hard" line break on CTL-M
   -- "hard" page break on CTL-L

   A discretionary hyphen (which can be removed on joining the lines) at
   end-of-line is encoded as "-\215" (i.e. "-<\200|CTL-M>").

   A required hyphen  falling at end-of-line  is encoded as  "-\040\215"
   (i.e.  followed by a space).

   Comment lines and WordStar command lines begin with a dot.

   "Hard"    line    breaks    are    encoded    as    \215\015    (i.e.
   "<\200|CTL-M><CTL-J>"); these occur between double-space lines.

   "Soft" line breaks are encoded as <CTL-M><CTL-J>; these occur at
   paragraph ends.

   Centered text is stored centered without any special markings.

   Underlined text is bracketed by CTL-S.

   Boldface text is bracketed by CTL-E ..text.. CTL-R.

   Superscripts are bracketed by CTL-T.

   Subscripts are bracketed by CTL-V.

   Alternate typewheel strings appear as CTL-Q ..text.. CTL-W.  Some of
   these at least are mnemonic (e = epsilon, m = mu, . = centered dot).

   CTL-R also appears after all sub/superscript sequences, apparently
   functioning as a "return to normal" state.
   =====================================================================
   */

    while ((c = getc(WordStar_file)) != EOF)
    {
        if (c < 32) /* ordinary control character */
        {
            switch (c)
            {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:	/* CTL-@ .. CTL-D */
                W_octal(c);
                break;

            case 5:	/* CTL-E */
                Begin_boldface();
                break;

            case 6:
            case 7:
            case 8:	/* CTL-F .. CTL-H */
                W_octal(c);
                break;

            case 9:	/* CTL-I */
                W_byte(HT);
                break;

            case 10:	/* CTL-J */
                Line_feed();
                break;

            case 11:	/* CTL-K */
                W_octal(c);
                break;

            case 12:	/* CTL-L */
                W_byte(FF);
                break;

            case 13:	/* CTL-M */
                if (last_c == (int)('-'))
                    hyphen_break = TRUE;
                else if (!hyphen_break)
                    W_byte(CR);
                break;

            case 14:
            case 15:
            case 16:	/* CTL-N .. CTL-P */
                W_octal(c);
                break;

            case 17:	/* CTL-Q */
                Begin_alternate();
                break;

            case 18:	/* CTL-R */
                if (is_bold)
                    End_boldface();
                /* ELSE discard character */
                break;

            case 19:	/* CTL-S */
                Underline();
                break;

            case 20:	/* CTL-T */
                Superscript();
                break;

            case 21:	/* CTL-U */
                W_octal(c);
                break;

            case 22:	/* CTL-V */
                Subscript();
                break;

            case 23:	/* CTL-W */
                End_alternate();
                break;

            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:	/* CTL-X .. CTL-_ */
                W_octal(c);
                break;
            }
            /* case */
        }
        else if (strchr("#$%&~_^\\{}",c))
            /* LaTeX special characters must be quoted by backslash */
        {
            W_char('\\');
            W_byte(c);
        }
        else if (c == (int)('.'))	/* check for WordStar comment */
        {
            if (eol)
                W_string("%-WS-% ");
            W_byte(c);
        }
        else if (c < 128) /* c in 32 .. 127 */
            W_byte(c);
        else  /* c > 127 */
        {
            c7 = c & 0177;	/* lop off 8-th bit */
            switch (c7)
            {
            case 0:	/* CTL-@ */
                W_octal(c7);
                break;

            case 1:	/* CTL-A */
                W_octal(c7);
                break;

            case 2:	/* CTL-B */
                W_octal(c7);
                break;

            case 3:	/* CTL-C */
                W_octal(c7);
                break;

            case 4:	/* CTL-D */
                W_octal(c7);
                break;

            case 5:	/* CTL-E */
                Begin_boldface();
                break;

            case 6:	/* CTL-F */
                W_octal(c7);
                break;

            case 7:	/* CTL-G */
                W_octal(c7);
                break;

            case 8:	/* CTL-H */
                W_octal(c7);
                break;

            case 9:	/* CTL-I */
                W_byte(HT);
                break;

            case 10:	/* CTL-J */
                Line_feed();
                break;

            case 11:	/* CTL-K */
                W_octal(c7);
                break;

            case 12:	/* CTL-L */
                W_byte(FF);
                break;

            case 13:	/* CTL-M */
                if (last_c == (int)('-'))
                    hyphen_break = TRUE;
                else if (!hyphen_break)
                {
                    if (bo_eol < output_byte_offset)
                        W_byte(CR);
                    /* not double space */
                }
                break;

            case 14:	/* CTL-N */
                W_octal(c7);
                break;

            case 15:	/* CTL-O */
                W_octal(c7);
                break;

            case 16:	/* CTL-P */
                W_octal(c7);
                break;

            case 17:	/* CTL-Q */
                Begin_alternate();
                break;

            case 18:	/* CTL-R */
                if (is_bold)
                    End_boldface();
                /* ELSE discard character */
                break;

            case 19:	/* CTL-S */
                Underline();
                break;

            case 20:	/* CTL-T */
                Superscript();
                break;

            case 21:	/* CTL-U */
                W_octal(c7);
                break;

            case 22:	/* CTL-V */
                Subscript();
                break;

            case 23:	/* CTL-W */
                End_alternate();
                break;

            case 24:	/* CTL-X */
                W_octal(c7);
                break;

            case 25:	/* CTL-Y */
                W_octal(c7);
                break;

            case 26:	/* CTL-Z */
                W_octal(c7);
                break;

            case 27:	/* CTL-[ */
                W_octal(c7);
                break;

            case 28:	/* CTL-\ */
                W_octal(c7);
                break;

            case 29:	/* CTL-] */
                W_octal(c7);
                break;

            case 30:	/* CTL-^ */
                W_octal(c7);
                break;

            case 31:	/* CTL-_ */
                W_octal(c7);
                break;

            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            case 38:
            case 39:
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
                W_byte(c7);
                break;

            case 46:	/* period */
                if (eol)
                    W_string("%-WS-% ");
                W_byte(c7);
                break;

            case 47:
            case 48:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
            case 72:
            case 73:
            case 74:
            case 75:
            case 76:
            case 77:
            case 78:
            case 79:
            case 80:
            case 81:
            case 82:
            case 83:
            case 84:
            case 85:
            case 86:
            case 87:
            case 88:
            case 89:
            case 90:
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
            case 98:
            case 99:
            case 100:
            case 101:
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107:
            case 108:
            case 109:
            case 110:
            case 111:
            case 112:
            case 113:
            case 114:
            case 115:
            case 116:
            case 117:
            case 118:
            case 119:
            case 120:
            case 121:
            case 122:
            case 123:
            case 124:
            case 125:
            case 126:
                W_byte(c7);
                break;

            case 127:
                W_octal(c7);
                break;

            default:
                W_octal(c7);
                break;
            }            	/* CASE */
        }
        last_c = c;
        input_byte_offset++;
    }
    EMPTY_BUFFER;
}
/* Main -- WSLTEX */


/**********************************************************************/
/*-->Begin_alternate */
void
Begin_alternate ()
{
    if (is_alternate)
        Warning("BEGIN ALTERNATE CHARACTER SET command encounted inside\
 alternate sequence",
        bo_alternate,output_byte_offset);
    bi_alternate = input_byte_offset;
    bo_alternate = output_byte_offset;
    is_alternate = TRUE;
    W_string("{\\alt ");
}
/* Begin_alternate */


/**********************************************************************/
/*-->Begin_boldface */
void
Begin_boldface ()
{
    if (is_bold)
        Warning("BEGIN BOLDFACE command encountered inside boldface",
        bo_bold,output_byte_offset);
    bi_bold = input_byte_offset;
    bo_bold = output_byte_offset;
    is_bold = TRUE;
    W_string("{\\bf ");
}
/* Begin_boldface */

/**********************************************************************/
/*-->End_alternate */
void
End_alternate ()
{
    if (is_alternate)
    {
        if ((bo_alternate + MAXSPECIAL) < output_byte_offset)
            Warning("Long ALTERNATE CHARACTER SET sequence",
            bo_alternate,output_byte_offset);
    }
    else
        Warning("END ALTERNATE CHARACTER SET command encountered\
 outside alternate text",bo_alternate,output_byte_offset);
    is_alternate = FALSE;
    W_byte((int)('}'));
}
/* End_alternate */


/**********************************************************************/
/*-->End_boldface */
void
End_boldface ()
{
    if (is_bold)
    {
        if ((bo_bold + MAXSPECIAL) < output_byte_offset)
            Warning("Long BOLDFACE sequence",
            bo_bold,output_byte_offset);
    }
    else
        Warning("END BOLDFACE command encountered outside boldface",
        bo_bold,output_byte_offset);
    is_bold = FALSE;
    W_byte((int)('}'));
}
/* End_boldface */


/**********************************************************************/
/*-->Line_feed */
void
Line_feed ()
{
    if (hyphen_break)		/* discard line break */
        hyphen_break = FALSE;
    else
    {
        if (bo_eol < output_byte_offset)	/* text on line */
            W_byte(LF);
        else  /* empty line */
        {
            if (last_c != (0200 | CR))
                W_byte(LF);
        }
    }
}
/* Line_feed */


/**********************************************************************/
/*-->strchr() */
char *
strchr(s,c)	/* return pointer to c in s[], or (char*)NULL */
char *s;
char c;
{
    while (*s && (*s != c))
        ++s;
    if (*s == c)
        return (s);
    else
        return ((char *)NULL);
}
/* strchr */


/**********************************************************************/
/*-->Superscript */
void
Superscript ()
{
    if (is_superscript)
    /* ending old superscript */
    {
        if ((bo_superscript + MAXSPECIAL) < output_byte_offset)
            Warning("Long SUPERSCRIPT sequence",
            bo_superscript,output_byte_offset);
        W_byte((int)('}'));
    }
    else  /* beginning new superscript */
    {
        bi_superscript = input_byte_offset;
        bo_superscript = output_byte_offset;
        W_string("^{");
    }
    is_superscript = !is_superscript;
}
/* Superscipt */


/**********************************************************************/
/*-->Subscript */
void
Subscript ()
{
    if (is_subscript)
    /* ending old superscript */
    {
        if ((bo_subscript + MAXSPECIAL) < output_byte_offset)
            Warning("Long SUBSCRIPT sequence",
            bo_subscript,output_byte_offset);
        W_byte((int)('}'));
    }
    else  /* beginning new superscript  */
    {
        bi_subscript = input_byte_offset;
        bo_subscript = output_byte_offset;
        W_string("_{");
    }
    is_subscript = !is_subscript;
}
/* Subscript */


/**********************************************************************/
/*-->Underline */
void
Underline ()
{
    if (is_underline)
    /* ending old underline */
    {
        if ((bo_underline + MAXSPECIAL) < output_byte_offset)
            Warning("Long UNDERLINE sequence",
            bo_underline,output_byte_offset);
        W_byte((int)('}'));
    }
    else  /* beginning new underline */
    {
        bi_underline = input_byte_offset;
        bo_underline = output_byte_offset;
        W_string("{\\em ");
    }
    /* underlined text is \emphasized in LaTeX */
    is_underline = !is_underline;
}
/* Underline */


/**********************************************************************/
/*-->Warning */
void
Warning (s,b1,b2)
char s[];
int b1, b2 /* output byte range, or UNSET,UNSET */;
{
    int k;
    WRITELN(stderr);   /* !!!! temporary for debugging !!!! */
    WRITELN(stderr);   /* !!!! temporary for debugging !!!! */
    fprintf(stderr,
    "------------------------------------------------------------");
    WRITELN(stderr);
    fprintf(stderr,"%s",s);
    WRITELN(stderr);
    if (b1 != UNSET)
    {
        fprintf(stderr,"\toutput byte range  = %d .. %d",b1,b2);
        WRITELN(stderr);
    }
    fprintf(stderr,"\tinput byte offset  = %d",input_byte_offset);
    WRITELN(stderr);
    fprintf(stderr,"\toutput byte offset = %d",output_byte_offset);
    WRITELN(stderr);
    fprintf(stderr,"\tcurrent context = [");
    WRITELN(stderr);
    for (k=ring_index; k <= MAXRING; k++)
        putc(ring[k],stderr);
    for (k=0; k <= ((MAXRING + ring_index - 1) % MAXRING); k++)
        putc(ring[k],stderr);
    WRITELN(stderr);
    fprintf(stderr,"\t]");
    WRITELN(stderr);
    fprintf(stderr,
    "------------------------------------------------------------");
    WRITELN(stderr);
}
/* Warning */


/**********************************************************************/
/*-->W_byte */
void
W_byte (c)
int c;
{
    ring[ring_index] = (char)c;
    ring_index = (ring_index++) % MAXRING;

    /* All output bytes go through this single piece of code, so
    we optimize output by buffering it in large blocks.  The use
    of write() instead of fwrite() avoids CR LF <--> LF translation
    on most machines, and on some is significantly more efficient. */

    if (the_next >= MAXBUF)
        EMPTY_BUFFER;
    the_buffer[the_next++] = c;

    eol = (c == LF);
    output_byte_offset++;
    if (eol)
    {
        bi_eol = input_byte_offset;
        bo_eol = output_byte_offset;
    }
}
/* W_byte */


/**********************************************************************/
/*-->W_char */
void
W_char (c)
char c;
{
    W_byte((int)(c));
}
/* W_char */


/**********************************************************************/
/*-->W_octal */
void
W_octal (c)
int c;
{
    c = c & 0377;
    W_byte((int)('\\'));
    W_byte((c >> 6) + (int)('0'));
    W_byte(((c >> 3) & 07) + (int)('0'));
    W_byte((c & 07) + (int)('0'));
}
/* W_octal */


/**********************************************************************/
/*-->W_string */
void
W_string (s)
char *s;
{
    while (*s)
        W_byte((int)(*s++));
}
/* W_string */
