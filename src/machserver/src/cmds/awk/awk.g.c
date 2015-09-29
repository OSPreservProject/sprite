# define FIRSTTOKEN 257
# define FINAL 258
# define FATAL 259
# define LT 260
# define LE 261
# define GT 262
# define GE 263
# define EQ 264
# define NE 265
# define MATCH 266
# define NOTMATCH 267
# define APPEND 268
# define ADD 269
# define MINUS 270
# define MULT 271
# define DIVIDE 272
# define MOD 273
# define UMINUS 274
# define ASSIGN 275
# define ADDEQ 276
# define SUBEQ 277
# define MULTEQ 278
# define DIVEQ 279
# define MODEQ 280
# define JUMP 281
# define XBEGIN 282
# define XEND 283
# define NL 284
# define PRINT 285
# define PRINTF 286
# define SPRINTF 287
# define SPLIT 288
# define IF 289
# define ELSE 290
# define WHILE 291
# define FOR 292
# define IN 293
# define NEXT 294
# define EXIT 295
# define BREAK 296
# define CONTINUE 297
# define PROGRAM 298
# define PASTAT 299
# define PASTAT2 300
# define ASGNOP 301
# define BOR 302
# define AND 303
# define NOT 304
# define NUMBER 305
# define VAR 306
# define ARRAY 307
# define FNCN 308
# define SUBSTR 309
# define LSUBSTR 310
# define INDEX 311
# define GETLINE 312
# define RELOP 313
# define MATCHOP 314
# define OR 315
# define STRING 316
# define DOT 317
# define CCL 318
# define NCCL 319
# define CHAR 320
# define CAT 321
# define STAR 322
# define PLUS 323
# define QUEST 324
# define POSTINCR 325
# define PREINCR 326
# define POSTDECR 327
# define PREDECR 328
# define INCR 329
# define DECR 330
# define FIELD 331
# define INDIRECT 332
# define LASTTOKEN 333

# line 36 "awk.g.y"
#include "awk.def"
#ifndef	DEBUG	
#	define	PUTS(x)
#endif
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 274 "awk.g.y"

short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 122
# define YYLAST 1727
short yyact[]={

  55,  33,  35,  30,  69,  29,  67,  68, 159, 160,
 161, 177,  34, 225, 193, 201, 228, 200, 201, 155,
  45,  46, 209,  48,  46, 172, 114,  37,  38,  82,
 115, 197,  67,  68,  40,   6,   3,  43, 175,  55,
 182,  44,  30,   7,  29,  41,  15, 183,  55, 158,
  82,  30,  73,  29,  71, 210,  13, 181,  17,  66,
  57,  15,   4,  14,  64,  62,  39,  63, 132,  65,
  11,  66, 202, 236, 234, 152,  64,  59,  14,  49,
 230,  65, 122,  94, 113, 212,  15,  15, 120,  15,
  60,  61, 216, 221, 166, 215,  55, 132, 119,  30,
 108,  29,  47,  14,  14, 151,  14, 150, 116,  77,
  76, 102,  75,  70, 105,  16, 103, 104,  89, 106,
  88,  50,   9,  87,  36, 171,   8, 134,   5,   2,
   1,  94, 139, 174,   0,  56, 142, 143, 145, 146,
   0,   0,   0,   0,  55,   0, 196,  30,   0,  29,
 148, 149,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 147,   0,   0,   0, 162,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  94,
   0, 154,   0,   0,   0,   0,   0,   0, 178,   0,
 173,   0,  55,   0,   0,  30,   0,  29,  45,  46,
 176,  86,   0,   0,   0,   0,   0, 190,   0,   0,
   0,   0,   0,   0,   0,   0,  98,   0,   0,   0,
   0,   0, 189,  19,   0, 204,  30,  94,  29, 153,
  17, 211, 203,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 206,   0,   0,  95,  96,  25,  27,  99,
   0, 100, 101,   0,  90,  91,  92,  93,  55, 200,
 201,  30,  42,  29,  98,  33,  35,   0,  24,  26,
   0,  28,  23,   0, 114,  94,  34, 107, 115, 191,
   0,  45,  46, 200, 201,   0,  25,  27,   0,  31,
  32,  37,  38,  95,  96,  25,  27,  99,   0, 100,
 101,   0,  90,  91,  92,  93,  12, 110, 111, 112,
 109,   0,  98,  33,  35,  34,  24,  26,   0,  28,
  23,   0,  55,   0,  34,  30,   0,  29,  31,  32,
  37,  38, 113, 200, 201,   0,   0,  31,  32,  37,
  38,  95,  96,  25,  27,  99,   0, 100, 101, 220,
  90,  91,  92,  93, 187,   0, 222,   0,   0,   0,
  98,  33,  35,   0,  24,  26,   0,  28,  23,   0,
  55,   0,  34,  30,   0,  29,   0, 235,   0,   0,
   0, 237,   0, 239,   0,  31,  32,  37,  38,  95,
  96,  25,  27,  99,   0, 100, 101,   0,  90,  91,
  92,  93,   0,   0,   0,  94,   0,  84,  98,  33,
  35,   0,  24,  26,   0,  28,  23,   0,  55, 121,
  34,  30,   0,  29,   0,   0, 229,   0,   0,   0,
 233,   0,   0,  31,  32,  37,  38,  95,  96,  25,
  27,  99,   0, 100, 101,   0,  90,  91,  92,  93,
   0,  55,   0,  94,  30,   0,  29,  33,  35,   0,
  24,  26,   0,  28,  23,   0,  10,   0,  34,   0,
  25,  27,   0,   0,  98,  85,   0,   0,   0,   0,
   0,  31,  32,  37,  38,   0,   0,  20,  33,  35,
   0,  24,  26,   0,  28,  23,   0, 184,   0,  34,
  30,   0,  29,  95,  96,  25,  27,   0,   0,   0,
   0,   0,  31,  32,  37,  38, 224,   0,   0,   0,
   0,   0,   0,  33,  35,   0,  24,  26,   0,  28,
  23,   0,  55, 214,  34,  30, 213,  29,  98,   0,
   0,   0,   0,   0,   0,   0,   0,  31,  32,  37,
  38,   0,   0, 157,   0, 110, 111, 112, 109,   0,
 159, 160, 161, 140, 141,   0,   0,  95,  96,  25,
  27,  99,   0, 100, 101,   0,  90,  91,  92,  93,
  55,   0,   0,  30,   0,  29,  98,  33,  35,   0,
  24,  26,  55,  28,  23,  30,   0,  29,  34,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  31,  32,  37,  38,  95,  96,  25,  27,  99,
   0, 100, 101,   0,  90,  91,  92,  93,   0,  55,
   0,   0,  30,   0,  29,  33,  35,   0,  24,  26,
   0,  28,  23,   0,   0, 170,  34, 195,  43,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  31,
  32,  37,  38,   0,   0,  25,  27,  98,  55, 121,
   0,  30, 131,  29,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  33,  35,   0,  24,  26,   0,  28,
  23,  53,  51,   0,  34,   0,  95,  96,  25,  27,
   0,   0,   0,   0,   0,   0,   0,  31,  32,  37,
  38, 238,   0, 240,   0, 241,  33, 188,   0,  24,
  26,  19,  28,  23,  30,   0,  29,  34,  17,   0,
  55, 232,   0,  30,   0,  29,   0,   0,   0,   0,
  31,  32,  37,  38,  25,  27,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 185,  33,  35,   0,  24,  26,   0,  28,  23,
   0,   0,   0,  34,   0,   0,   0,   0, 184,  25,
  27,  30,   0,  29,   0,   0,  31,  32,  37,  38,
  55, 231,   0,  30,   0,  29,   0,  33,  35,   0,
  24,  26,   0,  28,  23,   0,   0,   0,  34,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  31,  32,  37,  38,   0,   0,  25,  27,  55,
 217,   0,  30,   0,  29,   0,   0,   0,  55,  25,
  27,  30, 169,  29,   0,  33,  35,   0,  24,  26,
   0,  28,  23,  53,  51,   0,  34,  33,  35,   0,
  24,  26,   0,  28,  23,   0,   0,   0,  34,  31,
  32,  37,  38,  42,   0,  55,  25,  27,  30, 168,
  29,  31,  32,  37,  38,   0,   0,  55,   0,   0,
  30, 167,  29,   0,  33,  35,   0,  24,  26,   0,
  28,  23,   0,   0,   0,  34,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  25,  27,   0,  31,  32,
  37,  38,  55, 163,   0,  30,   0,  29,   0,   0,
   0,   0,   0,  33,  35,   0,  24,  26,   0,  28,
  23,   0,   0,   0,  34,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  31,  32,  37,
  38,  55, 121,   0,  30,   0,  29,   0,  25,  27,
  55,   0,   0,  30, 131,  29,   0,  25,  27,   0,
   0,   0,   0,   0,   0,  20,  33,  35,   0,  24,
  26,   0,  28,  23,   0,  33,  35,  34,  24,  26,
   0,  28,  23,   0,   0,   0,  34,   0,   0,   0,
  31,  32,  37,  38,   0,   0,   0,   0,   0,  31,
  32,  37,  38,   0,   0,  25,  27,  55, 129,   0,
  30,   0,  29,   0,   0,   0,  55,  25,  27,  30,
   0,  29, 185,  33,  35,   0,  24,  26,   0,  28,
  23,   0,   0, 114,  34,  33,  35, 115,  24,  26,
   0,  28,  23,   0,   0,   0,  34,  31,  32,  37,
  38, 179,   0,   0,   0,   0,  25,  27,   0,  31,
  32,  37,  38,   0,  74,  25,  27,  30,   0,  29,
   0,   0,   0,   0,  33,  35,   0,  24,  26,   0,
  28,  23,   0,  33,  35,  34,  24,  26,   0,  28,
  23, 113,   0, 114,  34,   0,   0, 115,  31,  32,
  37,  38,  25,  27, 156,   0,   0,  31,  32,  37,
  38,   0,   0,   0,  25,  27,   0,   0,   0,   0,
  33,  35,   0,  24,  26,   0,  28,  23,   0,   0,
   0,  34,  33,  35,   0,  24,  26,   0,  28,  23,
   0,   0,   0,  34,  31,  32,  37,  38,   0,  25,
  27, 113,   0,   0,   0,   0,  31,  32,  37,  38,
   0,   0,   0,   0,   0,   0,   0,  33,  35,   0,
  24,  26,   0,  28,  23,   0,   0,   0,  34,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  25,  27,
   0,  31,  32,  37,  38,   0,   0,  25,  27,   0,
   0,   0,   0, 186,   0,   0,  33,  35,   0,  24,
  26,   0,  28,  23,   0,  33,  35,  34,  24,  26,
   0,  28,  23,   0,   0,   0,  34,   0,   0,   0,
  31,  32,  37,  38,   0,   0, 207, 208,   0,  31,
  32,  37,  38,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 218, 219,  25,  27,   0,   0,   0,   0,
   0,   0, 223,  25,  27,   0,   0,   0,   0,   0,
   0,   0,  33,  35,   0,  24,  26,   0,  28,  23,
   0,  33,  35,  34,  24,  26,   0,  28,  23,   0,
   0,   0,  34,   0,   0,   0,  31,  32,  37,  38,
   0,   0,   0,   0,   0,  31,  32,  37,  38,   0,
   0,  25,  27,   0, 110, 111, 112, 109,   0, 159,
 160, 161,   0,   0,   0,   0,   0,  22,   0,  33,
  35,   0,  24,  26,   0,  28,  23,   0,   0,   0,
  34,   0,   0,   0,   0,   0,  54,   0,   0,   0,
   0,   0,   0,  31,  32,  37,  38,  54,  54,  80,
  81,   0,   0,   0,   0,   0,  54,   0,   0,   0,
   0,   0, 157,   0, 110, 111, 112, 109,   0, 159,
 160, 161,   0,   0,   0,   0,  54,   0,   0,   0,
  54,  54,  54,  54,  54,   0,   0,   0,   0,   0,
  54,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  21,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  54,   0,   0,   0,   0,
  52,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  78,  79,   0,   0,  54,  54,   0,   0,   0,
  83,   0,   0,   0,   0,   0,  54,   0,  54,   0,
   0,  54,   0,  54,  54,  54,  54,   0,   0,   0,
  52,   0,  54,   0, 123, 124, 125, 126, 127,   0,
   0,   0,   0,   0,  52,   0,   0,   0,   0,   0,
  97,   0,  54,  54,   0,   0,  18,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  54,  52,
  58,  18,   0,   0,   0,   0,  72,   0,   0,   0,
  54,   0,  54,   0,   0,   0,  54,  54,   0,  52,
  52,   0,   0,  54,   0,   0,  18,  18,   0,  18,
  52,   0,  52,   0, 117,  52, 118,  52,  52,  52,
  52,   0,   0,   0,  54,  54,  52,   0,   0,   0,
 128, 130,   0,   0,   0, 133, 135, 136, 137,   0,
   0,   0,   0, 138,   0,   0,  52,  52,   0,   0,
   0,   0, 144,   0,   0,   0,  72,  72,   0,   0,
   0,   0,  52,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  52,   0,  52,   0,   0,   0,
  52,  52,   0,   0,   0,   0,   0,  52,   0,   0,
   0,   0, 164, 165,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  52,  52,
   0, 180, 180,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 192,   0,
 194,   0,   0,   0,   0,   0, 198,   0,   0, 199,
   0,   0,   0,   0,   0, 205, 180,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 180, 180,   0,   0,   0,   0,   0,   0,   0,
   0, 180,   0,   0, 226,   0, 227 };
short yypact[]={

-220,-1000,-249,-1000, -80, 183,-1000,-1000,-250, -22,
 -82, -21,-1000,-1000,-1000,-1000,-1000,-1000, 540, 681,
 681,  22,-297,-1000,  73,1044,  72,  70,  69, 996,
 996,-304,-304,-1000,-1000, -41,-1000,-1000, 996, 282,
-1000,-1000,-1000,-1000,-1000, 681, 681,-1000, 681, 152,
 -10,  11,  22, 996,-323, 996,  57,  47, 378,  41,
-282,-1000, 996, 996, 996, 996, 996,-1000,-1000, 996,
 987,-1000, 930,  24,1044, 996, 996, 996,-1000,-1000,
-1000,-1000, 996,-1000,-1000,-1000, -22, 330, 330,-1000,
 -22, 589, -22, -22,-1000,1044,1044, 996,-1000,  67,
  65,  35, 104,-279,-1000,  56,-104,-1000,1077,-1000,
-1000,-1000,-1000,-1000,-1000, -10,-1000,  -1, 921,-1000,
-1000,-1000,-1000,  34,  34,-1000,-1000,-1000, 996,-1000,
 882, 996, 996, 628,  53, 847, 835, 798, 552,-1000,
-265,-1000,-1000,-1000, 589,-1000,-1000,   8,-113,-113,
 738, 738, 411,-1000,-1000,-1000,-1000, -10,-314,-1000,
-1000,-1000, 238,-1000, 996, 996,-1000, 996,-292, 996,
-1000, 330,-253,-1000,-1000, 996,-1000,-1000, 996,  31,
 540,-1000,-1000,-1000, 738, 738, -19,  -4, -62, -40,
1017,-1000, 492,  51, 789,-1000,-1000,-1000, 996, 996,
 738, 738,-253,  52,  47, 378,  41,-285,-1000,-253,
 457,-293,-1000, 996,-1000, 996,-1000,-1000,-288,-1000,
-1000,-1000,-1000, -43, 218,  39, 750, 690, 218,  33,
-253,-1000,-1000,  32,-253, 330,-253, 330,-1000, 330,
-1000,-1000 };
short yypgo[]={

   0, 130, 129, 128, 126,  66,  47,1071, 115,  70,
1510,  57,  40, 125, 146, 124,1431, 123,  56,1347,
  54, 122,  45,  52,  38, 121,  49, 201, 475, 120,
 118 };
short yyr1[]={

   0,   1,   1,   2,   2,   2,   4,   4,   4,   6,
   6,   6,   6,   8,   8,   8,   8,   7,   7,   7,
   7,  13,  15,  15,  17,  12,  12,  19,  19,  19,
  19,  19,  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  10,  10,  10,  14,  14,
  21,  21,  21,  21,  21,   3,   3,   3,   9,   9,
   9,   9,  20,  20,  20,  23,  23,  23,  24,  24,
  25,  18,  26,  26,  26,  26,  26,  26,  26,  26,
  26,  26,  26,  26,  11,  11,  22,  22,  27,  27,
  27,  27,  27,  27,  27,  28,  28,  28,  28,  28,
  28,  28,  28,  28,  28,  28,   5,   5,  29,  30,
  30,  30 };
short yyr2[]={

   0,   3,   1,   4,   2,   0,   4,   2,   0,   3,
   3,   2,   3,   3,   3,   2,   3,   1,   1,   1,
   1,   2,   1,   2,   5,   3,   3,   1,   1,   1,
   4,   1,   1,   1,   1,   3,   4,   2,   8,   6,
   8,   6,   6,   3,   3,   3,   3,   3,   3,   2,
   2,   2,   2,   2,   2,   1,   2,   3,   1,   0,
   1,   4,   3,   6,   3,   3,   0,   2,   1,   1,
   1,   1,   1,   1,   0,   3,   3,   3,   1,   1,
   0,   4,   1,   1,   1,   1,   1,   1,   3,   2,
   2,   2,   2,   3,   3,   3,   1,   1,   4,   2,
   4,   2,   1,   0,   1,   2,   2,   4,   2,   1,
   2,   2,   3,   2,   2,   3,   2,   0,   5,  10,
   9,   8 };
short yychk[]={

-1000,  -1,  -2, 256, 282,  -3, 284, 123,  -4, -21,
 283,  -9, 123, -18, -11, -12,  -8,  47, -10,  40,
 304, -16, -19, 312, 308, 287, 309, 288, 311,  45,
  43, 329, 330, 305, 316, 306, -15, 331, 332,  -5,
 284, -22, 284,  59, 123, 302, 303, 123,  44,  -5,
 -25, 314, -16, 313, -19,  40,  -8, -12, -10, -11,
  -9,  -9,  43,  45,  42,  47,  37, 329, 330, 301,
  40, -20, -10, -23,  40,  40,  40,  40, -16, -16,
 -19, -19,  91, -16, 125, -28, -27, -17, -29, -30,
 294, 295, 296, 297, 123, 285, 286, -10, 256, 289,
 291, 292,  -5,  -9,  -9,  -5,  -9, 125, -26, 320,
 317, 318, 319,  94,  36,  40, -18, -10, -10,  41,
  41,  41,  41, -16, -16, -16, -16, -16, -10,  41,
 -10,  44,  44, -10, -23, -10, -10, -10, -10, -22,
 -28, -28, -22, -22, -10, -22, -22,  -5, -20, -20,
  40,  40,  40, 125, 125, 123,  47, 315, -26, 322,
 323, 324, -26,  41, -10, -10,  41,  44,  44,  44,
  93, -13, 290, -22, 125, -24, 313, 124, -24,  -7,
 -10, -11, -12,  -6,  40, 304,  -7, -27, 306,  -5,
 -26,  41, -10, 306, -10, -28, -14, 284, -10, -10,
 302, 303,  41,  -6, -12, -10, -11,  -7,  -7,  41,
  59, 293, 125,  44,  41,  44,  41,  41,  -7,  -7,
 -14,  41, -14,  -7,  59, 306, -10, -10,  59, -27,
  41,  41,  41, -27,  41, -14,  41, -14, -28, -14,
 -28, -28 };
short yydef[]={

   5,  -2,  66,   2,   0,   8,   4, 117,   1,  67,
   0,  60, 117,  68,  69,  70,  71,  80,   0,   0,
   0,  55,  32,  33,  34,  74,   0,   0,   0,   0,
   0,   0,   0,  27,  28,  29,  31,  22,   0, 103,
   7,  65,  96,  97, 117,   0,   0, 117,   0, 103,
   0,   0,  56,   0,  32,   0,  71,  70,   0,  69,
   0,  15,   0,   0,   0,   0,   0,  53,  54,   0,
   0,  37,  72,  73,   0,   0,   0,   0,  49,  50,
  51,  52,   0,  23,   3, 116,   0, 103, 103, 109,
   0,   0,   0,   0, 117,  74,  74, 102, 104,   0,
   0,   0, 103,  13,  14, 103,  62,  64,   0,  82,
  83,  84,  85,  86,  87,   0,  25,  94,   0,  16,
  26,  43,  95,  44,  45,  46,  47,  48,  57,  35,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 105,
 106, 108, 110, 111,   0, 113, 114, 103,  99, 101,
   0,   0, 103,   6,  61, 117,  81,   0,  89,  90,
  91,  92,   0,  36,  75,  76,  77,   0,   0,   0,
  30, 103,  59, 112, 115,   0,  78,  79,   0,   0,
  17,  18,  19,  20,   0,   0,   0,   0,  29, 103,
  88,  93,   0,   0,   0, 107,  21,  58,  98, 100,
   0,   0,  59,  20,  19,  17,  18,   0,  11,  59,
   0,   0,  63,   0,  39,   0,  41,  42,   9,  10,
  24,  12, 118,   0, 103,   0,   0,   0, 103,   0,
  59,  38,  40,   0,  59, 103,  59, 103, 121, 103,
 120, 119 };
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
int yydebug = 1; /* 1 for debugging */
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
			
case 1:
# line 44 "awk.g.y"
{ if (errorflag==0) winner = (node *)stat3(PROGRAM, yypvt[-2], yypvt[-1], yypvt[-0]); } break;
case 2:
# line 45 "awk.g.y"
{ yyclearin; yyerror("bailing out"); } break;
case 3:
# line 49 "awk.g.y"
{ PUTS("XBEGIN list"); yyval = yypvt[-1]; } break;
case 5:
# line 51 "awk.g.y"
{ PUTS("empty XBEGIN"); yyval = (hack)nullstat; } break;
case 6:
# line 55 "awk.g.y"
{ PUTS("XEND list"); yyval = yypvt[-1]; } break;
case 8:
# line 57 "awk.g.y"
{ PUTS("empty END"); yyval = (hack)nullstat; } break;
case 9:
# line 61 "awk.g.y"
{ PUTS("cond||cond"); yyval = op2(BOR, yypvt[-2], yypvt[-0]); } break;
case 10:
# line 62 "awk.g.y"
{ PUTS("cond&&cond"); yyval = op2(AND, yypvt[-2], yypvt[-0]); } break;
case 11:
# line 63 "awk.g.y"
{ PUTS("!cond"); yyval = op1(NOT, yypvt[-0]); } break;
case 12:
# line 64 "awk.g.y"
{ yyval = yypvt[-1]; } break;
case 13:
# line 68 "awk.g.y"
{ PUTS("pat||pat"); yyval = op2(BOR, yypvt[-2], yypvt[-0]); } break;
case 14:
# line 69 "awk.g.y"
{ PUTS("pat&&pat"); yyval = op2(AND, yypvt[-2], yypvt[-0]); } break;
case 15:
# line 70 "awk.g.y"
{ PUTS("!pat"); yyval = op1(NOT, yypvt[-0]); } break;
case 16:
# line 71 "awk.g.y"
{ yyval = yypvt[-1]; } break;
case 17:
# line 75 "awk.g.y"
{ PUTS("expr"); yyval = op2(NE, yypvt[-0], valtonode(lookup("$zero&null", symtab, 0), CCON)); } break;
case 18:
# line 76 "awk.g.y"
{ PUTS("relexpr"); } break;
case 19:
# line 77 "awk.g.y"
{ PUTS("lexexpr"); } break;
case 20:
# line 78 "awk.g.y"
{ PUTS("compcond"); } break;
case 21:
# line 82 "awk.g.y"
{ PUTS("else"); } break;
case 22:
# line 86 "awk.g.y"
{ PUTS("field"); yyval = valtonode(yypvt[-0], CFLD); } break;
case 23:
# line 87 "awk.g.y"
{ PUTS("ind field"); yyval = op1(INDIRECT, yypvt[-0]); } break;
case 24:
# line 91 "awk.g.y"
{ PUTS("if(cond)"); yyval = yypvt[-2]; } break;
case 25:
# line 95 "awk.g.y"
{ PUTS("expr~re"); yyval = op2(yypvt[-1], yypvt[-2], makedfa(yypvt[-0])); } break;
case 26:
# line 96 "awk.g.y"
{ PUTS("(lex_expr)"); yyval = yypvt[-1]; } break;
case 27:
# line 100 "awk.g.y"
{PUTS("number"); yyval = valtonode(yypvt[-0], CCON); } break;
case 28:
# line 101 "awk.g.y"
{ PUTS("string"); yyval = valtonode(yypvt[-0], CCON); } break;
case 29:
# line 102 "awk.g.y"
{ PUTS("var"); yyval = valtonode(yypvt[-0], CVAR); } break;
case 30:
# line 103 "awk.g.y"
{ PUTS("array[]"); yyval = op2(ARRAY, yypvt[-3], yypvt[-1]); } break;
case 33:
# line 108 "awk.g.y"
{ PUTS("getline"); yyval = op1(GETLINE, 0); } break;
case 34:
# line 109 "awk.g.y"
{ PUTS("func");
			yyval = op2(FNCN, yypvt[-0], valtonode(lookup("$record", symtab, 0), CFLD));
			} break;
case 35:
# line 112 "awk.g.y"
{ PUTS("func()"); 
			yyval = op2(FNCN, yypvt[-2], valtonode(lookup("$record", symtab, 0), CFLD));
			} break;
case 36:
# line 115 "awk.g.y"
{ PUTS("func(expr)"); yyval = op2(FNCN, yypvt[-3], yypvt[-1]); } break;
case 37:
# line 116 "awk.g.y"
{ PUTS("sprintf"); yyval = op1(yypvt[-1], yypvt[-0]); } break;
case 38:
# line 118 "awk.g.y"
{ PUTS("substr(e,e,e)"); yyval = op3(SUBSTR, yypvt[-5], yypvt[-3], yypvt[-1]); } break;
case 39:
# line 120 "awk.g.y"
{ PUTS("substr(e,e,e)"); yyval = op3(SUBSTR, yypvt[-3], yypvt[-1], nullstat); } break;
case 40:
# line 122 "awk.g.y"
{ PUTS("split(e,e,e)"); yyval = op3(SPLIT, yypvt[-5], yypvt[-3], yypvt[-1]); } break;
case 41:
# line 124 "awk.g.y"
{ PUTS("split(e,e,e)"); yyval = op3(SPLIT, yypvt[-3], yypvt[-1], nullstat); } break;
case 42:
# line 126 "awk.g.y"
{ PUTS("index(e,e)"); yyval = op2(INDEX, yypvt[-3], yypvt[-1]); } break;
case 43:
# line 127 "awk.g.y"
{PUTS("(expr)");  yyval = yypvt[-1]; } break;
case 44:
# line 128 "awk.g.y"
{ PUTS("t+t"); yyval = op2(ADD, yypvt[-2], yypvt[-0]); } break;
case 45:
# line 129 "awk.g.y"
{ PUTS("t-t"); yyval = op2(MINUS, yypvt[-2], yypvt[-0]); } break;
case 46:
# line 130 "awk.g.y"
{ PUTS("t*t"); yyval = op2(MULT, yypvt[-2], yypvt[-0]); } break;
case 47:
# line 131 "awk.g.y"
{ PUTS("t/t"); yyval = op2(DIVIDE, yypvt[-2], yypvt[-0]); } break;
case 48:
# line 132 "awk.g.y"
{ PUTS("t%t"); yyval = op2(MOD, yypvt[-2], yypvt[-0]); } break;
case 49:
# line 133 "awk.g.y"
{ PUTS("-term"); yyval = op1(UMINUS, yypvt[-0]); } break;
case 50:
# line 134 "awk.g.y"
{ PUTS("+term"); yyval = yypvt[-0]; } break;
case 51:
# line 135 "awk.g.y"
{ PUTS("++var"); yyval = op1(PREINCR, yypvt[-0]); } break;
case 52:
# line 136 "awk.g.y"
{ PUTS("--var"); yyval = op1(PREDECR, yypvt[-0]); } break;
case 53:
# line 137 "awk.g.y"
{ PUTS("var++"); yyval= op1(POSTINCR, yypvt[-1]); } break;
case 54:
# line 138 "awk.g.y"
{ PUTS("var--"); yyval= op1(POSTDECR, yypvt[-1]); } break;
case 55:
# line 142 "awk.g.y"
{ PUTS("term"); } break;
case 56:
# line 143 "awk.g.y"
{ PUTS("expr term"); yyval = op2(CAT, yypvt[-1], yypvt[-0]); } break;
case 57:
# line 144 "awk.g.y"
{ PUTS("var=expr"); yyval = stat2(yypvt[-1], yypvt[-2], yypvt[-0]); } break;
case 60:
# line 153 "awk.g.y"
{ PUTS("pattern"); yyval = stat2(PASTAT, yypvt[-0], genprint()); } break;
case 61:
# line 154 "awk.g.y"
{ PUTS("pattern {...}"); yyval = stat2(PASTAT, yypvt[-3], yypvt[-1]); } break;
case 62:
# line 155 "awk.g.y"
{ PUTS("srch,srch"); yyval = pa2stat(yypvt[-2], yypvt[-0], genprint()); } break;
case 63:
# line 157 "awk.g.y"
{ PUTS("srch, srch {...}"); yyval = pa2stat(yypvt[-5], yypvt[-3], yypvt[-1]); } break;
case 64:
# line 158 "awk.g.y"
{ PUTS("null pattern {...}"); yyval = stat2(PASTAT, nullstat, yypvt[-1]); } break;
case 65:
# line 162 "awk.g.y"
{ PUTS("pa_stats pa_stat"); yyval = linkum(yypvt[-2], yypvt[-1]); } break;
case 66:
# line 163 "awk.g.y"
{ PUTS("null pa_stat"); yyval = (hack)nullstat; } break;
case 67:
# line 164 "awk.g.y"
{PUTS("pa_stats pa_stat"); yyval = linkum(yypvt[-1], yypvt[-0]); } break;
case 68:
# line 168 "awk.g.y"
{ PUTS("regex");
		yyval = op2(MATCH, valtonode(lookup("$record", symtab, 0), CFLD), makedfa(yypvt[-0]));
		} break;
case 69:
# line 171 "awk.g.y"
{ PUTS("relexpr"); } break;
case 70:
# line 172 "awk.g.y"
{ PUTS("lexexpr"); } break;
case 71:
# line 173 "awk.g.y"
{ PUTS("comp pat"); } break;
case 72:
# line 177 "awk.g.y"
{ PUTS("expr"); } break;
case 73:
# line 178 "awk.g.y"
{ PUTS("pe_list"); } break;
case 74:
# line 179 "awk.g.y"
{ PUTS("null print_list"); yyval = valtonode(lookup("$record", symtab, 0), CFLD); } break;
case 75:
# line 183 "awk.g.y"
{yyval = linkum(yypvt[-2], yypvt[-0]); } break;
case 76:
# line 184 "awk.g.y"
{yyval = linkum(yypvt[-2], yypvt[-0]); } break;
case 77:
# line 185 "awk.g.y"
{yyval = yypvt[-1]; } break;
case 80:
# line 194 "awk.g.y"
{ startreg(); } break;
case 81:
# line 196 "awk.g.y"
{ PUTS("/r/"); yyval = yypvt[-1]; } break;
case 82:
# line 200 "awk.g.y"
{ PUTS("regex CHAR"); yyval = op2(CHAR, (node *) 0, yypvt[-0]); } break;
case 83:
# line 201 "awk.g.y"
{ PUTS("regex DOT"); yyval = op2(DOT, (node *) 0, (node *) 0); } break;
case 84:
# line 202 "awk.g.y"
{ PUTS("regex CCL"); yyval = op2(CCL, (node *) 0, cclenter(yypvt[-0])); } break;
case 85:
# line 203 "awk.g.y"
{ PUTS("regex NCCL"); yyval = op2(NCCL, (node *) 0, cclenter(yypvt[-0])); } break;
case 86:
# line 204 "awk.g.y"
{ PUTS("regex ^"); yyval = op2(CHAR, (node *) 0, HAT); } break;
case 87:
# line 205 "awk.g.y"
{ PUTS("regex $"); yyval = op2(CHAR, (node *) 0 ,(node *) 0); } break;
case 88:
# line 206 "awk.g.y"
{ PUTS("regex OR"); yyval = op2(OR, yypvt[-2], yypvt[-0]); } break;
case 89:
# line 208 "awk.g.y"
{ PUTS("regex CAT"); yyval = op2(CAT, yypvt[-1], yypvt[-0]); } break;
case 90:
# line 209 "awk.g.y"
{ PUTS("regex STAR"); yyval = op2(STAR, yypvt[-1], (node *) 0); } break;
case 91:
# line 210 "awk.g.y"
{ PUTS("regex PLUS"); yyval = op2(PLUS, yypvt[-1], (node *) 0); } break;
case 92:
# line 211 "awk.g.y"
{ PUTS("regex QUEST"); yyval = op2(QUEST, yypvt[-1], (node *) 0); } break;
case 93:
# line 212 "awk.g.y"
{ PUTS("(regex)"); yyval = yypvt[-1]; } break;
case 94:
# line 217 "awk.g.y"
{ PUTS("expr relop expr"); yyval = op2(yypvt[-1], yypvt[-2], yypvt[-0]); } break;
case 95:
# line 219 "awk.g.y"
{ PUTS("(relexpr)"); yyval = yypvt[-1]; } break;
case 98:
# line 229 "awk.g.y"
{ PUTS("print>stat"); yyval = stat3(yypvt[-3], yypvt[-2], yypvt[-1], yypvt[-0]); } break;
case 99:
# line 231 "awk.g.y"
{ PUTS("print list"); yyval = stat3(yypvt[-1], yypvt[-0], nullstat, nullstat); } break;
case 100:
# line 233 "awk.g.y"
{ PUTS("printf>stat"); yyval = stat3(yypvt[-3], yypvt[-2], yypvt[-1], yypvt[-0]); } break;
case 101:
# line 235 "awk.g.y"
{ PUTS("printf list"); yyval = stat3(yypvt[-1], yypvt[-0], nullstat, nullstat); } break;
case 102:
# line 236 "awk.g.y"
{ PUTS("expr"); yyval = exptostat(yypvt[-0]); } break;
case 103:
# line 237 "awk.g.y"
{ PUTS("null simple statement"); yyval = (hack)nullstat; } break;
case 104:
# line 238 "awk.g.y"
{ yyclearin; yyerror("illegal statement"); } break;
case 105:
# line 242 "awk.g.y"
{ PUTS("simple stat"); } break;
case 106:
# line 243 "awk.g.y"
{ PUTS("if stat"); yyval = stat3(IF, yypvt[-1], yypvt[-0], nullstat); } break;
case 107:
# line 245 "awk.g.y"
{ PUTS("if-else stat"); yyval = stat3(IF, yypvt[-3], yypvt[-2], yypvt[-0]); } break;
case 108:
# line 246 "awk.g.y"
{ PUTS("while stat"); yyval = stat2(WHILE, yypvt[-1], yypvt[-0]); } break;
case 109:
# line 247 "awk.g.y"
{ PUTS("for stat"); } break;
case 110:
# line 248 "awk.g.y"
{ PUTS("next"); yyval = stat1(NEXT, 0); } break;
case 111:
# line 249 "awk.g.y"
{ PUTS("exit"); yyval = stat1(EXIT, 0); } break;
case 112:
# line 250 "awk.g.y"
{ PUTS("exit"); yyval = stat1(EXIT, yypvt[-1]); } break;
case 113:
# line 251 "awk.g.y"
{ PUTS("break"); yyval = stat1(BREAK, 0); } break;
case 114:
# line 252 "awk.g.y"
{ PUTS("continue"); yyval = stat1(CONTINUE, 0); } break;
case 115:
# line 253 "awk.g.y"
{ PUTS("{statlist}"); yyval = yypvt[-1]; } break;
case 116:
# line 257 "awk.g.y"
{ PUTS("stat_list stat"); yyval = linkum(yypvt[-1], yypvt[-0]); } break;
case 117:
# line 258 "awk.g.y"
{ PUTS("null stat list"); yyval = (hack)nullstat; } break;
case 118:
# line 262 "awk.g.y"
{ PUTS("while(cond)"); yyval = yypvt[-2]; } break;
case 119:
# line 267 "awk.g.y"
{ PUTS("for(e;e;e)"); yyval = stat4(FOR, yypvt[-7], yypvt[-5], yypvt[-3], yypvt[-0]); } break;
case 120:
# line 269 "awk.g.y"
{ PUTS("for(e;e;e)"); yyval = stat4(FOR, yypvt[-6], nullstat, yypvt[-3], yypvt[-0]); } break;
case 121:
# line 271 "awk.g.y"
{ PUTS("for(v in v)"); yyval = stat3(IN, yypvt[-5], yypvt[-3], yypvt[-0]); } break;
		}
		goto yystack;  /* stack new state and value */

	}
