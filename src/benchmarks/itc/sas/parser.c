# define LSHIFT 257
# define RSHIFT 258
# define EOLN 259
# define ID 260
# define WORDID 261
# define INTCONST 262
# define FLOATCONST 263
# define STRINGCONST 264
# define CONDITION 265
# define INTREG 266
# define FLOATREG 267
# define SPECIALREG 268
# define CACHEREG 269
# define CACHEOP 270
# define TEMPLABEL 271
# define FWDTEMPREF 272
# define BCKWDTEMPREF 273
# define FWDNUMTEMPREF 274
# define BCKWDNUMTEMPREF 275
# define rrx_form 276
# define rrr_form 277
# define rr0_form 278
# define r2r_form 279
# define r00_form 280
# define zzz_form 281
# define r0f_form 282
# define r0f_form_reverse 283
# define frx_form 284
# define fff_form 285
# define ff0_form 286
# define fstore_form 287
# define ff_cmp_form 288
# define zrx_form 289
# define zzx_form 290
# define store_form 291
# define r_store_form 292
# define rx_cmp_form 293
# define cmp_tag_form 294
# define rx_cmp_trap_form 295
# define cmp_tag_trap_form 296
# define jump_form 297
# define external_form 298
# define external_store_form 299
# define undef_form 300
# define _globl 301
# define _extern 302
# define _long 303
# define _word 304
# define _byte 305
# define _single 306
# define _double 307
# define _ascii 308
# define _asciz 309
# define _align 310
# define _comm 311
# define _lcomm 312
# define _scomm 313
# define _slcomm 314
# define _text0 315
# define _text1 316
# define _text2 317
# define _data0 318
# define _data1 319
# define _data2 320
# define _sdata0 321
# define _sdata1 322
# define _set 323
# define _org 324
# define _space 325
# define _stabs 326
# define _stabn 327
# define _stabd 328

# line 23 "parser.y"
#include <stdio.h>
#include <sys/file.h>
#include "sas.h"
#include "a.out.h"

static char *rcsid = 
    "$Header: parser.y,v 3.9 88/10/18 16:34:17 hilfingr Exp $";

static struct _operandType 
     _r0 = { REG, 0 };

static int list_oper;
		/* Indicates bytes of data expected in list. */

int currentSegment;		/* Current region number into which 
				 * instructions are being assembled.  Possible
				 * values in #defines just above. */

bool haveWarnedAboutRegStores = FALSE;

# line 44 "parser.y"
typedef union  {
	unsigned int	num;
	struct { char *str; int len; }
			string;
	symbolType      *sym;
        operandType	operand;
	exprType	*expr;
} YYSTYPE;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 416 "parser.y"


#include <stdio.h>

int
yyerror(s)
     char *s;
{
    ErrorMsg "%s.", s EndMsg;
}

void
initParser()
{
}

short yyexca[] ={
-1, 0,
	0, 1,
	-2, 3,
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 2,
	-2, 5,
	};
# define YYNPROD 160
# define YYLAST 594
short yyact[]={

  15, 311,  93,  48,  50,  52,  56,  57,  58,  54,
  55,  49,  51,  53,  62,  64,  59,  60,  61, 298,
  63,  65,  66,  67,  68, 294, 125, 290, 103, 239,
 124, 123, 122, 121,  91,  80,  79, 255, 134,  81,
  15,  89,  87,  90, 296, 101,  86,  89, 105,  85,
 101, 101, 105, 118, 101,  89,  84,  83,  82,  89,
  73,  72,   7,  88,  96,  89, 151, 133, 129,  89,
  99, 126, 316,  78,  95, 308, 307, 130, 130, 130,
 107, 306, 109,  71,  15, 304, 303, 302, 278, 277,
 276, 120, 158, 299, 275, 274, 148, 166, 159, 257,
 144, 142,   9, 143, 273, 145, 272, 271, 270, 267,
  46, 265, 264, 263,  69,  70, 262, 261, 260, 259,
 258, 208, 217, 216, 215, 214, 213, 212, 266, 211,
 210, 209, 207, 206, 205, 204,  92,  94, 203, 218,
 219, 220, 221, 222, 131, 132, 135, 202, 201, 200,
  97, 198, 150, 196, 195, 194, 193, 115, 192, 140,
 116, 226, 139, 138, 137, 136,   5,  77,  76,  75,
  74,   8,  13,  11,  47,  10,   6,   4, 167,   3,
   2,   1, 149, 117, 127, 160, 141, 165, 153,   0,
   0,   0,   0,   0,   0, 197,   0,   0,   0, 199,
  14,  12,   0,  44,   0, 225,   0, 241, 241,  98,
   0, 106,  45, 108,   0, 110, 111, 112, 113, 254,
 114,   0, 119,   0,   0,   0,   0, 227,   0,   0,
 235,   0, 238, 240, 244, 243,   0,   0,   0,   0,
  14, 250,  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
 241, 223, 224, 237, 100, 291, 291, 242, 104, 100,
 100, 102, 104, 100,  14, 292, 228, 229, 230, 242,
 128,   0, 256,   0, 286,   0,   0, 287, 301, 288,
 289, 293,   0, 300, 297,   0, 310, 312, 313, 312,
 314, 315, 152,   0, 161, 146, 147, 164,   0, 317,
   0, 163, 162,   0, 154, 155, 156, 157, 168, 169,
 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
 190, 191, 279, 280, 281,   0, 231, 232,   0, 233,
   0, 234,   0, 236, 245, 246, 268, 269,   0,   0,
 247, 248, 249, 148, 251, 252, 253, 144, 142,   0,
 143,   0, 145,   0,   0,   0,   0,   0,   0,   0,
 305,   0,   0,   0,   0,   0, 309,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 282,
 283, 284,   0,   0,   0, 285,   0,   0,   0, 150,
   0,   0,   0,   0,   0, 295,   0,   0, 295,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 149,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 146, 147 };
short yypact[]={

-1000,-1000,-1000,-194,-194,-1000, -59, -19,-1000,-1000,
-273, -19,  25,-1000,-1000,-1000,-199,-200,-1000,-1000,
-1000,-1000,-1000,-228,-229,-223,-202,-203,-204,-211,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-214,  33,
  33,-230,  33,  33,  16,   6,-1000, -19,  13,  15,
  14,  15,  14,  15,  14,  14,  14,  14,-1000,  14,
  17,  14,  15,-232,-233,-234,-235,-239,  29,-1000,
-1000,-1000,-1000,-1000,  33,  33,  33,-225,-225,-1000,
-1000,-1000, 121, 120, 119, 118, 115, 335,  52,-1000,
 335, 114, 112, 335, 111,-1000,-1000,-1000, 110, 109,
-1000,  33,-1000, 107,-1000,  33, 105, 104, 103,  94,
  91,  90,  89,-1000,  88,-1000,-1000,-1000,  33,  87,
  86,  85,  83,  82,  81,  80, 335,-1000,-1000,  79,
 335,  79,  79,  78,-1000,  78,  33,  33,  33,  33,
   5,  33,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  33,-1000,
  52,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,  33,  33,  33,  14,  14,-1000,  14,-1000,
  14,  15,  13,  15,  11,  11,  17,  17, 335,  14,
  14,  14,  15,  14,  14,  14,  33,-226, 335, 335,
 335, 335, 335,-1000,-1000,  52,  58,-1000,  76,  75,
  74,  73,  72,  69,  68,  67,  65,  65,  65,  64,
-1000, 335,-1000,  63,-1000,-1000,-1000,  62,  60,  51,
  50,  46,  45,  44, 335,-1000,-1000,-1000,  33,  33,
  33,  17,  17,  17,  14,  15,-1000,  23,-1000,-1000,
  15,  15,  19,  19,   8,  15,  33,   8,  33,  43,
  42,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 335,-1000,-1000,  41,-1000,  33,-1000,  37,-1000,
  32,  31,  33,  33,  33,-1000,  33,  33,  33,  28,
 335,-1000, 335,-1000, 335, 335,  33, 335 };
short yypgo[]={

   0, 188, 187,  93, 186, 185, 157,  25, 160,  70,
  28,   2,  66,  27,  19,   1, 184, 183, 181, 180,
 179, 166, 177, 176, 102, 175, 174, 173, 172, 128,
  29,  63, 170,  68, 169, 168, 167,  67,  73 };
short yyr1[]={

   0,  18,  18,  20,  19,  22,  19,  21,  25,  21,
  21,  21,  21,  24,  24,  23,  23,  28,  28,  28,
  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,
  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,
  26,  26,  26,  26,  26,  26,  26,  26,   8,   8,
  10,  10,   9,   6,   6,  14,  13,  13,   7,   7,
  15,  17,  31,  31,  29,  29,  30,  30,  11,  11,
  12,  12,  12,  12,  12,  12,  12,  12,  12,   1,
   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   4,   4,
   4,   4,   4,   4,   4,   4,   4,   5,   5,  16,
  27,  27,  32,  27,  34,  27,  35,  27,  36,  27,
  38,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  33,  33,  37,  37,   3 };
short yyr2[]={

   0,   0,   1,   0,   2,   0,   3,   2,   0,   4,
   3,   3,   2,   1,   1,   0,   2,   2,   2,   2,
   6,   6,   6,   6,   6,   5,   5,   5,   6,   4,
   6,   4,   4,   2,   1,   4,   2,   6,   6,   8,
   6,   8,   8,   8,   6,   6,   2,   2,   1,   2,
   1,   2,   1,   1,   1,   1,   1,   1,   1,   2,
   1,   2,   1,   0,   0,   2,   1,   1,   2,   4,
   1,   1,   1,   1,   1,   1,   3,   1,   2,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   2,   0,   3,   0,   3,   0,   3,   0,   3,
   0,   3,   2,   2,   2,   4,   4,   4,   4,   1,
   1,   1,   1,   1,   1,   1,   1,   4,   4,   4,
   2,   2,  10,   8,   6,   1,   3,   1,   3,   1 };
short yychk[]={

-1000, -18, -19, -20, -22, -21, -23, 256, -21, -24,
 -25, -27, 260, -28, 259,  59, 301, 302, 303, 304,
 305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
 315, 316, 317, 318, 319, 320, 321, 322, 323, 324,
 325, 326, 327, 328, 262, 271, -24, -26, 276, 284,
 277, 285, 278, 286, 282, 283, 279, 280, 281, 289,
 290, 291, 287, 293, 288, 294, 295, 296, 297, -24,
 -24,  58, 260, 260, -32, -34, -35, -36, -38, 264,
 264, 262, 260, 260, 260, 260, 260, -11, -31,  36,
 -11, 264,  -3, -11,  -3,  58,  58, -24,  -8,  -9,
 266,  37, 268, -10, 267,  37,  -8, -10,  -8, -10,
  -8,  -8,  -8,  -8,  -8,  -6,  -8, -17,  36,  -8,
 -10, 265, 265, 265, 265, 265, -11, -16, 261, -33,
 -11, -33, -33, -37, 263, -37,  44,  44,  44,  44,
  44,  -4,  43,  45,  42,  47, 257, 258,  38, 124,
  94, -12, 260,  -1, 272, 273, 274, 275,  40,  46,
  -5, 262, 270, 269, 265,  -2,  45, 126, 276, 277,
 278, 279, 280, 281, 282, 283, 284, 285, 286, 287,
 288, 289, 290, 291, 292, 293, 294, 295, 296, 297,
 298, 299,  44,  44,  44,  44,  44,  -3,  44,  -3,
  44,  44,  44,  44,  44,  44,  44,  44, -11,  44,
  44,  44,  44,  44,  44,  44,  44,  44, -11, -11,
 -11, -11, -11, 266, 267, -31, -11, -12,  -3,  -3,
  -3,  -8,  -8,  -8,  -8, -10,  -8,  -9, -10, -30,
 -10, -11, 266, -30, -10,  -6,  -6,  -8,  -8,  -8,
 -10,  -8,  -8,  -8, -11, 263, -12,  41,  44,  44,
  44,  44,  44,  44,  44,  44, -29,  44, -29, -29,
  44,  44,  44,  44,  44,  44,  44,  44,  44,  -3,
  -3,  -3,  -6,  -6,  -6,  -8, -10, -30, -10, -10,
 -13, -11, 266, -13,  -7,  -8,  36, -10, -14,  -3,
  -7, -14,  44,  44,  44,  -3,  44,  44,  44,  -3,
 -11, -15, -11, -15, -11, -11,  44, -11 };
short yydef[]={

  -2,  -2,  -2,  15,  15,   4,   8,   0,   6,   7,
   0,   0,   0,  16,  13,  14,   0,   0, 122, 124,
 126, 128, 130,   0,   0,   0,   0,   0,   0,   0,
 139, 140, 141, 142, 143, 144, 145, 146,   0,  63,
  63,   0,  63,  63,   0,   0,  12,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  34,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  63,  10,
  11,  17, 120, 121,  63,  63,  63,   0,   0, 132,
 133, 134,   0,   0,   0,   0,   0, 150,   0,  62,
 151,   0,   0, 159,   0,  18,  19,   9,   0,   0,
  48,  63,  52,   0,  50,  63,   0,   0,   0,   0,
   0,   0,   0,  33,   0,  36,  53,  54,  63,   0,
   0,   0,   0,   0,   0,   0,  46,  47, 119, 123,
 155, 125, 127, 129, 157, 131,  63,  63,  63,  63,
  63,  63, 108, 109, 110, 111, 112, 113, 114, 115,
 116,  68,  70,  71,  72,  73,  74,  75,  63,  77,
   0,  79,  80,  81,  82,  83, 117, 118,  84,  85,
  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99, 100, 101, 102, 103, 104, 105,
 106, 107,  63,  63,  63,   0,   0,  49,   0,  51,
   0,   0,   0,   0,  63,  63,   0,   0,  61,   0,
   0,   0,   0,   0,   0,   0,  63,   0, 135, 136,
 137, 138, 147, 148, 149,   0,   0,  78,   0,   0,
   0,   0,   0,   0,   0,   0,  64,  64,  64,   0,
  29,  66,  67,   0,  31,  32,  35,   0,   0,   0,
   0,   0,   0,   0, 156, 158,  69,  76,  63,  63,
  63,   0,   0,   0,   0,   0,  25,  63,  26,  27,
   0,   0,  63,  63,   0,   0,  63,   0,  63,   0,
   0, 154,  20,  21,  22,  23,  24,  65,  28,  30,
  37,  56,  57,  38,   0,  58,  63,  40,   0,  55,
  44,  45,  63,  63,  63,  59,  63,  63,  63,   0,
 153,  39,  60,  41,  42,  43,  63, 152 };
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif not lint

#
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 3:
# line 88 "parser.y"
{ initExprs(); } break;
case 4:
# line 88 "parser.y"
{ } break;
case 5:
# line 89 "parser.y"
{ initExprs(); } break;
case 6:
# line 90 "parser.y"
{ } break;
case 7:
# line 94 "parser.y"
{ } break;
case 8:
# line 95 "parser.y"
{ setAlignment(2); } break;
case 9:
# line 95 "parser.y"
{ } break;
case 10:
# line 96 "parser.y"
{ } break;
case 11:
# line 98 "parser.y"
{ ErrorMsg "Unknown or missing command." EndMsg; } break;
case 12:
# line 99 "parser.y"
{ } break;
case 13:
# line 103 "parser.y"
{ } break;
case 14:
# line 104 "parser.y"
{ } break;
case 15:
# line 108 "parser.y"
{ } break;
case 16:
# line 109 "parser.y"
{ } break;
case 17:
# line 113 "parser.y"
{ DefineLabel(yypvt[-1].sym); } break;
case 18:
# line 114 "parser.y"
{ DefineTempLabel((int) yypvt[-1].num, (symbolType *) NULL); } break;
case 19:
# line 115 "parser.y"
{ DefineTempLabel(-1, yypvt[-1].sym); } break;
case 20:
# line 120 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 21:
# line 122 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 22:
# line 124 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 23:
# line 126 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 24:
# line 128 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 25:
# line 130 "parser.y"
{ emitRRxInst(yypvt[-4].num, yypvt[-3].operand, yypvt[-1].operand, _r0); } break;
case 26:
# line 132 "parser.y"
{ emitRRxInst(yypvt[-4].num, yypvt[-3].operand, yypvt[-1].operand, _r0); } break;
case 27:
# line 134 "parser.y"
{ emitRRxInst(yypvt[-4].num, yypvt[-3].operand, yypvt[-1].operand, _r0); } break;
case 28:
# line 136 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-4].operand, _r0, yypvt[-0].operand); } break;
case 29:
# line 138 "parser.y"
{ emitRRxInst(yypvt[-3].num, yypvt[-2].operand, _r0, yypvt[-0].operand); } break;
case 30:
# line 141 "parser.y"
{ emitRRxInst(yypvt[-5].num, yypvt[-0].operand, _r0, yypvt[-4].operand); } break;
case 31:
# line 144 "parser.y"
{ emitRRxInst(yypvt[-3].num, yypvt[-0].operand, _r0, yypvt[-2].operand); } break;
case 32:
# line 146 "parser.y"
{ emitRRxInst(yypvt[-3].num, yypvt[-2].operand, yypvt[-2].operand, yypvt[-0].operand); } break;
case 33:
# line 148 "parser.y"
{ emitRRxInst(yypvt[-1].num, yypvt[-0].operand, _r0, _r0); } break;
case 34:
# line 150 "parser.y"
{ emitRRxInst(yypvt[-0].num, _r0, _r0, _r0); } break;
case 35:
# line 152 "parser.y"
{ emitRRxInst(yypvt[-3].num, _r0, yypvt[-2].operand, yypvt[-0].operand); } break;
case 36:
# line 153 "parser.y"
{ emitRRxInst(yypvt[-1].num, _r0, _r0, yypvt[-0].operand); } break;
case 37:
# line 155 "parser.y"
{ emitStoreInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].expr); } break;
case 38:
# line 157 "parser.y"
{ emitStoreInst(yypvt[-5].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].expr); } break;
case 39:
# line 159 "parser.y"
{ emitRxCmpInst(yypvt[-7].num, yypvt[-6].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].expr, FALSE); } break;
case 40:
# line 161 "parser.y"
{ emitRxCmpInst(yypvt[-5].num, yypvt[-4].num, yypvt[-2].operand, yypvt[-0].operand, nullExpr, FALSE); } break;
case 41:
# line 163 "parser.y"
{ emitCmpTagInst(yypvt[-7].num, yypvt[-6].num, yypvt[-4].operand, yypvt[-2].expr, yypvt[-0].expr, FALSE); } break;
case 42:
# line 165 "parser.y"
{ emitRxCmpInst(yypvt[-7].num, yypvt[-6].num, yypvt[-4].operand, yypvt[-2].operand, yypvt[-0].expr, TRUE); } break;
case 43:
# line 167 "parser.y"
{ emitCmpTagInst(yypvt[-7].num, yypvt[-6].num, yypvt[-4].operand, yypvt[-2].expr, yypvt[-0].expr, TRUE); } break;
case 44:
# line 169 "parser.y"
{ emitRxCmpInst(yypvt[-5].num, yypvt[-4].num, yypvt[-2].operand, yypvt[-0].operand, nullExpr, TRUE); } break;
case 45:
# line 171 "parser.y"
{ emitCmpTagInst(yypvt[-5].num, yypvt[-4].num, yypvt[-2].operand, yypvt[-0].expr, nullExpr, TRUE); } break;
case 46:
# line 172 "parser.y"
{ emitJumpInst(yypvt[-1].num, yypvt[-0].expr); } break;
case 47:
# line 173 "parser.y"
{ emitJumpInst(yypvt[-1].num, yypvt[-0].expr); } break;
case 48:
# line 177 "parser.y"
{ yyval.operand.type = REG; yyval.operand.number = yypvt[-0].num; } break;
case 49:
# line 178 "parser.y"
{ if (yypvt[-0].num > LASTINTREG) {
				      yyval.operand.number = 0;
				      ErrorMsg 
					 "Integer register number %d out of range.",
					 yypvt[-0].num
				      EndMsg;
				  }
				  else yyval.operand.number = yypvt[-0].num;
				  yyval.operand.type = REG;
				} break;
case 50:
# line 191 "parser.y"
{ yyval.operand.type = REG;  yyval.operand.number = yypvt[-0].num; } break;
case 51:
# line 192 "parser.y"
{ if (yypvt[-0].num > LASTFLOATREG) {
				      yyval.operand.number = 0;
				      ErrorMsg
					  "Floating point register number %d out of range.",
					  yypvt[-0].num
				      EndMsg;
			          }
				  else yyval.operand.number = yypvt[-0].num;
				  yyval.operand.type = REG;
			      } break;
case 52:
# line 205 "parser.y"
{ yyval.operand.type = REG; yyval.operand.number = yypvt[-0].num; } break;
case 54:
# line 210 "parser.y"
{ yyval.operand.type = IMMED;
				  yyval.operand.expr = yypvt[-0].expr; } break;
case 55:
# line 216 "parser.y"
{ yyval.expr = numToExpr(yypvt[-0].num); } break;
case 57:
# line 221 "parser.y"
{ yyval.expr = numToExpr(yypvt[-0].num);
				  if (! haveWarnedAboutRegStores) {
				      haveWarnedAboutRegStores = TRUE;
				      WarningMsg 
					  "Warning: at least one store with three register operands."
				      EndMsg;
				  }
			      } break;
case 59:
# line 233 "parser.y"
{ yyval.operand.type = IMMED;
				  yyval.operand.expr = numToExpr(yypvt[-0].num); } break;
case 61:
# line 242 "parser.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 62:
# line 246 "parser.y"
{ } break;
case 63:
# line 247 "parser.y"
{ } break;
case 64:
# line 251 "parser.y"
{ } break;
case 65:
# line 252 "parser.y"
{ } break;
case 66:
# line 256 "parser.y"
{ if (yypvt[-0].expr -> class != MANIFEST_INT ||
				      yypvt[-0].expr -> v.value != 0) {
					  ErrorMsg "Operand must be 0." EndMsg;
				  }
				} break;
case 67:
# line 261 "parser.y"
{ if (yypvt[-0].num != 0) {
				      ErrorMsg "Operand must be 0." EndMsg;
			          }
				} break;
case 68:
# line 268 "parser.y"
{ yyval.expr = yypvt[-0].expr; } break;
case 69:
# line 269 "parser.y"
{ yyval.expr = consBinaryExpr(yypvt[-2].num, yypvt[-3].expr, yypvt[-0].expr); } break;
case 70:
# line 273 "parser.y"
{ yyval.expr = symToExpr(yypvt[-0].sym); } break;
case 71:
# line 274 "parser.y"
{ yyval.expr = numToExpr(yypvt[-0].num); } break;
case 72:
# line 275 "parser.y"
{ yyval.expr = fwdTempLabelToExpr(-1, yypvt[-0].sym); } break;
case 73:
# line 276 "parser.y"
{ yyval.expr = bckwdTempLabelToExpr(-1, yypvt[-0].sym); } break;
case 74:
# line 277 "parser.y"
{ yyval.expr = fwdTempLabelToExpr((int) yypvt[-0].num,
						        (symbolType *) NULL); } break;
case 75:
# line 279 "parser.y"
{ yyval.expr = bckwdTempLabelToExpr((int) yypvt[-0].num,
							(symbolType *) NULL);
			        } break;
case 76:
# line 282 "parser.y"
{ yyval.expr = yypvt[-1].expr; } break;
case 77:
# line 283 "parser.y"
{ yyval.expr = pointExpr(); } break;
case 78:
# line 284 "parser.y"
{ yyval.expr = consUnaryExpr(yypvt[-1].num, yypvt[-0].expr); } break;
case 83:
# line 292 "parser.y"
{ yyval.num = yypvt[-0].num & ~instModifierBits; } break;
case 117:
# line 335 "parser.y"
{ yyval.num = '-'; } break;
case 118:
# line 336 "parser.y"
{ yyval.num = '~'; } break;
case 119:
# line 341 "parser.y"
{ yyval.expr = symToExpr(yypvt[-0].sym); } break;
case 120:
# line 345 "parser.y"
{ setGlobalSym(yypvt[-0].sym); } break;
case 121:
# line 346 "parser.y"
{ setGlobalSym(yypvt[-0].sym); } break;
case 122:
# line 347 "parser.y"
{ list_oper = LONG; setAlignment(2); } break;
case 123:
# line 348 "parser.y"
{ } break;
case 124:
# line 349 "parser.y"
{ list_oper = WORD; setAlignment(1); } break;
case 125:
# line 350 "parser.y"
{ } break;
case 126:
# line 351 "parser.y"
{ list_oper = BYTE; } break;
case 127:
# line 352 "parser.y"
{ } break;
case 128:
# line 353 "parser.y"
{ list_oper = SINGLE; setAlignment(2); } break;
case 129:
# line 354 "parser.y"
{ } break;
case 130:
# line 355 "parser.y"
{ list_oper = DOUBLE; setAlignment(3); } break;
case 131:
# line 356 "parser.y"
{ } break;
case 132:
# line 357 "parser.y"
{ emitBytes(yypvt[-0].string.str, yypvt[-0].string.len); } break;
case 133:
# line 358 "parser.y"
{ emitBytes(yypvt[-0].string.str, yypvt[-0].string.len);
				      	  emitBytes((char *) "\0", 1); } break;
case 134:
# line 360 "parser.y"
{ setAlignment(yypvt[-0].num); } break;
case 135:
# line 361 "parser.y"
{ setSymCommDefn(yypvt[-2].sym, yypvt[-0].expr, FALSE); } break;
case 136:
# line 362 "parser.y"
{ setSymLcommDefn(yypvt[-2].sym, yypvt[-0].expr, FALSE); } break;
case 137:
# line 363 "parser.y"
{ setSymCommDefn(yypvt[-2].sym, yypvt[-0].expr, TRUE); } break;
case 138:
# line 364 "parser.y"
{ setSymLcommDefn(yypvt[-2].sym, yypvt[-0].expr, TRUE); } break;
case 139:
# line 365 "parser.y"
{ currentSegment = RP_SEG0; } break;
case 140:
# line 366 "parser.y"
{ currentSegment = RP_SEG0+1; } break;
case 141:
# line 367 "parser.y"
{ currentSegment = RP_SEG0+2; } break;
case 142:
# line 368 "parser.y"
{ currentSegment = RP_SEG0+3; } break;
case 143:
# line 369 "parser.y"
{ currentSegment = RP_SEG0+4; } break;
case 144:
# line 370 "parser.y"
{ currentSegment = RP_SEG0+5; } break;
case 145:
# line 371 "parser.y"
{ currentSegment = RP_SEG0+7; } break;
case 146:
# line 372 "parser.y"
{ currentSegment = RP_SEG0+8; } break;
case 147:
# line 373 "parser.y"
{ setSymDefn(yypvt[-2].sym, yypvt[-0].expr); } break;
case 148:
# line 374 "parser.y"
{ setSymDefn(yypvt[-2].sym, numToExpr(yypvt[-0].num)); } break;
case 149:
# line 375 "parser.y"
{ setSymDefn(yypvt[-2].sym, numToExpr(yypvt[-0].num)); } break;
case 150:
# line 376 "parser.y"
{ SetOrg(yypvt[-0].expr); } break;
case 151:
# line 377 "parser.y"
{ leaveSpace(yypvt[-0].expr); } break;
case 152:
# line 380 "parser.y"
{ emitStab(yypvt[-8].string.str, yypvt[-6].num, yypvt[-4].num, yypvt[-2].num, yypvt[-0].expr); } break;
case 153:
# line 383 "parser.y"
{ emitStab((char *) "", 
						   yypvt[-6].num, yypvt[-4].num, yypvt[-2].num, yypvt[-0].expr); } break;
case 154:
# line 387 "parser.y"
{ emitStab((char *) "",
						   yypvt[-4].num, yypvt[-2].num, yypvt[-0].num, 
						   pointExpr()); } break;
case 155:
# line 394 "parser.y"
{ emitExpr(yypvt[-0].expr, list_oper); } break;
case 156:
# line 395 "parser.y"
{ emitExpr(yypvt[-0].expr, list_oper); } break;
case 157:
# line 399 "parser.y"
{ emitFloat(yypvt[-0].string.str, yypvt[-0].string.len, list_oper); } break;
case 158:
# line 401 "parser.y"
{ emitFloat(yypvt[-0].string.str, yypvt[-0].string.len, list_oper); } break;
case 159:
# line 406 "parser.y"
{ if (yypvt[-0].expr->class != MANIFEST_INT) {
		                      ErrorMsg
					  "Expression must be static."
				      EndMsg;
				      yyval.num = 0;
				  }
				  else yyval.num = yypvt[-0].expr->v.value;
			      } break;
		}
		goto yystack;  /* stack new state and value */

	}
