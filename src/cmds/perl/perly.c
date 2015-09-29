
# line 23 "perly.y"
#include "INTERN.h"
#include "perl.h"

/*SUPPRESS 530*/
/*SUPPRESS 593*/
/*SUPPRESS 595*/

STAB *scrstab;
ARG *arg4;	/* rarely used arguments to make_op() */
ARG *arg5;


# line 38 "perly.y"
typedef union  {
    int	ival;
    char *cval;
    ARG *arg;
    CMD *cmdval;
    struct compcmd compval;
    STAB *stabval;
    FCMD *formval;
} YYSTYPE;
# define WORD 257
# define APPEND 258
# define OPEN 259
# define SSELECT 260
# define LOOPEX 261
# define USING 262
# define FORMAT 263
# define DO 264
# define SHIFT 265
# define PUSH 266
# define POP 267
# define LVALFUN 268
# define WHILE 269
# define UNTIL 270
# define IF 271
# define UNLESS 272
# define ELSE 273
# define ELSIF 274
# define CONTINUE 275
# define SPLIT 276
# define FLIST 277
# define FOR 278
# define FILOP 279
# define FILOP2 280
# define FILOP3 281
# define FILOP4 282
# define FILOP22 283
# define FILOP25 284
# define FUNC0 285
# define FUNC1 286
# define FUNC2 287
# define FUNC2x 288
# define FUNC3 289
# define FUNC4 290
# define FUNC5 291
# define HSHFUN 292
# define HSHFUN3 293
# define FLIST2 294
# define SUB 295
# define FILETEST 296
# define LOCAL 297
# define DELETE 298
# define RELOP 299
# define EQOP 300
# define MULOP 301
# define ADDOP 302
# define PACKAGE 303
# define AMPER 304
# define FORMLIST 305
# define REG 306
# define ARYLEN 307
# define ARY 308
# define HSH 309
# define STAR 310
# define SUBST 311
# define PATTERN 312
# define RSTRING 313
# define TRANS 314
# define LISTOP 315
# define DOTDOT 316
# define OROR 317
# define ANDAND 318
# define UNIOP 319
# define LS 320
# define RS 321
# define MATCH 322
# define NMATCH 323
# define UMINUS 324
# define POW 325
# define INC 326
# define DEC 327
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 828 "perly.y"
 /* PROGRAM */
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 3,
	0, 2,
	-2, 39,
-1, 130,
	326, 0,
	327, 0,
	-2, 84,
-1, 131,
	326, 0,
	327, 0,
	-2, 85,
-1, 232,
	299, 0,
	-2, 66,
-1, 233,
	300, 0,
	-2, 67,
-1, 234,
	316, 0,
	-2, 71,
-1, 257,
	41, 177,
	44, 177,
	-2, 185,
-1, 258,
	44, 177,
	-2, 185,
	};
# define YYNPROD 186
# define YYLAST 3267
short yyact[]={

  26, 327,  27, 201, 107, 324, 305, 243, 124, 125,
 145,  21, 115, 116, 108, 109,  92, 146, 154,  88,
  90, 183, 155,  94,  96, 115, 181, 108, 109, 112,
 287, 166, 255, 110, 111, 121, 122, 108, 107, 133,
 138, 169, 137, 108, 109, 191, 110, 111, 121, 122,
 194, 107, 152, 149, 165, 150, 108, 109, 121, 122,
  83, 107, 110, 111, 121, 122, 147, 107, 121, 122,
 173, 107, 148, 184, 104, 105, 102, 121, 122,  85,
 107, 189, 167,  84, 164, 192, 101, 101,  11, 101,
   3,  28, 247, 252,  12, 244, 143, 101, 242, 141,
 196, 101, 198, 212, 211, 213, 204, 101, 206, 215,
 217, 219, 221, 223, 225, 227, 229, 231, 232, 233,
 234, 235, 236, 237, 238, 239,  13,  95, 144, 101,
  93, 142,  89, 182,  14,  87, 190, 101,  11, 308,
 245, 387, 292, 141,  12, 193, 389,  86, 168,  81,
 360, 244, 328, 325, 242, 256, 316, 260, 260, 101,
 335, 260, 260, 260, 260, 260, 380, 363, 317, 328,
 315, 334, 332, 271, 272, 142,  13, 412, 410, 277,
 278, 279, 280, 281,  14, 408, 314, 259, 261, 406,
  32, 263, 264, 265, 266, 267, 377,  38, 254, 325,
  31, 357,  30, 342, 101, 404, 101, 403, 291, 294,
  28, 401, 101,  28, 240,  28,  16, 293,  28, 296,
 101, 297, 398, 298, 397, 299, 396, 300, 392, 301,
 391, 302, 379, 303, 289, 153, 378, 101, 370, 156,
 368, 348, 337, 183, 336, 211, 329, 326, 268, 306,
 269, 323, 288, 274, 270, 253,  82, 246, 166, 214,
 199, 185, 180, 179, 178, 330,  32, 177, 176, 151,
 331, 166, 333,  38, 340, 175,  31, 338,  30, 343,
  28, 341,  91,  33, 344, 345, 346, 347, 157, 349,
 115, 116, 108, 109, 174, 184, 172, 171, 352, 170,
 350, 351, 112, 353,  10, 355, 356, 358, 163, 167,
 162, 110, 111, 121, 122, 282, 107, 161, 361, 283,
 362, 160, 167,  99, 100,  97,  98, 364, 367, 365,
 366, 159, 158, 371, 134, 260,   9, 260, 202, 373,
   2,   6,  77, 354, 376,  17, 381, 382,  78, 103,
  15, 385, 383, 384,  99, 100,  97,  98, 113,  33,
   7,  29,   4, 123, 386, 372,  32, 374,   8, 388,
   5,   1, 390,  38, 322, 393,  31,   0,  30,   0,
 394,   0,   0, 395,   0, 399,   0,   0, 114,   0,
 400, 402, 126, 127, 128, 129, 130, 131,   0,   0,
 405,   0, 407,   0,   0,   0,   0, 409,   0, 411,
   0,   0,   0,  24,  79,   0,  54,  53,  51,   0,
   0,  39,  63,  61,  62,  67,  18,  19,  22,  23,
   0,   0,   0,  64,  66,  20,  55,  56,  57,  59,
  58,  60,  68,  69,  70,  71,  72,  73,  74,  75,
  76,  65,   0,  36,  37,  44,   0,   0,   0,  33,
   0,  50,   0,  40,  45,  43,  42,  41,  48,  47,
  46,  49,  80,   0,   0,   0,  52,   0,   0,   0,
   0,   0,   0,  34,  35,   0,   0,   0,   0,  24,
  79,   0,  54,  53,  51,  32,   0,  39,  63,  61,
  62,  67,  38, 320,   0,  31,   0,  30,   0,  64,
  66,   0,  55,  56,  57,  59,  58,  60,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  65,   0,  36,
  37,  44,   0,   0,   0,   0,   0,  50,   0,  40,
  45,  43,  42,  41,  48,  47,  46,  49,  80,   0,
   0,   0,  52,   0,   0,   0,   0,   0,   0,  34,
  35,   0,   0, 115, 116, 108, 109,   0,  32,   0,
   0,   0,   0,   0,   0,  38, 313,   0,  31,   0,
  30, 119, 118,   0, 110, 111, 121, 122,  33, 107,
  79,   0,  54,  53,  51,   0,   0,  39,  63,  61,
  62,  67,   0,   0,   0,   0,   0,   0,   0,  64,
  66,   0,  55,  56,  57,  59,  58,  60,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  65,   0,  36,
  37,  44,   0,   0,   0,   0,   0,  50,   0,  40,
  45,  43,  42,  41,  48,  47,  46,  49,  80,   0,
   0,   0,  52,   0,   0,   0,   0,   0,   0,  34,
  35,  33,   0,   0,  32,   0,   0,   0,   0,   0,
   0,  38, 311,   0,  31,   0,  30,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 112,   0,   0, 339,   0,   0, 325,   0,  79,
   0,  54,  53,  51, 112,   0,  39,  63,  61,  62,
  67,   0,   0,   0, 106,   0, 120,   0,  64,  66,
   0,  55,  56,  57,  59,  58,  60,  68,  69,  70,
  71,  72,  73,  74,  75,  76,  65,  33,  36,  37,
  44,   0,   0,   0,   0,   0,  50, 113,  40,  45,
  43,  42,  41,  48,  47,  46,  49,  80,   0, 112,
 113,  52,   0,   0,   0,   0,   0,   0,  34,  35,
   0,   0,  79,   0,  54,  53,  51, 114,   0,  39,
  63,  61,  62,  67, 120,   0,   0,   0,   0,   0,
 114,  64,  66,   0,  55,  56,  57,  59,  58,  60,
  68,  69,  70,  71,  72,  73,  74,  75,  76,  65,
   0,  36,  37,  44,   0, 113,   0,   0,   0,  50,
   0,  40,  45,  43,  42,  41,  48,  47,  46,  49,
  80,   0,   0,   0,  52,   0,  32,   0,   0,   0,
   0,  34,  35,  38, 307, 114,  31,   0,  30,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  79,   0,
  54,  53,  51,   0,   0,  39,  63,  61,  62,  67,
   0,   0,   0,   0,   0,   0,   0,  64,  66,   0,
  55,  56,  57,  59,  58,  60,  68,  69,  70,  71,
  72,  73,  74,  75,  76,  65,   0,  36,  37,  44,
   0,   0,   0,   0,   0,  50,   0,  40,  45,  43,
  42,  41,  48,  47,  46,  49,  80,   0,   0,  33,
  52,   0,  32,   0,   0,   0,   0,  34,  35,  38,
 275,   0,  31,   0,  30,   0,   0,   0,   0,   0,
   0,   0, 115, 116, 108, 109,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 115, 116, 108, 109, 117,
 119, 118,   0, 110, 111, 121, 122,   0, 107,   0,
   0,   0,   0,   0, 118,   0, 110, 111, 121, 122,
   0, 107,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  32,   0,   0,   0,   0,
   0,   0,  38, 262,   0,  31,   0,  30,   0,   0,
 115, 116, 108, 109,   0,  33,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 117, 119, 118,
   0, 110, 111, 121, 122,   0, 107,   0,   0, 112,
   0,   0,   0,   0,   0, 325,   0,   0,   0,   0,
  79,   0,  54,  53,  51,   0,   0,  39,  63,  61,
  62,  67, 106,   0, 120,   0,   0,   0,   0,  64,
  66,   0,  55,  56,  57,  59,  58,  60,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  65,  33,  36,
  37,  44,   0,   0,   0, 113,   0,  50,   0,  40,
  45,  43,  42,  41,  48,  47,  46,  49,  80,   0,
   0,   0,  52,   0,  32,   0,   0,   0,   0,  34,
  35,  38,   0,   0,  31, 114,  30,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 230,   0,   0,   0,  79,   0,  54,  53,
  51,   0,   0,  39,  63,  61,  62,  67,   0,   0,
   0,   0,   0,   0,   0,  64,  66,   0,  55,  56,
  57,  59,  58,  60,  68,  69,  70,  71,  72,  73,
  74,  75,  76,  65,   0,  36,  37,  44,   0,   0,
   0,   0,   0,  50,   0,  40,  45,  43,  42,  41,
  48,  47,  46,  49,  80,   0, 112,  33,  52,   0,
   0,   0,   0,   0,   0,  34,  35,   0,   0, 257,
   0,  54,  53,  51,   0,   0,  39,  63,  61,  62,
  67,   0,   0,   0,   0,   0,   0,   0,  64,  66,
   0,  55,  56,  57,  59,  58,  60,  68,  69,  70,
  71,  72,  73,  74,  75,  76,  65,   0,  36,  37,
  44,   0, 113,   0,   0,   0,  50,   0,  40,  45,
  43,  42,  41,  48,  47,  46,  49,  80,   0,   0,
   0,  52,   0,  32,   0,   0,   0,   0,  34,  35,
  38,   0, 114,  31,   0,  30,   0,   0,   0,   0,
 115, 116, 108, 109,   0,   0,   0,   0,   0,   0,
   0, 228,   0,   0,   0,   0,   0, 117, 119, 118,
   0, 110, 111, 121, 122,   0, 107, 112,   0,   0,
   0,   0,   0, 328,   0,   0,   0,   0,  79,   0,
  54,  53,  51,   0,   0,  39,  63,  61,  62,  67,
 106,   0, 120,   0,   0,   0,   0,  64,  66,   0,
  55,  56,  57,  59,  58,  60,  68,  69,  70,  71,
  72,  73,  74,  75,  76,  65,  33,  36,  37,  44,
   0,   0,   0, 113,   0,  50,   0,  40,  45,  43,
  42,  41,  48,  47,  46,  49,  80,   0,   0,  32,
  52,   0,   0,   0,   0,   0,  38,  34,  35,  31,
   0,  30,   0, 114,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 226,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 115, 116, 108,
 109,   0,   0,  32,   0,   0,   0,   0,   0,   0,
  38,   0,   0,  31,   0,  30,   0,   0, 110, 111,
 121, 122,  33, 107,   0,   0,   0,   0,   0,   0,
   0, 224,   0,   0,   0,   0,   0,  79,   0,  54,
  53,  51,   0,   0,  39,  63,  61,  62,  67,   0,
   0,   0,   0,   0,   0,   0,  64,  66,   0,  55,
  56,  57,  59,  58,  60,  68,  69,  70,  71,  72,
  73,  74,  75,  76,  65,   0,  36,  37,  44,   0,
   0,   0,   0,   0,  50,   0,  40,  45,  43,  42,
  41,  48,  47,  46,  49,  80,  33,   0,  32,  52,
   0,   0,   0,   0,   0,  38,  34,  35,  31,   0,
  30,   0,   0,   0,   0,   0,   0,   0, 115, 116,
 108, 109,   0,   0,   0,   0, 222,   0,   0,   0,
   0,   0,   0,   0,   0, 117, 119, 118,   0, 110,
 111, 121, 122,   0, 107,   0,   0,   0,   0,   0,
   0,   0,   0,  79,   0,  54,  53,  51,   0,   0,
  39,  63,  61,  62,  67,   0,   0,   0,   0,   0,
   0,   0,  64,  66,   0,  55,  56,  57,  59,  58,
  60,  68,  69,  70,  71,  72,  73,  74,  75,  76,
  65,  33,  36,  37,  44,   0,   0,   0,   0,   0,
  50,   0,  40,  45,  43,  42,  41,  48,  47,  46,
  49,  80,   0,   0,  32,  52,   0,   0,   0,   0,
   0,  38,  34,  35,  31,   0,  30,  79,   0,  54,
  53,  51,   0,   0,  39,  63,  61,  62,  67,   0,
   0,   0, 220,   0,   0,   0,  64,  66,   0,  55,
  56,  57,  59,  58,  60,  68,  69,  70,  71,  72,
  73,  74,  75,  76,  65,   0,  36,  37,  44,   0,
   0,   0,   0,   0,  50,   0,  40,  45,  43,  42,
  41,  48,  47,  46,  49,  80,   0,   0,  32,  52,
   0,   0,   0,   0,   0,  38,  34,  35,  31,   0,
  30,   0,   0,   0,   0,   0,   0,  33,   0,   0,
   0,   0,   0,   0,   0,   0, 218,   0,   0,   0,
   0,   0,  79,   0,  54,  53,  51,   0,   0,  39,
  63,  61,  62,  67,   0,   0,   0,   0,   0,   0,
   0,  64,  66,   0,  55,  56,  57,  59,  58,  60,
  68,  69,  70,  71,  72,  73,  74,  75,  76,  65,
   0,  36,  37,  44,   0,   0,   0,   0,   0,  50,
   0,  40,  45,  43,  42,  41,  48,  47,  46,  49,
  80,  33,   0,  32,  52,   0,   0,   0,   0,   0,
  38,  34,  35,  31,   0,  30,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 216,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 112,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  79,   0,
  54,  53,  51,   0,   0,  39,  63,  61,  62,  67,
 106,   0, 120,   0,   0,   0,   0,  64,  66,   0,
  55,  56,  57,  59,  58,  60,  68,  69,  70,  71,
  72,  73,  74,  75,  76,  65,  33,  36,  37,  44,
   0,   0,   0, 113,   0,  50,   0,  40,  45,  43,
  42,  41,  48,  47,  46,  49,  80,   0,   0,  32,
  52,   0,   0,   0,   0,   0,  38,  34,  35,  31,
   0,  30,  79, 114,  54,  53,  51,   0,   0,  39,
  63,  61,  62,  67,   0,   0,   0,   0,   0,   0,
   0,  64,  66,   0,  55,  56,  57,  59,  58,  60,
  68,  69,  70,  71,  72,  73,  74,  75,  76,  65,
   0,  36,  37,  44,   0,   0,   0,   0,   0,  50,
   0,  40,  45,  43,  42,  41,  48,  47,  46,  49,
  80,   0,   0,   0,  52,   0,   0,   0,  32,   0,
   0,  34,  35,   0,   0,  38,   0,   0,  31,  28,
  30,   0,  33,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  79,   0,  54,
  53,  51,   0,   0,  39,  63,  61,  62,  67,   0,
   0,   0,   0,   0,   0,   0,  64,  66,   0,  55,
  56,  57,  59,  58,  60,  68,  69,  70,  71,  72,
  73,  74,  75,  76,  65,   0,  36,  37,  44,   0,
   0,   0,   0,   0,  50,   0,  40,  45,  43,  42,
  41,  48,  47,  46,  49,  80,   0,   0,  28,  52,
   0,  33,   0,   0,   0,  32,  34,  35,   0,   0,
   0,   0,  38,   0,   0,  31,   0,  30, 115, 116,
 108, 109,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 117, 119, 118,   0, 110,
 111, 121, 122,   0, 107, 112,   0,   0,   0,   0,
   0,   0,   0, 187,   0,  54,  53,  51,   0,   0,
  39,  63,  61,  62,  67, 304,   0,   0, 106,   0,
 120,   0,  64,  66,   0,  55,  56,  57,  59,  58,
  60,  68,  69,  70,  71,  72,  73,  74,  75,  76,
  65,   0,  36,  37,  44,  28,   0,   0,  33,   0,
  50, 113, 188,  45,  43,  42,  41,  48,  47,  46,
  49,  80,   0,   0,   0,  52,   0,  32,   0,   0,
   0,   0,  34,  35,  38, 136,   0,  31,   0,  30,
   0, 114,  79,   0,  54,  53,  51,   0,   0,  39,
  63,  61,  62,  67,   0,   0,   0,   0,   0,   0,
   0,  64,  66,   0,  55,  56,  57,  59,  58,  60,
  68,  69,  70,  71,  72,  73,  74,  75,  76,  65,
   0,  36,  37,  44,   0,   0,   0,   0,   0,  50,
   0,  40,  45,  43,  42,  41,  48,  47,  46,  49,
  80,   0,   0,   0,  52,   0,  32,   0,   0,   0,
   0,  34,  35,  38,   0,   0,  31,   0,  30,   0,
  33,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 139,
   0,  54,  53,  51,   0,   0,  39,  63,  61,  62,
  67,   0,   0,   0,   0,   0,   0,   0,  64,  66,
   0,  55,  56,  57,  59,  58,  60,  68,  69,  70,
  71,  72,  73,  74,  75,  76,  65,   0,  36,  37,
  44,   0,   0,   0,   0,   0,  50,   0, 140,  45,
  43,  42,  41,  48,  47,  46,  49,  80,   0,  33,
  32,  52,   0,   0,   0,   0,   0,  38,  34,  35,
  31,   0,  30,   0,   0,   0, 115, 116, 108, 109,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 117, 119, 118,   0, 110, 111, 121,
 122,   0, 107,   0,   0,   0,   0,   0,   0,   0,
   0,  79,   0,  54,  53,  51,   0,   0,  39,  63,
  61,  62,  67,   0,   0,   0,   0,   0,   0,   0,
  64,  66,   0,  55,  56,  57,  59,  58,  60,  68,
  69,  70,  71,  72,  73,  74,  75,  76,  65,   0,
  36,  37,  44,  33,   0,   0,   0,   0,  50,   0,
  40,  45,  43,  42,  41,  48,  47,  46,  49,  80,
   0,   0,  32,  52,   0,   0,   0,   0,   0,  38,
  34,  35,  31,   0,  30,   0,   0,   0,   0,   0,
  79,   0,  54,  53,  51,   0,   0,  39,  63,  61,
  62,  67,   0,   0,   0,   0,   0,   0,   0,  64,
  66,   0,  55,  56,  57,  59,  58,  60,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  65,   0,  36,
  37,  44,   0,   0,   0,   0,   0,  50,   0,  40,
  45,  43,  42,  41,  48,  47,  46,  49,  80,   0,
   0,  32,  52,   0,   0,   0,   0,   0,  38,  34,
  35,  31,   0,  30,   0,  33,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 257,   0,  54,  53,  51,   0,
   0,  39,  63,  61,  62,  67,   0,   0,   0,   0,
   0,   0,   0,  64,  66,   0,  55,  56,  57,  59,
  58,  60,  68,  69,  70,  71,  72,  73,  74,  75,
  76,  65,   0,  36,  37,  44,   0,   0,   0,   0,
   0,  50,   0,  40,  45,  43,  42,  41,  48,  47,
  46,  49,  80,   0,  33,   0,  52,   0,   0,   0,
   0,   0,   0,  34,  35,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0, 258,   0,  54,  53,
  51,   0,   0,  39,  63,  61,  62,  67,   0,   0,
   0,   0,   0,   0,   0,  64,  66,   0,  55,  56,
  57,  59,  58,  60,  68,  69,  70,  71,  72,  73,
  74,  75,  76,  65,   0,  36,  37,  44,   0,   0,
   0,   0,   0,  50,   0,  40,  45,  43,  42,  41,
  48,  47,  46,  49,  80,   0,   0,   0,  52,   0,
   0,   0,   0,   0,   0,  34,  35,   0,   0,   0,
   0,   0,   0,   0,  25, 132,   0,  54,  53,  51,
   0,   0,  39,  63,  61,  62,  67,   0,   0,   0,
   0,   0,   0,   0,  64,  66,   0,  55,  56,  57,
  59,  58,  60,  68,  69,  70,  71,  72,  73,  74,
  75,  76,  65, 135,  36,  37,  44,   0,   0,   0,
   0,   0,  50,   0,  40,  45,  43,  42,  41,  48,
  47,  46,  49,  80,   0,   0,   0,  52,   0,   0,
   0,   0,   0,   0,  34,  35,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 186,   0,   0,   0,   0,
   0,   0, 195,   0, 197,   0,   0, 200, 203,   0,
 205,   0, 207, 208, 209, 210,   0,   0,   0,   0,
  32,   0,   0,   0,   0,   0,   0,  38,   0,   0,
  31,   0,  30,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 241,
   0,   0,   0,   0,   0,   0, 248, 249, 250, 251,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 141,   0,
   0,   0,   0,   0,   0,   0,   0, 273,   0,   0,
 276,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 284, 285, 286,   0,   0,   0,   0,   0,
 142,   0,   0,  33, 290,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 295,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 309, 310, 312,   0,   0,   0,   0, 318, 319, 321,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 195,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 359,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 369,  79,   0,  54,  53,  51,   0,
 375,  39,  63,  61,  62,  67,   0,   0,   0,   0,
   0,   0,   0,  64,  66,   0,  55,  56,  57,  59,
  58,  60,  68,  69,  70,  71,  72,  73,  74,  75,
  76,  65,   0,  36,  37,  44,   0,   0,   0,   0,
   0,  50,   0,  40,  45,  43,  42,  41,  48,  47,
  46,  49,  80,   0,   0,   0,  52 };
short yypact[]={

-1000,-1000,-1000,-169,-1000,-1000, 157,-1000,-1000,-1000,
-1000,  91,  -1,-174,-178,-1000,-1000,  88,  95,  92,
 -24,-1000,  90,  87,-1000,  85,-199,1889,-1000,-318,
2333,2333,2333,2333,2333,2333,2618, 294,2254,2142,
   8,-1000,-1000,   5,-296,-1000,-1000,-1000,-1000,-1000,
-240,-185,2045,  12, -22, -18, 292, 291, 281, 277,
 270, 268,  14,   1, 259, 257, 256,2333, 254, 235,
 228, 227, 224, 223, 222, -14, 221,-1000,-1000,-1000,
1966,-1000,  75,-260, -32,  86,-1000,2333, -32,2333,
 -32, 220, 233,2333, -32,2333, -32,2333,2333,2333,
2333,2333, -32,-1000, -32, 219,2333,1850,1755,1681,
1565,1470,1396,1280,1111,2333,2333,2333,2333,2333,
2333,2333,2333,-1000,-1000,-1000,-318,-318,-318,-318,
-1000,-1000,-1000,-258,2333, 110,  49,-258,-1000, 217,
  52,2333,2333,2333,2333, -30, 215, 158,-1000,-1000,
-258,-1000,2427,-1000,2539, 992,-1000,-1000,2427,2427,
2427,2427,2427,-226,-1000,-226,-1000,-1000,-1000,-226,
2333,2333,2333,-1000, 212, 919,2333,2333,2333,2333,
2333,-236,-1000,-1000,-1000,-236, 115,2333,2947,2333,
-275,-1000,-1000,-1000, 211, 115,-1000, 193,-1000,2333,
  54,  83,-1000, 176,-1000, 168,-1000, 115, 115, 115,
 115,1889,-1000,-1000,2333,1889,2333,-321,2333,-254,
2333,-264,2333,-245,2333,-245,2333,-287,2333,  -9,
2333,  -9,-258,-274, 264,1198, 686,2177,-321,-321,
-119, 110, 823,  48,-1000,2333, 631, 535,  93,  45,
  63,  43,2333, 462, 333, 210,1031,-1000, 206, 108,
1889, 205,-1000, 108, 109, 128, 109, 127, 116, 203,
 201, 673,1319, 110,-1000,-1000, 162,1319,1031,1031,
1031,1031, 200, 109, 115, 115, 115,-1000, -32, -32,
 110, -32,2333, -32, -32, 160,1889,1889,1889,1889,
1889,1889,1889,1889,2333,-1000,-1000,-1000,2333,  57,
 110,-1000, 110,-1000,-1000,-1000,-1000,-1000,  42, 110,
-1000, 110,-1000,-1000, 109,2333,-1000, 199,2333,-1000,
 197, 108,2427, 109,2427,2333,-1000,-1000, 155,-1000,
 195,-1000,-1000, 191, 125, 108, 109, 109,-1000, 108,
-1000,-1000, -32,-1000,  82,-1000,-1000, -32, 741,  53,
-1000,-1000,-1000,-1000,-1000,-1000, 109,1889,-1000, 115,
-1000, 189, 187, 108, 109, 110, 185,-1000,-1000,-1000,
-1000, 183, 181, 108, 109, 170,-1000, 233,-1000,-1000,
 166,-1000,-1000, 164, 109,-1000,-1000,-1000,-1000, 148,
 108,-1000, 144,-1000,-1000, 108,-1000, 137, -32, 136,
-1000,-1000,-1000 };
short yypgo[]={

   0, 371, 370, 368, 363,   7,   0,  90, 362, 360,
 350, 338,   3, 349,2874,   2,   1,   5, 361,  32,
  84, 133,  50, 348, 342, 341,  11, 340, 336, 304 };
short yyr1[]={

   0,  27,   1,  26,  26,  13,  13,  13,   6,   4,
   7,   7,   8,   8,   8,   8,   8,  11,  11,  11,
  11,  11,  11,  10,  10,  10,  10,   9,   9,   9,
   9,   9,   9,   9,   9,  12,  12,  22,  22,  25,
  25,   2,   2,   2,   3,   3,  28,  29,  16,  14,
  14,  17,  15,  15,  15,  15,  15,  15,  15,  15,
  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
  15,  15,  15,  15,  15,  15,  15,  15,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  18,  18,  18,  18,  18,  18,  18,  18,  18,
  18,  23,  23,  23,  23,  23,  23,  19,  19,  20,
  20,  21,  21,   5,   5,  24 };
short yyr2[]={

   0,   0,   2,   3,   2,   0,   2,   5,   4,   0,
   0,   2,   1,   2,   1,   2,   3,   1,   1,   3,
   3,   3,   3,   5,   5,   3,   3,   6,   6,   4,
   4,   7,   6,  10,   2,   0,   1,   0,   1,   0,
   2,   1,   1,   1,   4,   3,   3,   3,   2,   3,
   1,   2,   3,   4,   4,   4,   4,   4,   4,   4,
   4,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   5,   3,   3,   1,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   1,   4,
   3,   2,   2,   2,   1,   1,   4,   1,   1,   4,
   6,   5,   4,   4,   5,   1,   1,   1,   1,   1,
   5,   5,   4,   4,   2,   5,   5,   4,   4,   2,
   1,   2,   1,   2,   2,   1,   2,   4,   7,   2,
   4,   5,   4,   2,   2,   3,   1,   5,   6,   6,
   7,   9,   6,   2,   4,   2,   4,   1,   1,   6,
   5,   4,   5,   4,   2,   1,   1,   3,   3,   4,
   5,   5,   6,   6,   7,   8,   4,   2,   6,   1,
   1,   1,   2,   2,   3,   3,   3,   1,   1,   1,
   1,   1,   1,   2,   1,   1 };
short yychk[]={

-1000,  -1, -27,  -7,  -8,  -2, -25,  -9,  -3, -28,
 -29, 257, 263, 295, 303, -10,  59, -11, 269, 270,
 278, -26, 271, 272, 256, -14,  -6, -15, 123, -18,
  45,  43,  33, 126, 326, 327, 296, 297,  40, 264,
 306, 310, 309, 308, 298, 307, 313, 312, 311, 314,
 304, 261, 319, 260, 259, 279, 280, 281, 283, 282,
 284, 266, 267, 265, 276, 294, 277, 268, 285, 286,
 287, 288, 289, 290, 291, 292, 293, -24, -23, 257,
 315,  58, 257,  61, 257, 257,  59,  40,  -6,  40,
  -6, 306,  40,  40,  -6,  40,  -6, 271, 272, 269,
 270,  44, 275, -13, 273, 274,  61, 325, 301, 302,
 320, 321,  38,  94, 124, 299, 300, 316, 318, 317,
  63, 322, 323,  -4, 326, 327, -18, -18, -18, -18,
 -18, -18, 257, -15,  40, -14,  41, -15,  -6, 257,
 306,  91, 123,  91, 123, 306, 257, 306, 257,  -6,
 -15, 257,  40, 257,  40,  40, 257, 306,  40,  40,
  40,  40,  40,  40, -20,  40, 257, 308, -20,  40,
  40,  40,  40, -15,  40,  40,  40,  40,  40,  40,
  40,  40, -21, 257, 309,  40, -14, 257, 306,  -6,
  61, 305,  -6,  59, -22, -14, -26, -14, -26,  40,
 -14, -12, -11, -14, -26, -14, -26, -14, -14, -14,
 -14, -15,  -6,  -6,  40, -15,  61, -15,  61, -15,
  61, -15,  61, -15,  61, -15,  61, -15,  61, -15,
  61, -15, -15, -15, -15, -15, -15, -15, -15, -15,
  -7, -14,  44,  -5,  41,  91,  40,  40, -14, -14,
 -14, -14, 123,  40,  40, -19, -15, 257, 257, -19,
 -15, -19,  41, -19, -19, -19, -19, -19, -20, -20,
 -20, -15, -15, -14,  41,  41, -14, -15, -15, -15,
 -15, -15, -21, -21, -14, -14, -14, 305,  41,  41,
 -14,  -5,  59,  41,  41, -14, -15, -15, -15, -15,
 -15, -15, -15, -15,  58, 125,  -5,  41,  91, -14,
 -14,  41, -14,  41,  93, 125,  93, 125, -14, -14,
  41, -14,  41,  41, -17,  44,  41, -16,  44,  41,
 -16, -17,  44, -17,  44,  44,  41,  41, -17,  41,
 -16,  -5,  41, -16, -17, -17, -17, -17,  41, -17,
 -26, -26,  -5, -26, -22, -26, -26,  41, -15, -14,
  93,  -5,  -5, 125,  -5,  -5, -17, -15,  41, -14,
  41, -16, -19, -17, -19, -14, -17,  41,  41,  41,
  41, -16, -16, -17, -17, -16, -26,  59, -26,  93,
 -17,  41,  41, -16, -17,  -5,  41,  41,  41, -16,
 -17,  41, -12,  41,  41, -17,  41, -16,  41, -16,
  41,  -6,  41 };
short yydef[]={

   1,  -2,  10,  -2,  11,  12,   0,  14,  41,  42,
  43,   0,   0,   0,   0,  13,  15,   0,   0,   0,
   0,  34,   0,   0,  17,  18,   5,  50,   9,  77,
   0,   0,   0,   0,   0,   0,  88,   0,   0,   0,
  94,  95,  97,  98,   0, 105, 106, 107, 108, 109,
   0, 120, 122, 125,   0, 136,   0,   0,   0,   0,
   0,   0,   0, 147, 148,   0,   0, 155, 156,   0,
   0,   0,   0,   0,   0,   0,   0, 169, 170, 185,
 171,  40,   0,   0,   0,   0,  16,  37,   0,   0,
   0,   0,  35,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  10,  82,  83,  78,  79,  80,  81,
  -2,  -2,  86,  87,   0,   0,  91,  92,  93, 185,
  94,   0,   0,   0,   0,   0, 114, 119, 121, 123,
 124, 126,   0, 129,   0,   0, 133, 134,   0,   0,
   0,   0,   0,   0, 143,   0, 179, 180, 145,   0,
   0,   0,   0, 154,   0,   0,   0,   0,   0,   0,
   0,   0, 167, 181, 182,   0, 172, 173,  94,   0,
   0,  45,  46,  47,   0,  38,  29,   0,  30,   0,
  18,   0,  36,   0,  25,   0,  26,  19,  20,  21,
  22,  49,   3,   6,   0,  52,   0,  61,   0,  62,
   0,  63,   0,  64,   0,  65,   0,  68,   0,  69,
   0,  70,  -2,  -2,  -2,  72,  73,   0,  75,  76,
  39,   0,   0,  90, 184,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0, 178,  -2,  -2,   0,
 178,   0, 135,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 157, 158,   0,   0,   0,   0,
   0,   0,   0,   0, 174, 175, 176,  44,   0,   0,
   0,   0,  37,   0,   0,   0,  53,  54,  55,  56,
  57,  58,  59,  60,   0,   8,  89, 183,   0,   0,
   0, 112,   0, 117,  96,  99, 102, 103,   0,   0,
 113,   0, 118, 127,   0,   0, 130,   0,   0, 132,
   0,   0,   0,   0,   0,   0, 144, 146,   0, 151,
   0, 153, 159,   0,   0,   0,   0,   0, 166,   0,
  27,  28,   0,  32,   0,  23,  24,   0,  74,   0,
 101, 110, 115, 104, 111, 116,   0,  51, 131,  48,
 137,   0,   0,   0,   0,   0,   0, 150, 152, 160,
 161,   0,   0,   0,   0,   0,  31,  35,   7, 100,
   0, 138, 139,   0,   0, 142, 149, 162, 163,   0,
   0, 168,   0, 128, 140,   0, 164,   0,   0,   0,
 165,  33, 141 };
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
int yymaxdepth = YYMAXDEPTH;
YYSTYPE *yyv; /* where the values are stored */
short *yys;
short *maxyyps;
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	if (!yyv) {
	    yyv = (YYSTYPE*) malloc(yymaxdepth * sizeof(YYSTYPE));
	    yys = (short*) malloc(yymaxdepth * sizeof(short));
	    if ( !yyv || !yys ) {
		yyerror( "out of memory" );
		return(1);
	    }
	    maxyyps = &yys[yymaxdepth];
	}
	yyps = &yys[-1];
	yypv = &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps >= maxyyps ) {
		    int tv = yypv - yyv;
		    int ts = yyps - yys;

		    yymaxdepth *= 2;
		    yyv = (YYSTYPE*)realloc((char*)yyv,
		      yymaxdepth*sizeof(YYSTYPE));
		    yys = (short*)realloc((char*)yys,
		      yymaxdepth*sizeof(short));
		    if ( !yyv || !yys ) {
			yyerror( "yacc stack overflow" );
			return(1);
		    }
		    yyps = yys + ts;
		    yypv = yyv + tv;
		    maxyyps = &yys[yymaxdepth];
		}
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
			
case 1:
# line 95 "perly.y"
{
#if defined(YYDEBUG) && defined(DEBUGGING)
		    yydebug = (debug & 1);
#endif
		} break;
case 2:
# line 101 "perly.y"
{ if (in_eval)
				eval_root = block_head(yypvt[-0].cmdval);
			    else
				main_root = block_head(yypvt[-0].cmdval); } break;
case 3:
# line 108 "perly.y"
{ yyval.compval.comp_true = yypvt[-2].cmdval; yyval.compval.comp_alt = yypvt[-0].cmdval; } break;
case 4:
# line 110 "perly.y"
{ yyval.compval.comp_true = yypvt[-1].cmdval; yyval.compval.comp_alt = yypvt[-0].cmdval; } break;
case 5:
# line 114 "perly.y"
{ yyval.cmdval = Nullcmd; } break;
case 6:
# line 116 "perly.y"
{ yyval.cmdval = yypvt[-0].cmdval; } break;
case 7:
# line 118 "perly.y"
{ cmdline = yypvt[-4].ival;
			    yyval.cmdval = make_ccmd(C_ELSIF,yypvt[-2].arg,yypvt[-0].compval); } break;
case 8:
# line 123 "perly.y"
{ yyval.cmdval = block_head(yypvt[-1].cmdval);
			  if (cmdline > yypvt[-3].ival)
			      cmdline = yypvt[-3].ival;
			  if (savestack->ary_fill > yypvt[-2].ival)
			    restorelist(yypvt[-2].ival); } break;
case 9:
# line 131 "perly.y"
{ yyval.ival = savestack->ary_fill; } break;
case 10:
# line 135 "perly.y"
{ yyval.cmdval = Nullcmd; } break;
case 11:
# line 137 "perly.y"
{ yyval.cmdval = append_line(yypvt[-1].cmdval,yypvt[-0].cmdval); } break;
case 12:
# line 141 "perly.y"
{ yyval.cmdval = Nullcmd; } break;
case 13:
# line 143 "perly.y"
{ yyval.cmdval = add_label(yypvt[-1].cval,yypvt[-0].cmdval); } break;
case 15:
# line 146 "perly.y"
{ if (yypvt[-1].cval != Nullch) {
			      yyval.cmdval = add_label(yypvt[-1].cval, make_acmd(C_EXPR, Nullstab,
				  Nullarg, Nullarg) );
			    }
			    else {
			      yyval.cmdval = Nullcmd;
			      cmdline = NOLINE;
			    } } break;
case 16:
# line 155 "perly.y"
{ yyval.cmdval = add_label(yypvt[-2].cval,yypvt[-1].cmdval); } break;
case 17:
# line 159 "perly.y"
{ yyval.cmdval = Nullcmd; } break;
case 18:
# line 161 "perly.y"
{ yyval.cmdval = make_acmd(C_EXPR, Nullstab, yypvt[-0].arg, Nullarg); } break;
case 19:
# line 163 "perly.y"
{ yyval.cmdval = addcond(
			       make_acmd(C_EXPR, Nullstab, Nullarg, yypvt[-2].arg), yypvt[-0].arg); } break;
case 20:
# line 166 "perly.y"
{ yyval.cmdval = addcond(invert(
			       make_acmd(C_EXPR, Nullstab, Nullarg, yypvt[-2].arg)), yypvt[-0].arg); } break;
case 21:
# line 169 "perly.y"
{ yyval.cmdval = addloop(
			       make_acmd(C_EXPR, Nullstab, Nullarg, yypvt[-2].arg), yypvt[-0].arg); } break;
case 22:
# line 172 "perly.y"
{ yyval.cmdval = addloop(invert(
			       make_acmd(C_EXPR, Nullstab, Nullarg, yypvt[-2].arg)), yypvt[-0].arg); } break;
case 23:
# line 177 "perly.y"
{ cmdline = yypvt[-4].ival;
			    yyval.cmdval = make_icmd(C_IF,yypvt[-2].arg,yypvt[-0].compval); } break;
case 24:
# line 180 "perly.y"
{ cmdline = yypvt[-4].ival;
			    yyval.cmdval = invert(make_icmd(C_IF,yypvt[-2].arg,yypvt[-0].compval)); } break;
case 25:
# line 183 "perly.y"
{ cmdline = yypvt[-2].ival;
			    yyval.cmdval = make_ccmd(C_IF,cmd_to_arg(yypvt[-1].cmdval),yypvt[-0].compval); } break;
case 26:
# line 186 "perly.y"
{ cmdline = yypvt[-2].ival;
			    yyval.cmdval = invert(make_ccmd(C_IF,cmd_to_arg(yypvt[-1].cmdval),yypvt[-0].compval)); } break;
case 27:
# line 191 "perly.y"
{ cmdline = yypvt[-4].ival;
			    yyval.cmdval = wopt(add_label(yypvt[-5].cval,
			    make_ccmd(C_WHILE,yypvt[-2].arg,yypvt[-0].compval) )); } break;
case 28:
# line 195 "perly.y"
{ cmdline = yypvt[-4].ival;
			    yyval.cmdval = wopt(add_label(yypvt[-5].cval,
			    invert(make_ccmd(C_WHILE,yypvt[-2].arg,yypvt[-0].compval)) )); } break;
case 29:
# line 199 "perly.y"
{ cmdline = yypvt[-2].ival;
			    yyval.cmdval = wopt(add_label(yypvt[-3].cval,
			    make_ccmd(C_WHILE, cmd_to_arg(yypvt[-1].cmdval),yypvt[-0].compval) )); } break;
case 30:
# line 203 "perly.y"
{ cmdline = yypvt[-2].ival;
			    yyval.cmdval = wopt(add_label(yypvt[-3].cval,
			    invert(make_ccmd(C_WHILE, cmd_to_arg(yypvt[-1].cmdval),yypvt[-0].compval)) )); } break;
case 31:
# line 207 "perly.y"
{ cmdline = yypvt[-5].ival;
			    /*
			     * The following gobbledygook catches EXPRs that
			     * aren't explicit array refs and translates
			     *		foreach VAR (EXPR) {
			     * into
			     *		@ary = EXPR;
			     *		foreach VAR (@ary) {
			     * where @ary is a hidden array made by genstab().
			     * (Note that @ary may become a local array if
			     * it is determined that it might be called
			     * recursively.  See cmd_tosave().)
			     */
			    if (yypvt[-2].arg->arg_type != O_ARRAY) {
				scrstab = aadd(genstab());
				yyval.cmdval = append_line(
				    make_acmd(C_EXPR, Nullstab,
				      l(make_op(O_ASSIGN,2,
					listish(make_op(O_ARRAY, 1,
					  stab2arg(A_STAB,scrstab),
					  Nullarg,Nullarg )),
					listish(make_list(yypvt[-2].arg)),
					Nullarg)),
				      Nullarg),
				    wopt(over(yypvt[-4].stabval,add_label(yypvt[-6].cval,
				      make_ccmd(C_WHILE,
					make_op(O_ARRAY, 1,
					  stab2arg(A_STAB,scrstab),
					  Nullarg,Nullarg ),
					yypvt[-0].compval)))));
				yyval.cmdval->c_line = yypvt[-5].ival;
				yyval.cmdval->c_head->c_line = yypvt[-5].ival;
			    }
			    else {
				yyval.cmdval = wopt(over(yypvt[-4].stabval,add_label(yypvt[-6].cval,
				make_ccmd(C_WHILE,yypvt[-2].arg,yypvt[-0].compval) )));
			    }
			} break;
case 32:
# line 246 "perly.y"
{ cmdline = yypvt[-4].ival;
			    if (yypvt[-2].arg->arg_type != O_ARRAY) {
				scrstab = aadd(genstab());
				yyval.cmdval = append_line(
				    make_acmd(C_EXPR, Nullstab,
				      l(make_op(O_ASSIGN,2,
					listish(make_op(O_ARRAY, 1,
					  stab2arg(A_STAB,scrstab),
					  Nullarg,Nullarg )),
					listish(make_list(yypvt[-2].arg)),
					Nullarg)),
				      Nullarg),
				    wopt(over(defstab,add_label(yypvt[-5].cval,
				      make_ccmd(C_WHILE,
					make_op(O_ARRAY, 1,
					  stab2arg(A_STAB,scrstab),
					  Nullarg,Nullarg ),
					yypvt[-0].compval)))));
				yyval.cmdval->c_line = yypvt[-4].ival;
				yyval.cmdval->c_head->c_line = yypvt[-4].ival;
			    }
			    else {	/* lisp, anyone? */
				yyval.cmdval = wopt(over(defstab,add_label(yypvt[-5].cval,
				make_ccmd(C_WHILE,yypvt[-2].arg,yypvt[-0].compval) )));
			    }
			} break;
case 33:
# line 274 "perly.y"
{   yyval.compval.comp_true = yypvt[-0].cmdval;
			    yyval.compval.comp_alt = yypvt[-2].cmdval;
			    cmdline = yypvt[-8].ival;
			    yyval.cmdval = append_line(yypvt[-6].cmdval,wopt(add_label(yypvt[-9].cval,
				make_ccmd(C_WHILE,yypvt[-4].arg,yyval.compval) ))); } break;
case 34:
# line 280 "perly.y"
{ yyval.cmdval = add_label(yypvt[-1].cval,make_ccmd(C_BLOCK,Nullarg,yypvt[-0].compval)); } break;
case 35:
# line 284 "perly.y"
{ yyval.cmdval = Nullcmd; } break;
case 37:
# line 289 "perly.y"
{ (void)scanstr("1"); yyval.arg = yylval.arg; } break;
case 39:
# line 294 "perly.y"
{ yyval.cval = Nullch; } break;
case 41:
# line 299 "perly.y"
{ yyval.ival = 0; } break;
case 42:
# line 301 "perly.y"
{ yyval.ival = 0; } break;
case 43:
# line 303 "perly.y"
{ yyval.ival = 0; } break;
case 44:
# line 307 "perly.y"
{ if (strEQ(yypvt[-2].cval,"stdout"))
			    make_form(stabent("STDOUT",TRUE),yypvt[-0].formval);
			  else if (strEQ(yypvt[-2].cval,"stderr"))
			    make_form(stabent("STDERR",TRUE),yypvt[-0].formval);
			  else
			    make_form(stabent(yypvt[-2].cval,TRUE),yypvt[-0].formval);
			  Safefree(yypvt[-2].cval); yypvt[-2].cval = Nullch; } break;
case 45:
# line 315 "perly.y"
{ make_form(stabent("STDOUT",TRUE),yypvt[-0].formval); } break;
case 46:
# line 319 "perly.y"
{ make_sub(yypvt[-1].cval,yypvt[-0].cmdval);
			  cmdline = NOLINE;
			  if (savestack->ary_fill > yypvt[-2].ival)
			    restorelist(yypvt[-2].ival); } break;
case 47:
# line 326 "perly.y"
{ char tmpbuf[256];
			  STAB *tmpstab;

			  savehptr(&curstash);
			  saveitem(curstname);
			  str_set(curstname,yypvt[-1].cval);
			  sprintf(tmpbuf,"'_%s",yypvt[-1].cval);
			  tmpstab = stabent(tmpbuf,TRUE);
			  if (!stab_xhash(tmpstab))
			      stab_xhash(tmpstab) = hnew(0);
			  curstash = stab_xhash(tmpstab);
			  if (!curstash->tbl_name)
			      curstash->tbl_name = savestr(yypvt[-1].cval);
			  curstash->tbl_coeffsize = 0;
			  Safefree(yypvt[-1].cval); yypvt[-1].cval = Nullch;
			  cmdline = NOLINE;
			} break;
case 48:
# line 346 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 49:
# line 350 "perly.y"
{ yyval.arg = make_op(O_COMMA, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 51:
# line 355 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 52:
# line 359 "perly.y"
{   yypvt[-2].arg = listish(yypvt[-2].arg);
			    if (yypvt[-2].arg->arg_type == O_ASSIGN && yypvt[-2].arg->arg_len == 1)
				yypvt[-2].arg->arg_type = O_ITEM;	/* a local() */
			    if (yypvt[-2].arg->arg_type == O_LIST)
				yypvt[-0].arg = listish(yypvt[-0].arg);
			    yyval.arg = l(make_op(O_ASSIGN, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg)); } break;
case 53:
# line 366 "perly.y"
{ yyval.arg = l(make_op(O_POW, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 54:
# line 368 "perly.y"
{ yyval.arg = l(make_op(yypvt[-2].ival, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 55:
# line 370 "perly.y"
{ yyval.arg = rcatmaybe(l(make_op(yypvt[-2].ival, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)));} break;
case 56:
# line 372 "perly.y"
{ yyval.arg = l(make_op(O_LEFT_SHIFT, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 57:
# line 374 "perly.y"
{ yyval.arg = l(make_op(O_RIGHT_SHIFT, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 58:
# line 376 "perly.y"
{ yyval.arg = l(make_op(O_BIT_AND, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 59:
# line 378 "perly.y"
{ yyval.arg = l(make_op(O_XOR, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 60:
# line 380 "perly.y"
{ yyval.arg = l(make_op(O_BIT_OR, 2, yypvt[-3].arg, yypvt[-0].arg, Nullarg)); } break;
case 61:
# line 384 "perly.y"
{ yyval.arg = make_op(O_POW, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 62:
# line 386 "perly.y"
{ if (yypvt[-1].ival == O_REPEAT)
			      yypvt[-2].arg = listish(yypvt[-2].arg);
			    yyval.arg = make_op(yypvt[-1].ival, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg);
			    if (yypvt[-1].ival == O_REPEAT) {
				if (yyval.arg[1].arg_type != A_EXPR ||
				  yyval.arg[1].arg_ptr.arg_arg->arg_type != O_LIST)
				    yyval.arg[1].arg_flags &= ~AF_ARYOK;
			    } } break;
case 63:
# line 395 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 64:
# line 397 "perly.y"
{ yyval.arg = make_op(O_LEFT_SHIFT, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 65:
# line 399 "perly.y"
{ yyval.arg = make_op(O_RIGHT_SHIFT, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 66:
# line 401 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 67:
# line 403 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 68:
# line 405 "perly.y"
{ yyval.arg = make_op(O_BIT_AND, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 69:
# line 407 "perly.y"
{ yyval.arg = make_op(O_XOR, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 70:
# line 409 "perly.y"
{ yyval.arg = make_op(O_BIT_OR, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 71:
# line 411 "perly.y"
{ arg4 = Nullarg;
			  yyval.arg = make_op(O_F_OR_R, 4, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 72:
# line 414 "perly.y"
{ yyval.arg = make_op(O_AND, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 73:
# line 416 "perly.y"
{ yyval.arg = make_op(O_OR, 2, yypvt[-2].arg, yypvt[-0].arg, Nullarg); } break;
case 74:
# line 418 "perly.y"
{ yyval.arg = make_op(O_COND_EXPR, 3, yypvt[-4].arg, yypvt[-2].arg, yypvt[-0].arg); } break;
case 75:
# line 420 "perly.y"
{ yyval.arg = mod_match(O_MATCH, yypvt[-2].arg, yypvt[-0].arg); } break;
case 76:
# line 422 "perly.y"
{ yyval.arg = mod_match(O_NMATCH, yypvt[-2].arg, yypvt[-0].arg); } break;
case 77:
# line 424 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 78:
# line 428 "perly.y"
{ yyval.arg = make_op(O_NEGATE, 1, yypvt[-0].arg, Nullarg, Nullarg); } break;
case 79:
# line 430 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 80:
# line 432 "perly.y"
{ yyval.arg = make_op(O_NOT, 1, yypvt[-0].arg, Nullarg, Nullarg); } break;
case 81:
# line 434 "perly.y"
{ yyval.arg = make_op(O_COMPLEMENT, 1, yypvt[-0].arg, Nullarg, Nullarg);} break;
case 82:
# line 436 "perly.y"
{ yyval.arg = addflags(1, AF_POST|AF_UP,
			    l(make_op(O_ITEM,1,yypvt[-1].arg,Nullarg,Nullarg))); } break;
case 83:
# line 439 "perly.y"
{ yyval.arg = addflags(1, AF_POST,
			    l(make_op(O_ITEM,1,yypvt[-1].arg,Nullarg,Nullarg))); } break;
case 84:
# line 442 "perly.y"
{ yyval.arg = addflags(1, AF_PRE|AF_UP,
			    l(make_op(O_ITEM,1,yypvt[-0].arg,Nullarg,Nullarg))); } break;
case 85:
# line 445 "perly.y"
{ yyval.arg = addflags(1, AF_PRE,
			    l(make_op(O_ITEM,1,yypvt[-0].arg,Nullarg,Nullarg))); } break;
case 86:
# line 448 "perly.y"
{ opargs[yypvt[-1].ival] = 0;	/* force it special */
			    yyval.arg = make_op(yypvt[-1].ival, 1,
				stab2arg(A_STAB,stabent(yypvt[-0].cval,TRUE)),
				Nullarg, Nullarg);
			} break;
case 87:
# line 454 "perly.y"
{ opargs[yypvt[-1].ival] = 1;
			    yyval.arg = make_op(yypvt[-1].ival, 1, yypvt[-0].arg, Nullarg, Nullarg); } break;
case 88:
# line 457 "perly.y"
{ opargs[yypvt[-0].ival] = (yypvt[-0].ival != O_FTTTY);
			    yyval.arg = make_op(yypvt[-0].ival, 1,
				stab2arg(A_STAB,
				  yypvt[-0].ival == O_FTTTY?stabent("STDIN",TRUE):defstab),
				Nullarg, Nullarg); } break;
case 89:
# line 463 "perly.y"
{ yyval.arg = l(localize(make_op(O_ASSIGN, 1,
				localize(listish(make_list(yypvt[-1].arg))),
				Nullarg,Nullarg))); } break;
case 90:
# line 467 "perly.y"
{ yyval.arg = make_list(yypvt[-1].arg); } break;
case 91:
# line 469 "perly.y"
{ yyval.arg = make_list(Nullarg); } break;
case 92:
# line 471 "perly.y"
{ yyval.arg = make_op(O_DOFILE,2,yypvt[-0].arg,Nullarg,Nullarg);
			  allstabs = TRUE;} break;
case 93:
# line 474 "perly.y"
{ yyval.arg = cmd_to_arg(yypvt[-0].cmdval); } break;
case 94:
# line 476 "perly.y"
{ yyval.arg = stab2arg(A_STAB,yypvt[-0].stabval); } break;
case 95:
# line 478 "perly.y"
{ yyval.arg = stab2arg(A_STAR,yypvt[-0].stabval); } break;
case 96:
# line 480 "perly.y"
{ yyval.arg = make_op(O_AELEM, 2,
				stab2arg(A_STAB,aadd(yypvt[-3].stabval)), yypvt[-1].arg, Nullarg); } break;
case 97:
# line 483 "perly.y"
{ yyval.arg = make_op(O_HASH, 1,
				stab2arg(A_STAB,yypvt[-0].stabval),
				Nullarg, Nullarg); } break;
case 98:
# line 487 "perly.y"
{ yyval.arg = make_op(O_ARRAY, 1,
				stab2arg(A_STAB,yypvt[-0].stabval),
				Nullarg, Nullarg); } break;
case 99:
# line 491 "perly.y"
{ yyval.arg = make_op(O_HELEM, 2,
				stab2arg(A_STAB,hadd(yypvt[-3].stabval)),
				jmaybe(yypvt[-1].arg),
				Nullarg); } break;
case 100:
# line 496 "perly.y"
{ yyval.arg = make_op(O_LSLICE, 3,
				Nullarg,
				listish(make_list(yypvt[-1].arg)),
				listish(make_list(yypvt[-4].arg))); } break;
case 101:
# line 501 "perly.y"
{ yyval.arg = make_op(O_LSLICE, 3,
				Nullarg,
				listish(make_list(yypvt[-1].arg)),
				Nullarg); } break;
case 102:
# line 506 "perly.y"
{ yyval.arg = make_op(O_ASLICE, 2,
				stab2arg(A_STAB,aadd(yypvt[-3].stabval)),
				listish(make_list(yypvt[-1].arg)),
				Nullarg); } break;
case 103:
# line 511 "perly.y"
{ yyval.arg = make_op(O_HSLICE, 2,
				stab2arg(A_STAB,hadd(yypvt[-3].stabval)),
				listish(make_list(yypvt[-1].arg)),
				Nullarg); } break;
case 104:
# line 516 "perly.y"
{ yyval.arg = make_op(O_DELETE, 2,
				stab2arg(A_STAB,hadd(yypvt[-3].stabval)),
				jmaybe(yypvt[-1].arg),
				Nullarg); } break;
case 105:
# line 521 "perly.y"
{ yyval.arg = stab2arg(A_ARYLEN,yypvt[-0].stabval); } break;
case 106:
# line 523 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 107:
# line 525 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 108:
# line 527 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 109:
# line 529 "perly.y"
{ yyval.arg = yypvt[-0].arg; } break;
case 110:
# line 531 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_WORD,stabent(yypvt[-3].cval,MULTI)),
				make_list(yypvt[-1].arg),
				Nullarg); Safefree(yypvt[-3].cval); yypvt[-3].cval = Nullch;
			    yyval.arg->arg_flags |= AF_DEPR; } break;
case 111:
# line 537 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_WORD,stabent(yypvt[-3].cval,MULTI)),
				make_list(yypvt[-1].arg),
				Nullarg); Safefree(yypvt[-3].cval); yypvt[-3].cval = Nullch; } break;
case 112:
# line 542 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_WORD,stabent(yypvt[-2].cval,MULTI)),
				make_list(Nullarg),
				Nullarg);
			    yyval.arg->arg_flags |= AF_DEPR; } break;
case 113:
# line 548 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_WORD,stabent(yypvt[-2].cval,MULTI)),
				make_list(Nullarg),
				Nullarg); } break;
case 114:
# line 553 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_WORD,stabent(yypvt[-0].cval,MULTI)),
				Nullarg,
				Nullarg); } break;
case 115:
# line 558 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_STAB,yypvt[-3].stabval),
				make_list(yypvt[-1].arg),
				Nullarg);
			    yyval.arg->arg_flags |= AF_DEPR; } break;
case 116:
# line 564 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_STAB,yypvt[-3].stabval),
				make_list(yypvt[-1].arg),
				Nullarg); } break;
case 117:
# line 569 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_STAB,yypvt[-2].stabval),
				make_list(Nullarg),
				Nullarg);
			    yyval.arg->arg_flags |= AF_DEPR; } break;
case 118:
# line 575 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_STAB,yypvt[-2].stabval),
				make_list(Nullarg),
				Nullarg); } break;
case 119:
# line 580 "perly.y"
{ yyval.arg = make_op((perldb ? O_DBSUBR : O_SUBR), 2,
				stab2arg(A_STAB,yypvt[-0].stabval),
				Nullarg,
				Nullarg); } break;
case 120:
# line 585 "perly.y"
{ yyval.arg = make_op(yypvt[-0].ival,0,Nullarg,Nullarg,Nullarg); } break;
case 121:
# line 587 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival,1,cval_to_arg(yypvt[-0].cval),
			    Nullarg,Nullarg); } break;
case 122:
# line 590 "perly.y"
{ yyval.arg = make_op(yypvt[-0].ival,0,Nullarg,Nullarg,Nullarg); } break;
case 123:
# line 592 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival,1,cmd_to_arg(yypvt[-0].cmdval),Nullarg,Nullarg); } break;
case 124:
# line 594 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival,1,yypvt[-0].arg,Nullarg,Nullarg); } break;
case 125:
# line 596 "perly.y"
{ yyval.arg = make_op(O_SELECT, 0, Nullarg, Nullarg, Nullarg);} break;
case 126:
# line 598 "perly.y"
{ yyval.arg = make_op(O_SELECT, 1,
			    stab2arg(A_WORD,stabent(yypvt[-0].cval,TRUE)),
			    Nullarg,
			    Nullarg);
			    Safefree(yypvt[-0].cval); yypvt[-0].cval = Nullch; } break;
case 127:
# line 604 "perly.y"
{ yyval.arg = make_op(O_SELECT, 1, yypvt[-1].arg, Nullarg, Nullarg); } break;
case 128:
# line 606 "perly.y"
{ arg4 = yypvt[-1].arg;
			  yyval.arg = make_op(O_SSELECT, 4, yypvt[-4].arg, yypvt[-3].arg, yypvt[-2].arg); } break;
case 129:
# line 609 "perly.y"
{ yyval.arg = make_op(O_OPEN, 2,
			    stab2arg(A_WORD,stabent(yypvt[-0].cval,TRUE)),
			    stab2arg(A_STAB,stabent(yypvt[-0].cval,TRUE)),
			    Nullarg); } break;
case 130:
# line 614 "perly.y"
{ yyval.arg = make_op(O_OPEN, 2,
			    stab2arg(A_WORD,stabent(yypvt[-1].cval,TRUE)),
			    stab2arg(A_STAB,stabent(yypvt[-1].cval,TRUE)),
			    Nullarg); } break;
case 131:
# line 619 "perly.y"
{ yyval.arg = make_op(O_OPEN, 2,
			    yypvt[-2].arg,
			    yypvt[-1].arg, Nullarg); } break;
case 132:
# line 623 "perly.y"
{ yyval.arg = make_op(yypvt[-3].ival, 1,
			    yypvt[-1].arg,
			    Nullarg, Nullarg); } break;
case 133:
# line 627 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 1,
			    stab2arg(A_WORD,stabent(yypvt[-0].cval,TRUE)),
			    Nullarg, Nullarg);
			  Safefree(yypvt[-0].cval); yypvt[-0].cval = Nullch; } break;
case 134:
# line 632 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 1,
			    stab2arg(A_STAB,yypvt[-0].stabval),
			    Nullarg, Nullarg); } break;
case 135:
# line 636 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival, 1,
			    stab2arg(A_WORD,Nullstab),
			    Nullarg, Nullarg); } break;
case 136:
# line 640 "perly.y"
{ yyval.arg = make_op(yypvt[-0].ival, 0,
			    Nullarg, Nullarg, Nullarg); } break;
case 137:
# line 643 "perly.y"
{ yyval.arg = make_op(yypvt[-4].ival, 2, yypvt[-2].arg, yypvt[-1].arg, Nullarg); } break;
case 138:
# line 645 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 3, yypvt[-3].arg, yypvt[-2].arg, make_list(yypvt[-1].arg)); } break;
case 139:
# line 647 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 2, yypvt[-3].arg, yypvt[-1].arg, Nullarg); } break;
case 140:
# line 649 "perly.y"
{ arg4 = yypvt[-1].arg; yyval.arg = make_op(yypvt[-6].ival, 4, yypvt[-4].arg, yypvt[-3].arg, yypvt[-2].arg); } break;
case 141:
# line 651 "perly.y"
{ arg4 = yypvt[-2].arg; arg5 = yypvt[-1].arg;
			  yyval.arg = make_op(yypvt[-8].ival, 5, yypvt[-6].arg, yypvt[-4].arg, yypvt[-3].arg); } break;
case 142:
# line 654 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 2,
			    yypvt[-3].arg,
			    make_list(yypvt[-1].arg),
			    Nullarg); } break;
case 143:
# line 659 "perly.y"
{ yyval.arg = make_op(O_POP, 1, yypvt[-0].arg, Nullarg, Nullarg); } break;
case 144:
# line 661 "perly.y"
{ yyval.arg = make_op(O_POP, 1, yypvt[-1].arg, Nullarg, Nullarg); } break;
case 145:
# line 663 "perly.y"
{ yyval.arg = make_op(O_SHIFT, 1, yypvt[-0].arg, Nullarg, Nullarg); } break;
case 146:
# line 665 "perly.y"
{ yyval.arg = make_op(O_SHIFT, 1, yypvt[-1].arg, Nullarg, Nullarg); } break;
case 147:
# line 667 "perly.y"
{ yyval.arg = make_op(O_SHIFT, 1,
			    stab2arg(A_STAB,
			      aadd(stabent(subline ? "_" : "ARGV", TRUE))),
			    Nullarg, Nullarg); } break;
case 148:
# line 672 "perly.y"
{   static char p[]="/\\s+/";
			    char *oldend = bufend;
			    ARG *oldarg = yylval.arg;
			    
			    bufend=p+5;
			    (void)scanpat(p);
			    bufend=oldend;
			    yyval.arg = make_split(defstab,yylval.arg,Nullarg);
			    yylval.arg = oldarg; } break;
case 149:
# line 682 "perly.y"
{ yyval.arg = mod_match(O_MATCH, yypvt[-2].arg,
			  make_split(defstab,yypvt[-3].arg,yypvt[-1].arg));} break;
case 150:
# line 685 "perly.y"
{ yyval.arg = mod_match(O_MATCH, yypvt[-1].arg,
			  make_split(defstab,yypvt[-2].arg,Nullarg) ); } break;
case 151:
# line 688 "perly.y"
{ yyval.arg = mod_match(O_MATCH,
			    stab2arg(A_STAB,defstab),
			    make_split(defstab,yypvt[-1].arg,Nullarg) ); } break;
case 152:
# line 692 "perly.y"
{ yyval.arg = make_op(yypvt[-4].ival, 2,
			    yypvt[-2].arg,
			    listish(make_list(yypvt[-1].arg)),
			    Nullarg); } break;
case 153:
# line 697 "perly.y"
{ yyval.arg = make_op(yypvt[-3].ival, 1,
			    make_list(yypvt[-1].arg),
			    Nullarg,
			    Nullarg); } break;
case 154:
# line 702 "perly.y"
{ yyval.arg = l(make_op(yypvt[-1].ival, 1, fixl(yypvt[-1].ival,yypvt[-0].arg),
			    Nullarg, Nullarg)); } break;
case 155:
# line 705 "perly.y"
{ yyval.arg = l(make_op(yypvt[-0].ival, 1,
			    stab2arg(A_STAB,defstab),
			    Nullarg, Nullarg)); } break;
case 156:
# line 709 "perly.y"
{ yyval.arg = make_op(yypvt[-0].ival, 0, Nullarg, Nullarg, Nullarg); } break;
case 157:
# line 711 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival, 0, Nullarg, Nullarg, Nullarg); } break;
case 158:
# line 713 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival, 0, Nullarg, Nullarg, Nullarg); } break;
case 159:
# line 715 "perly.y"
{ yyval.arg = make_op(yypvt[-3].ival, 1, yypvt[-1].arg, Nullarg, Nullarg); } break;
case 160:
# line 717 "perly.y"
{ yyval.arg = make_op(yypvt[-4].ival, 2, yypvt[-2].arg, yypvt[-1].arg, Nullarg);
			    if (yypvt[-4].ival == O_INDEX && yyval.arg[2].arg_type == A_SINGLE)
				fbmcompile(yyval.arg[2].arg_ptr.arg_str,0); } break;
case 161:
# line 721 "perly.y"
{ yyval.arg = make_op(yypvt[-4].ival, 2, yypvt[-2].arg, yypvt[-1].arg, Nullarg);
			    if (yypvt[-4].ival == O_INDEX && yyval.arg[2].arg_type == A_SINGLE)
				fbmcompile(yyval.arg[2].arg_ptr.arg_str,0); } break;
case 162:
# line 725 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 3, yypvt[-3].arg, yypvt[-2].arg, yypvt[-1].arg);
			    if (yypvt[-5].ival == O_INDEX && yyval.arg[2].arg_type == A_SINGLE)
				fbmcompile(yyval.arg[2].arg_ptr.arg_str,0); } break;
case 163:
# line 729 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 3, yypvt[-3].arg, yypvt[-2].arg, yypvt[-1].arg); } break;
case 164:
# line 731 "perly.y"
{ arg4 = yypvt[-1].arg;
			  yyval.arg = make_op(yypvt[-6].ival, 4, yypvt[-4].arg, yypvt[-3].arg, yypvt[-2].arg); } break;
case 165:
# line 734 "perly.y"
{ arg4 = yypvt[-2].arg; arg5 = yypvt[-1].arg;
			  yyval.arg = make_op(yypvt[-7].ival, 5, yypvt[-5].arg, yypvt[-4].arg, yypvt[-3].arg); } break;
case 166:
# line 737 "perly.y"
{ yyval.arg = make_op(yypvt[-3].ival, 1,
				yypvt[-1].arg,
				Nullarg,
				Nullarg); } break;
case 167:
# line 742 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival, 1,
				yypvt[-0].arg,
				Nullarg,
				Nullarg); } break;
case 168:
# line 747 "perly.y"
{ yyval.arg = make_op(yypvt[-5].ival, 3, yypvt[-3].arg, yypvt[-2].arg, yypvt[-1].arg); } break;
case 171:
# line 753 "perly.y"
{ yyval.arg = make_op(yypvt[-0].ival,2,
				stab2arg(A_WORD,Nullstab),
				stab2arg(A_STAB,defstab),
				Nullarg); } break;
case 172:
# line 758 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival,2,
				stab2arg(A_WORD,Nullstab),
				maybelistish(yypvt[-1].ival,make_list(yypvt[-0].arg)),
				Nullarg); } break;
case 173:
# line 763 "perly.y"
{ yyval.arg = make_op(yypvt[-1].ival,2,
				stab2arg(A_WORD,stabent(yypvt[-0].cval,TRUE)),
				stab2arg(A_STAB,defstab),
				Nullarg); } break;
case 174:
# line 768 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival,2,
				stab2arg(A_WORD,stabent(yypvt[-1].cval,TRUE)),
				maybelistish(yypvt[-2].ival,make_list(yypvt[-0].arg)),
				Nullarg); Safefree(yypvt[-1].cval); yypvt[-1].cval = Nullch; } break;
case 175:
# line 773 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival,2,
				stab2arg(A_STAB,yypvt[-1].stabval),
				maybelistish(yypvt[-2].ival,make_list(yypvt[-0].arg)),
				Nullarg); } break;
case 176:
# line 778 "perly.y"
{ yyval.arg = make_op(yypvt[-2].ival,2,
				cmd_to_arg(yypvt[-1].cmdval),
				maybelistish(yypvt[-2].ival,make_list(yypvt[-0].arg)),
				Nullarg); } break;
case 177:
# line 785 "perly.y"
{ yyval.arg = stab2arg(A_WORD,stabent(yypvt[-0].cval,TRUE));
			  Safefree(yypvt[-0].cval); yypvt[-0].cval = Nullch;} break;
case 179:
# line 791 "perly.y"
{ yyval.arg = stab2arg(A_WORD,aadd(stabent(yypvt[-0].cval,TRUE)));
			    Safefree(yypvt[-0].cval); yypvt[-0].cval = Nullch; } break;
case 180:
# line 794 "perly.y"
{ yyval.arg = stab2arg(A_STAB,yypvt[-0].stabval); } break;
case 181:
# line 798 "perly.y"
{ yyval.arg = stab2arg(A_WORD,hadd(stabent(yypvt[-0].cval,TRUE)));
			    Safefree(yypvt[-0].cval); yypvt[-0].cval = Nullch; } break;
case 182:
# line 801 "perly.y"
{ yyval.arg = stab2arg(A_STAB,yypvt[-0].stabval); } break;
case 183:
# line 805 "perly.y"
{ yyval.ival = 1; } break;
case 184:
# line 807 "perly.y"
{ yyval.ival = 0; } break;
case 185:
# line 816 "perly.y"
{ char *s;
			    yyval.arg = op_new(1);
			    yyval.arg->arg_type = O_ITEM;
			    yyval.arg[1].arg_type = A_SINGLE;
			    yyval.arg[1].arg_ptr.arg_str = str_make(yypvt[-0].cval,0);
			    for (s = yypvt[-0].cval; *s && isLOWER(*s); s++) ;
			    if (dowarn && !*s)
				warn(
				  "\"%s\" may clash with future reserved word",
				  yypvt[-0].cval );
			} break;
		}
		goto yystack;  /* stack new state and value */

	}
