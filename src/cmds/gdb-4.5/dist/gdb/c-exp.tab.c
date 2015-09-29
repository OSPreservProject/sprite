
/*  A Bison parser, made from ./c-exp.y  */

#define	INT	258
#define	CHAR	259
#define	UINT	260
#define	FLOAT	261
#define	STRING	262
#define	NAME	263
#define	TYPENAME	264
#define	NAME_OR_INT	265
#define	NAME_OR_UINT	266
#define	STRUCT	267
#define	UNION	268
#define	ENUM	269
#define	SIZEOF	270
#define	UNSIGNED	271
#define	COLONCOLON	272
#define	TEMPLATE	273
#define	ERROR	274
#define	SIGNED_KEYWORD	275
#define	LONG	276
#define	SHORT	277
#define	INT_KEYWORD	278
#define	LAST	279
#define	REGNAME	280
#define	VARIABLE	281
#define	ASSIGN_MODIFY	282
#define	THIS	283
#define	ABOVE_COMMA	284
#define	OROR	285
#define	ANDAND	286
#define	EQUAL	287
#define	NOTEQUAL	288
#define	LEQ	289
#define	GEQ	290
#define	LSH	291
#define	RSH	292
#define	UNARY	293
#define	INCREMENT	294
#define	DECREMENT	295
#define	ARROW	296
#define	BLOCKNAME	297

#line 29 "./c-exp.y"


#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "expression.h"
#include "parser-defs.h"
#include "value.h"
#include "language.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"

/* Ensure that if the generated parser contains any calls to malloc/realloc,
   that they get mapped to xmalloc/xrealloc. */

#define malloc	xmalloc
#define realloc	xrealloc

/* These MUST be included in any grammar file!!!! 
   Please choose unique names! */
#define	yymaxdepth c_maxdepth
#define	yyparse	c_parse
#define	yylex	c_lex
#define	yyerror	c_error
#define	yylval	c_lval
#define	yychar	c_char
#define	yydebug	c_debug
#define	yypact	c_pact	
#define	yyr1	c_r1			
#define	yyr2	c_r2			
#define	yydef	c_def		
#define	yychk	c_chk		
#define	yypgo	c_pgo		
#define	yyact	c_act		
#define	yyexca	c_exca
#define yyerrflag c_errflag
#define yynerrs	c_nerrs
#define	yyps	c_ps
#define	yypv	c_pv
#define	yys	c_s
#define	yy_yys	c_yys
#define	yystate	c_state
#define	yytmp	c_tmp
#define	yyv	c_v
#define	yy_yyv	c_yyv
#define	yyval	c_val
#define	yylloc	c_lloc

int
yyparse PARAMS ((void));

int
yylex PARAMS ((void));

void
yyerror PARAMS ((char *));

/* #define	YYDEBUG	1 */


#line 98 "./c-exp.y"
typedef union
  {
    LONGEST lval;
    unsigned LONGEST ulval;
    double dval;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  } YYSTYPE;
#line 117 "./c-exp.y"

/* YYSTYPE gets defined by %union */
static int
parse_number PARAMS ((char *, int, int, YYSTYPE *));

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __STDC__
#define const
#endif



#define	YYFINAL		210
#define	YYFLAG		-32768
#define	YYNTBASE	67

#define YYTRANSLATE(x) ((unsigned)(x) <= 297 ? yytranslate[x] : 87)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    60,     2,     2,     2,    51,    37,     2,    58,
    63,    49,    47,    29,    48,    56,    50,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    66,     2,    40,
    31,    41,    32,    46,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    57,     2,    62,    36,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    64,    35,    65,    61,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    30,    33,    34,    38,    39,    42,    43,
    44,    45,    52,    53,    54,    55,    59
};

static const short yyrline[] = {     0,
   199,   200,   203,   210,   211,   216,   219,   222,   226,   230,
   234,   238,   242,   246,   250,   254,   260,   267,   271,   277,
   285,   289,   293,   297,   303,   306,   310,   314,   320,   326,
   332,   336,   340,   344,   348,   352,   356,   360,   364,   368,
   372,   376,   380,   384,   388,   392,   396,   400,   404,   408,
   412,   416,   422,   432,   445,   457,   470,   477,   484,   487,
   493,   499,   505,   512,   519,   526,   544,   554,   567,   580,
   605,   606,   648,   735,   736,   771,   773,   775,   777,   779,
   782,   784,   789,   795,   797,   801,   803,   807,   809,   813,
   814,   816,   818,   821,   828,   830,   832,   834,   836,   838,
   840,   842,   844,   846,   848,   850,   852,   855,   858,   861,
   863,   865,   867,   869,   875,   876,   882,   888,   897,   902,
   909,   910,   911,   912,   913,   916,   917
};

static const char * const yytname[] = {     0,
"error","$illegal.","INT","CHAR","UINT","FLOAT","STRING","NAME","TYPENAME","NAME_OR_INT",
"NAME_OR_UINT","STRUCT","UNION","ENUM","SIZEOF","UNSIGNED","COLONCOLON","TEMPLATE","ERROR","SIGNED_KEYWORD",
"LONG","SHORT","INT_KEYWORD","LAST","REGNAME","VARIABLE","ASSIGN_MODIFY","THIS","','","ABOVE_COMMA",
"'='","'?'","OROR","ANDAND","'|'","'^'","'&'","EQUAL","NOTEQUAL","'<'",
"'>'","LEQ","GEQ","LSH","RSH","'@'","'+'","'-'","'*'","'/'",
"'%'","UNARY","INCREMENT","DECREMENT","ARROW","'.'","'['","'('","BLOCKNAME","'!'",
"'~'","']'","')'","'{'","'}'","':'","start"
};

static const short yyr1[] = {     0,
    67,    67,    68,    69,    69,    70,    70,    70,    70,    70,
    70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    70,    70,    71,    70,    72,    72,    72,    70,    70,    70,
    70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    70,    70,    70,    70,    70,    73,    73,    74,    75,    75,
    74,    74,    74,    76,    76,    77,    77,    77,    77,    77,
    78,    78,    78,    78,    78,    79,    79,    80,    80,    81,
    81,    81,    81,    81,    82,    82,    82,    82,    82,    82,
    82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
    82,    82,    82,    82,    83,    83,    83,    83,    84,    84,
    85,    85,    85,    85,    85,    86,    86
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     3,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     3,     3,     4,     3,     3,
     4,     4,     0,     5,     0,     1,     3,     4,     4,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     5,
     3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     4,     1,     1,     1,     3,     3,     3,     4,
     1,     2,     1,     1,     2,     1,     2,     1,     2,     1,
     3,     2,     1,     2,     1,     2,     3,     2,     3,     1,
     3,     6,     8,     9,     1,     1,     1,     1,     2,     3,
     2,     3,     3,     4,     2,     3,     2,     2,     2,     2,
     1,     2,     1,     5,     1,     1,     1,     1,     1,     3,
     1,     1,     1,     1,     1,     1,     1
};

static const short yydefact[] = {     0,
    53,    57,    55,    58,    64,   126,    95,    54,    56,     0,
     0,     0,     0,   111,     0,     0,   113,    97,    98,    96,
    60,    61,    62,    65,     0,     0,     0,     0,     0,     0,
   127,     0,     0,     0,     2,     1,     4,     0,    59,    71,
    90,     3,    74,    73,   121,   123,   124,   125,   122,   107,
   108,   109,     0,    15,     0,   115,   117,   118,   116,   110,
    72,     0,   117,   118,   112,   101,    99,   105,     7,     8,
     6,    11,    12,     0,     0,     9,    10,     0,    74,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    13,    14,     0,     0,     0,    23,     0,     0,
     0,    78,    76,     0,     0,    75,    80,    83,    85,     0,
     0,   103,   100,   106,     0,   102,    30,     0,     0,     0,
     5,    52,    51,     0,    49,    48,    47,    46,    45,    39,
    40,    43,    44,    41,    42,    37,    38,    31,    35,    36,
    32,    33,    34,   123,     0,    17,    16,     0,    20,    19,
     0,    25,    68,     0,    91,     0,    69,    79,    77,     0,
    86,    88,     0,   119,     0,     0,    82,    84,    63,   104,
     0,    29,    28,     0,    18,    21,    22,    26,     0,     0,
    70,    87,    81,     0,    89,   114,    50,     0,    24,     0,
   120,    27,    92,     0,    93,     0,    94,     0,     0,     0
};

static const short yydefgoto[] = {   208,
    35,    74,    37,   162,   189,    38,    39,    40,    41,   116,
   117,   118,   119,   174,    55,    60,   175,   167,    44
};

static const short yypact[] = {   202,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   111,
   111,   111,   264,   158,   111,   111,   176,    25,    10,-32768,
-32768,-32768,-32768,-32768,   202,   202,   202,   202,   202,   202,
    20,   202,   202,   421,-32768,    -6,   454,    23,-32768,-32768,
-32768,    35,   184,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   202,   190,    51,-32768,    82,    41,-32768,-32768,
-32768,    60,-32768,-32768,-32768,    81,-32768,-32768,   190,   190,
   190,   190,   190,    -4,   -34,   190,   190,   -38,   371,   202,
   202,   202,   202,   202,   202,   202,   202,   202,   202,   202,
   202,   202,   202,   202,   202,   202,   202,   202,   202,   202,
   202,   202,-32768,-32768,   347,   389,   202,-32768,   111,   421,
    47,   135,   135,     4,   120,-32768,    54,-32768,-32768,   -19,
     2,    84,-32768,-32768,   421,-32768,-32768,   202,   202,    65,
   454,   454,   454,   418,   506,   530,   553,   575,   596,   615,
   615,   249,   249,   249,   249,   368,   368,   627,   637,   637,
   190,   190,   190,    92,   202,-32768,-32768,   202,-32768,-32768,
     3,   202,    99,   110,-32768,   111,-32768,-32768,-32768,    66,
-32768,-32768,    67,    35,    -3,    29,-32768,-32768,   326,-32768,
   -22,   190,   190,   202,   190,   190,-32768,   454,    -1,    86,
-32768,-32768,-32768,   421,-32768,-32768,   481,   202,-32768,    68,
    35,   454,    79,   140,-32768,    38,-32768,   139,   145,-32768
};

static const short yypgoto[] = {-32768,
-32768,     6,   -11,-32768,-32768,-32768,-32768,    18,-32768,   -18,
-32768,    31,    33,     1,     0,   129,   -49,    -7,-32768
};


#define	YYLAST		695


static const short yytable[] = {    43,
    42,    54,    50,    51,    52,    36,   170,    61,    62,    45,
    46,    47,    48,    69,    70,    71,    72,    73,   196,   110,
    76,    77,    80,   110,    80,   194,   129,   198,   128,    43,
    75,    80,    68,    79,    78,   110,   -66,     7,   110,   109,
    10,    11,    12,   179,    14,    66,    16,    67,    17,    18,
    19,    20,    43,   120,    45,    46,    47,    48,   127,   195,
    49,   199,   166,   124,   187,   171,   194,   121,   131,   132,
   133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
   143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
   153,   172,   110,   168,   169,   165,   173,   157,   160,   125,
   207,   163,   122,   126,   123,    49,   180,   166,   -95,   164,
   114,   176,   161,   165,    79,   -67,   182,   183,    45,    46,
    47,    48,   156,   159,    79,   181,   190,   192,     7,   193,
   203,    10,    11,    12,   200,    14,   204,    16,   209,    17,
    18,    19,    20,   185,   210,    65,   186,   177,     7,   178,
   188,    10,    11,    12,   206,    14,   112,    16,   191,    17,
    18,    19,    20,     0,     0,     0,    56,   182,   113,    49,
     0,   112,   197,     0,     0,    79,   114,   115,    57,    58,
    59,     0,   172,   113,    56,     0,   202,     0,     0,     0,
     0,   114,   115,    79,   201,     0,    63,    64,    59,     0,
   111,     0,   205,    79,     1,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
   112,    17,    18,    19,    20,    21,    22,    23,     0,    24,
     0,     0,   113,     0,     0,     0,     0,     0,    25,     0,
   114,   115,   103,   104,   105,   106,   107,   108,     0,    26,
    27,     0,     0,     0,    28,    29,     0,     0,     0,    30,
    31,    32,    33,     0,     0,    34,     1,     2,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,     0,    17,    18,    19,    20,    21,    22,    23,
     0,    24,    95,    96,    97,    98,    99,   100,   101,   102,
    25,   103,   104,   105,   106,   107,   108,     0,     0,     0,
     0,    26,    27,     0,     0,     0,    28,    29,     0,     0,
     0,    53,    31,    32,    33,     0,     0,    34,     1,     2,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,     0,    17,    18,    19,    20,    21,
    22,    23,     0,    24,    45,   154,    47,    48,    10,    11,
    12,     0,    14,     0,    16,     0,    17,    18,    19,    20,
     0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
     0,     0,     0,    30,    31,    32,    33,   130,     0,    34,
     0,     0,     0,     0,     0,   155,    45,   154,    47,    48,
    10,    11,    12,     0,    14,    49,    16,   112,    17,    18,
    19,    20,     0,    97,    98,    99,   100,   101,   102,   113,
   103,   104,   105,   106,   107,   108,     0,   114,   115,     7,
     0,     0,    10,    11,    12,     0,    14,   158,    16,     0,
    17,    18,    19,    20,    81,     0,     0,    49,    82,    83,
    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
    94,    95,    96,    97,    98,    99,   100,   101,   102,     0,
   103,   104,   105,   106,   107,   108,     0,     0,     0,     0,
    81,     0,     0,   184,    82,    83,    84,    85,    86,    87,
    88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
    98,    99,   100,   101,   102,     0,   103,   104,   105,   106,
   107,   108,    83,    84,    85,    86,    87,    88,    89,    90,
    91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
   101,   102,     0,   103,   104,   105,   106,   107,   108,    85,
    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
    96,    97,    98,    99,   100,   101,   102,     0,   103,   104,
   105,   106,   107,   108,    86,    87,    88,    89,    90,    91,
    92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
   102,     0,   103,   104,   105,   106,   107,   108,    87,    88,
    89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
    99,   100,   101,   102,     0,   103,   104,   105,   106,   107,
   108,    88,    89,    90,    91,    92,    93,    94,    95,    96,
    97,    98,    99,   100,   101,   102,     0,   103,   104,   105,
   106,   107,   108,    89,    90,    91,    92,    93,    94,    95,
    96,    97,    98,    99,   100,   101,   102,     0,   103,   104,
   105,   106,   107,   108,    91,    92,    93,    94,    95,    96,
    97,    98,    99,   100,   101,   102,     0,   103,   104,   105,
   106,   107,   108,    98,    99,   100,   101,   102,     0,   103,
   104,   105,   106,   107,   108,   100,   101,   102,     0,   103,
   104,   105,   106,   107,   108
};

static const short yycheck[] = {     0,
     0,    13,    10,    11,    12,     0,     3,    15,    16,     8,
     9,    10,    11,    25,    26,    27,    28,    29,    41,    58,
    32,    33,    29,    58,    29,    29,    65,    29,    63,    30,
    30,    29,    23,    34,    34,    58,    17,     9,    58,    17,
    12,    13,    14,    63,    16,    21,    18,    23,    20,    21,
    22,    23,    53,    53,     8,     9,    10,    11,    63,    63,
    59,    63,    61,    23,    62,    62,    29,    17,    80,    81,
    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
    92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
   102,    63,    58,   112,   113,    49,   115,   105,   106,    40,
    63,   109,    21,    23,    23,    59,    23,    61,    17,   110,
    57,    58,   107,    49,   115,    17,   128,   129,     8,     9,
    10,    11,   105,   106,   125,   125,    17,    62,     9,    63,
    63,    12,    13,    14,    49,    16,    58,    18,     0,    20,
    21,    22,    23,   155,     0,    17,   158,   117,     9,   117,
   162,    12,    13,    14,   204,    16,    37,    18,   166,    20,
    21,    22,    23,    -1,    -1,    -1,     9,   179,    49,    59,
    -1,    37,   184,    -1,    -1,   176,    57,    58,    21,    22,
    23,    -1,    63,    49,     9,    -1,   198,    -1,    -1,    -1,
    -1,    57,    58,   194,   194,    -1,    21,    22,    23,    -1,
    17,    -1,    63,   204,     3,     4,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
    37,    20,    21,    22,    23,    24,    25,    26,    -1,    28,
    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    37,    -1,
    57,    58,    53,    54,    55,    56,    57,    58,    -1,    48,
    49,    -1,    -1,    -1,    53,    54,    -1,    -1,    -1,    58,
    59,    60,    61,    -1,    -1,    64,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    -1,    20,    21,    22,    23,    24,    25,    26,
    -1,    28,    44,    45,    46,    47,    48,    49,    50,    51,
    37,    53,    54,    55,    56,    57,    58,    -1,    -1,    -1,
    -1,    48,    49,    -1,    -1,    -1,    53,    54,    -1,    -1,
    -1,    58,    59,    60,    61,    -1,    -1,    64,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,    17,    18,    -1,    20,    21,    22,    23,    24,
    25,    26,    -1,    28,     8,     9,    10,    11,    12,    13,
    14,    -1,    16,    -1,    18,    -1,    20,    21,    22,    23,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,
    -1,    -1,    -1,    58,    59,    60,    61,    17,    -1,    64,
    -1,    -1,    -1,    -1,    -1,    49,     8,     9,    10,    11,
    12,    13,    14,    -1,    16,    59,    18,    37,    20,    21,
    22,    23,    -1,    46,    47,    48,    49,    50,    51,    49,
    53,    54,    55,    56,    57,    58,    -1,    57,    58,     9,
    -1,    -1,    12,    13,    14,    -1,    16,    49,    18,    -1,
    20,    21,    22,    23,    27,    -1,    -1,    59,    31,    32,
    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    -1,
    53,    54,    55,    56,    57,    58,    -1,    -1,    -1,    -1,
    27,    -1,    -1,    66,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    -1,    53,    54,    55,    56,
    57,    58,    32,    33,    34,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
    50,    51,    -1,    53,    54,    55,    56,    57,    58,    34,
    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    -1,    53,    54,
    55,    56,    57,    58,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    -1,    53,    54,    55,    56,    57,    58,    36,    37,
    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
    48,    49,    50,    51,    -1,    53,    54,    55,    56,    57,
    58,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    -1,    53,    54,    55,
    56,    57,    58,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    -1,    53,    54,
    55,    56,    57,    58,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    -1,    53,    54,    55,
    56,    57,    58,    47,    48,    49,    50,    51,    -1,    53,
    54,    55,    56,    57,    58,    49,    50,    51,    -1,    53,
    54,    55,    56,    57,    58
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__)
#include <alloca.h>
#endif

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYFAIL		goto yyerrlab;
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYIMPURE
#define YYLEX		yylex()
#endif

#ifndef YYPURE
#define YYLEX		yylex(&yylval, &yylloc)
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYIMPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/

int yynerrs;			/*  number of parse errors so far       */
#endif  /* YYIMPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYMAXDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYMAXDEPTH
#define YYMAXDEPTH 200
#endif

/*  YYMAXLIMIT is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#ifndef YYMAXLIMIT
#define YYMAXLIMIT 10000
#endif


#line 90 "bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  YYLTYPE *yylsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYMAXDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYMAXDEPTH];	/*  the semantic value stack		*/
  YYLTYPE yylsa[YYMAXDEPTH];	/*  the location stack			*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */
  YYLTYPE *yyls = yylsa;

  int yymaxdepth = YYMAXDEPTH;

#ifndef YYPURE
  int yychar;
  YYSTYPE yylval;
  YYLTYPE yylloc;
  int yynerrs;
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
  yylsp = yyls;

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yymaxdepth - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      YYLTYPE *yyls1 = yyls;
      short *yyss1 = yyss;

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yymaxdepth);

      yyss = yyss1; yyvs = yyvs1; yyls = yyls1;
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yymaxdepth >= YYMAXLIMIT)
	yyerror("parser stack overflow");
      yymaxdepth *= 2;
      if (yymaxdepth > YYMAXLIMIT)
	yymaxdepth = YYMAXLIMIT;
      yyss = (short *) alloca (yymaxdepth * sizeof (*yyssp));
      bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yymaxdepth * sizeof (*yyvsp));
      bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yymaxdepth * sizeof (*yylsp));
      bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yymaxdepth);
#endif

      if (yyssp >= yyss + yymaxdepth - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
yyresume:

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Next token is %d (%s)\n", yychar, yytname[yychar1]);
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      if (yylen == 1)
	fprintf (stderr, "Reducing 1 value via line %d, ",
		 yyrline[yyn]);
      else
	fprintf (stderr, "Reducing %d values via line %d, ",
		 yylen, yyrline[yyn]);
    }
#endif


  switch (yyn) {

case 3:
#line 204 "./c-exp.y"
{ write_exp_elt_opcode(OP_TYPE);
			  write_exp_elt_type(yyvsp[0].tval);
			  write_exp_elt_opcode(OP_TYPE);;
    break;}
case 5:
#line 212 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_COMMA); ;
    break;}
case 6:
#line 217 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_IND); ;
    break;}
case 7:
#line 220 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_ADDR); ;
    break;}
case 8:
#line 223 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_NEG); ;
    break;}
case 9:
#line 227 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_ZEROP); ;
    break;}
case 10:
#line 231 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_LOGNOT); ;
    break;}
case 11:
#line 235 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_PREINCREMENT); ;
    break;}
case 12:
#line 239 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_PREDECREMENT); ;
    break;}
case 13:
#line 243 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_POSTINCREMENT); ;
    break;}
case 14:
#line 247 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_POSTDECREMENT); ;
    break;}
case 15:
#line 251 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_SIZEOF); ;
    break;}
case 16:
#line 255 "./c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_PTR);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (STRUCTOP_PTR); ;
    break;}
case 17:
#line 261 "./c-exp.y"
{ /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MPTR); ;
    break;}
case 18:
#line 268 "./c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_MPTR); ;
    break;}
case 19:
#line 272 "./c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); ;
    break;}
case 20:
#line 278 "./c-exp.y"
{ /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  write_exp_elt_opcode (UNOP_ADDR);
			  write_exp_elt_opcode (STRUCTOP_MEMBER); ;
    break;}
case 21:
#line 286 "./c-exp.y"
{ write_exp_elt_opcode (STRUCTOP_MEMBER); ;
    break;}
case 22:
#line 290 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_SUBSCRIPT); ;
    break;}
case 23:
#line 296 "./c-exp.y"
{ start_arglist (); ;
    break;}
case 24:
#line 298 "./c-exp.y"
{ write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); ;
    break;}
case 26:
#line 307 "./c-exp.y"
{ arglist_len = 1; ;
    break;}
case 27:
#line 311 "./c-exp.y"
{ arglist_len++; ;
    break;}
case 28:
#line 315 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_MEMVAL); ;
    break;}
case 29:
#line 321 "./c-exp.y"
{ write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_CAST); ;
    break;}
case 30:
#line 327 "./c-exp.y"
{ ;
    break;}
case 31:
#line 333 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_REPEAT); ;
    break;}
case 32:
#line 337 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_MUL); ;
    break;}
case 33:
#line 341 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_DIV); ;
    break;}
case 34:
#line 345 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_REM); ;
    break;}
case 35:
#line 349 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_ADD); ;
    break;}
case 36:
#line 353 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_SUB); ;
    break;}
case 37:
#line 357 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LSH); ;
    break;}
case 38:
#line 361 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_RSH); ;
    break;}
case 39:
#line 365 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_EQUAL); ;
    break;}
case 40:
#line 369 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_NOTEQUAL); ;
    break;}
case 41:
#line 373 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LEQ); ;
    break;}
case 42:
#line 377 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_GEQ); ;
    break;}
case 43:
#line 381 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LESS); ;
    break;}
case 44:
#line 385 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_GTR); ;
    break;}
case 45:
#line 389 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LOGAND); ;
    break;}
case 46:
#line 393 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LOGXOR); ;
    break;}
case 47:
#line 397 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_LOGIOR); ;
    break;}
case 48:
#line 401 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_AND); ;
    break;}
case 49:
#line 405 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_OR); ;
    break;}
case 50:
#line 409 "./c-exp.y"
{ write_exp_elt_opcode (TERNOP_COND); ;
    break;}
case 51:
#line 413 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN); ;
    break;}
case 52:
#line 417 "./c-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode (yyvsp[-1].opcode);
			  write_exp_elt_opcode (BINOP_ASSIGN_MODIFY); ;
    break;}
case 53:
#line 423 "./c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  if (yyvsp[0].lval == (int) yyvsp[0].lval || yyvsp[0].lval == (unsigned int) yyvsp[0].lval)
			    write_exp_elt_type (builtin_type_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_LONGEST);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 54:
#line 433 "./c-exp.y"
{ YYSTYPE val;
			  parse_number (yyvsp[0].ssym.stoken.ptr, yyvsp[0].ssym.stoken.length, 0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  if (val.lval == (int) val.lval ||
			      val.lval == (unsigned int) val.lval)
			    write_exp_elt_type (builtin_type_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_LONGEST);
			  write_exp_elt_longcst (val.lval);
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 55:
#line 446 "./c-exp.y"
{
			  write_exp_elt_opcode (OP_LONG);
			  if (yyvsp[0].ulval == (unsigned int) yyvsp[0].ulval)
			    write_exp_elt_type (builtin_type_unsigned_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_UNSIGNED_LONGEST);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].ulval);
			  write_exp_elt_opcode (OP_LONG);
			;
    break;}
case 56:
#line 458 "./c-exp.y"
{ YYSTYPE val;
			  parse_number (yyvsp[0].ssym.stoken.ptr, yyvsp[0].ssym.stoken.length, 0, &val);
			  write_exp_elt_opcode (OP_LONG);
			  if (val.ulval == (unsigned int) val.ulval)
			    write_exp_elt_type (builtin_type_unsigned_int);
			  else
			    write_exp_elt_type (BUILTIN_TYPE_UNSIGNED_LONGEST);
			  write_exp_elt_longcst ((LONGEST)val.ulval);
			  write_exp_elt_opcode (OP_LONG);
			;
    break;}
case 57:
#line 471 "./c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_char);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 58:
#line 478 "./c-exp.y"
{ write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (builtin_type_double);
			  write_exp_elt_dblcst (yyvsp[0].dval);
			  write_exp_elt_opcode (OP_DOUBLE); ;
    break;}
case 60:
#line 488 "./c-exp.y"
{ write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LAST); ;
    break;}
case 61:
#line 494 "./c-exp.y"
{ write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_REGISTER); ;
    break;}
case 62:
#line 500 "./c-exp.y"
{ write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern (yyvsp[0].ivar);
			  write_exp_elt_opcode (OP_INTERNALVAR); ;
    break;}
case 63:
#line 506 "./c-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH (yyvsp[-1].tval));
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 64:
#line 513 "./c-exp.y"
{ write_exp_elt_opcode (OP_STRING);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (OP_STRING); ;
    break;}
case 65:
#line 520 "./c-exp.y"
{ write_exp_elt_opcode (OP_THIS);
			  write_exp_elt_opcode (OP_THIS); ;
    break;}
case 66:
#line 527 "./c-exp.y"
{
			  if (yyvsp[0].ssym.sym != 0)
			      yyval.bval = SYMBOL_BLOCK_VALUE (yyvsp[0].ssym.sym);
			  else
			    {
			      struct symtab *tem =
				  lookup_symtab (copy_name (yyvsp[0].ssym.stoken));
			      if (tem)
				yyval.bval = BLOCKVECTOR_BLOCK
					 (BLOCKVECTOR (tem), STATIC_BLOCK);
			      else
				error ("No file or function \"%s\".",
				       copy_name (yyvsp[0].ssym.stoken));
			    }
			;
    break;}
case 67:
#line 545 "./c-exp.y"
{ struct symbol *tem
			    = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					     VAR_NAMESPACE, 0, NULL);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error ("No function \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));
			  yyval.bval = SYMBOL_BLOCK_VALUE (tem); ;
    break;}
case 68:
#line 555 "./c-exp.y"
{ struct symbol *sym;
			  sym = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					       VAR_NAMESPACE, 0, NULL);
			  if (sym == 0)
			    error ("No symbol \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));

			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); ;
    break;}
case 69:
#line 568 "./c-exp.y"
{
			  struct type *type = yyvsp[-2].tval;
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    error ("`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (OP_SCOPE);
			;
    break;}
case 70:
#line 581 "./c-exp.y"
{
			  struct type *type = yyvsp[-3].tval;
			  struct stoken tmp_token;
			  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
			      && TYPE_CODE (type) != TYPE_CODE_UNION)
			    error ("`%s' is not defined as an aggregate type.",
				   TYPE_NAME (type));

			  if (strcmp (type_name_no_tag (type), yyvsp[0].sval.ptr))
			    error ("invalid destructor `%s::~%s'",
				   type_name_no_tag (type), yyvsp[0].sval.ptr);

			  tmp_token.ptr = (char*) alloca (yyvsp[0].sval.length + 2);
			  tmp_token.length = yyvsp[0].sval.length + 1;
			  tmp_token.ptr[0] = '~';
			  memcpy (tmp_token.ptr+1, yyvsp[0].sval.ptr, yyvsp[0].sval.length);
			  tmp_token.ptr[tmp_token.length] = 0;
			  write_exp_elt_opcode (OP_SCOPE);
			  write_exp_elt_type (type);
			  write_exp_string (tmp_token);
			  write_exp_elt_opcode (OP_SCOPE);
			;
    break;}
case 72:
#line 607 "./c-exp.y"
{
			  char *name = copy_name (yyvsp[0].sval);
			  struct symbol *sym;
			  struct minimal_symbol *msymbol;

			  sym =
			    lookup_symbol (name, 0, VAR_NAMESPACE, 0, NULL);
			  if (sym)
			    {
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      break;
			    }

			  msymbol = lookup_minimal_symbol (name,
				      (struct objfile *) NULL);
			  if (msymbol != NULL)
			    {
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_type (builtin_type_int);
			      write_exp_elt_longcst ((LONGEST) msymbol -> address);
			      write_exp_elt_opcode (OP_LONG);
			      write_exp_elt_opcode (UNOP_MEMVAL);
			      if (msymbol -> type == mst_data ||
				  msymbol -> type == mst_bss)
				write_exp_elt_type (builtin_type_int);
			      else if (msymbol -> type == mst_text)
				write_exp_elt_type (lookup_function_type (builtin_type_int));
			      else
				write_exp_elt_type (builtin_type_char);
			      write_exp_elt_opcode (UNOP_MEMVAL);
			    }
			  else
			    if (!have_full_symbols () && !have_partial_symbols ())
			      error ("No symbol table is loaded.  Use the \"file\" command.");
			    else
			      error ("No symbol \"%s\" in current context.", name);
			;
    break;}
case 73:
#line 649 "./c-exp.y"
{ struct symbol *sym = yyvsp[0].ssym.sym;

			  if (sym)
			    {
			      switch (SYMBOL_CLASS (sym))
				{
				case LOC_REGISTER:
				case LOC_ARG:
				case LOC_REF_ARG:
				case LOC_REGPARM:
				case LOC_LOCAL:
				case LOC_LOCAL_ARG:
				  if (innermost_block == 0 ||
				      contained_in (block_found, 
						    innermost_block))
				    innermost_block = block_found;
				case LOC_UNDEF:
				case LOC_CONST:
				case LOC_STATIC:
				case LOC_TYPEDEF:
				case LOC_LABEL:
				case LOC_BLOCK:
				case LOC_CONST_BYTES:

				  /* In this case the expression can
				     be evaluated regardless of what
				     frame we are in, so there is no
				     need to check for the
				     innermost_block.  These cases are
				     listed so that gcc -Wall will
				     report types that may not have
				     been considered.  */

				  break;
				}
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			    }
			  else if (yyvsp[0].ssym.is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
			         not inadvertently convert from a method call
				 to data ref.  */
			      if (innermost_block == 0 || 
				  contained_in (block_found, innermost_block))
				innermost_block = block_found;
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (OP_THIS);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			      write_exp_string (yyvsp[0].ssym.stoken);
			      write_exp_elt_opcode (STRUCTOP_PTR);
			    }
			  else
			    {
			      struct minimal_symbol *msymbol;
			      register char *arg = copy_name (yyvsp[0].ssym.stoken);

			      msymbol = lookup_minimal_symbol (arg,
					  (struct objfile *) NULL);
			      if (msymbol != NULL)
				{
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_type (builtin_type_int);
				  write_exp_elt_longcst ((LONGEST) msymbol -> address);
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				  if (msymbol -> type == mst_data ||
				      msymbol -> type == mst_bss)
				    write_exp_elt_type (builtin_type_int);
				  else if (msymbol -> type == mst_text)
				    write_exp_elt_type (lookup_function_type (builtin_type_int));
				  else
				    write_exp_elt_type (builtin_type_char);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				}
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error ("No symbol table is loaded.  Use the \"file\" command.");
			      else
				error ("No symbol \"%s\" in current context.",
				       copy_name (yyvsp[0].ssym.stoken));
			    }
			;
    break;}
case 75:
#line 737 "./c-exp.y"
{
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = yyvsp[-1].tval;
		  
		  while (!done)
		    switch (pop_type ())
		      {
		      case tp_end:
			done = 1;
			break;
		      case tp_pointer:
			follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_reference:
			follow_type = lookup_reference_type (follow_type);
			break;
		      case tp_array:
			array_size = pop_type_int ();
			if (array_size != -1)
			  follow_type = create_array_type (follow_type,
							   array_size);
			else
			  follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_function:
			follow_type = lookup_function_type (follow_type);
			break;
		      }
		  yyval.tval = follow_type;
		;
    break;}
case 76:
#line 772 "./c-exp.y"
{ push_type (tp_pointer); yyval.voidval = 0; ;
    break;}
case 77:
#line 774 "./c-exp.y"
{ push_type (tp_pointer); yyval.voidval = yyvsp[0].voidval; ;
    break;}
case 78:
#line 776 "./c-exp.y"
{ push_type (tp_reference); yyval.voidval = 0; ;
    break;}
case 79:
#line 778 "./c-exp.y"
{ push_type (tp_reference); yyval.voidval = yyvsp[0].voidval; ;
    break;}
case 81:
#line 783 "./c-exp.y"
{ yyval.voidval = yyvsp[-1].voidval; ;
    break;}
case 82:
#line 785 "./c-exp.y"
{
			  push_type_int (yyvsp[0].lval);
			  push_type (tp_array);
			;
    break;}
case 83:
#line 790 "./c-exp.y"
{
			  push_type_int (yyvsp[0].lval);
			  push_type (tp_array);
			  yyval.voidval = 0;
			;
    break;}
case 84:
#line 796 "./c-exp.y"
{ push_type (tp_function); ;
    break;}
case 85:
#line 798 "./c-exp.y"
{ push_type (tp_function); ;
    break;}
case 86:
#line 802 "./c-exp.y"
{ yyval.lval = -1; ;
    break;}
case 87:
#line 804 "./c-exp.y"
{ yyval.lval = yyvsp[-1].lval; ;
    break;}
case 88:
#line 808 "./c-exp.y"
{ yyval.voidval = 0; ;
    break;}
case 89:
#line 810 "./c-exp.y"
{ free ((PTR)yyvsp[-1].tvec); yyval.voidval = 0; ;
    break;}
case 91:
#line 815 "./c-exp.y"
{ yyval.tval = lookup_member_type (builtin_type_int, yyvsp[-2].tval); ;
    break;}
case 92:
#line 817 "./c-exp.y"
{ yyval.tval = lookup_member_type (yyvsp[-5].tval, yyvsp[-3].tval); ;
    break;}
case 93:
#line 819 "./c-exp.y"
{ yyval.tval = lookup_member_type
			    (lookup_function_type (yyvsp[-7].tval), yyvsp[-5].tval); ;
    break;}
case 94:
#line 822 "./c-exp.y"
{ yyval.tval = lookup_member_type
			    (lookup_function_type (yyvsp[-8].tval), yyvsp[-6].tval);
			  free ((PTR)yyvsp[-1].tvec); ;
    break;}
case 95:
#line 829 "./c-exp.y"
{ yyval.tval = yyvsp[0].tsym.type; ;
    break;}
case 96:
#line 831 "./c-exp.y"
{ yyval.tval = builtin_type_int; ;
    break;}
case 97:
#line 833 "./c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 98:
#line 835 "./c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 99:
#line 837 "./c-exp.y"
{ yyval.tval = builtin_type_long; ;
    break;}
case 100:
#line 839 "./c-exp.y"
{ yyval.tval = builtin_type_unsigned_long; ;
    break;}
case 101:
#line 841 "./c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 102:
#line 843 "./c-exp.y"
{ yyval.tval = builtin_type_long_long; ;
    break;}
case 103:
#line 845 "./c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 104:
#line 847 "./c-exp.y"
{ yyval.tval = builtin_type_unsigned_long_long; ;
    break;}
case 105:
#line 849 "./c-exp.y"
{ yyval.tval = builtin_type_short; ;
    break;}
case 106:
#line 851 "./c-exp.y"
{ yyval.tval = builtin_type_unsigned_short; ;
    break;}
case 107:
#line 853 "./c-exp.y"
{ yyval.tval = lookup_struct (copy_name (yyvsp[0].sval),
					      expression_context_block); ;
    break;}
case 108:
#line 856 "./c-exp.y"
{ yyval.tval = lookup_union (copy_name (yyvsp[0].sval),
					     expression_context_block); ;
    break;}
case 109:
#line 859 "./c-exp.y"
{ yyval.tval = lookup_enum (copy_name (yyvsp[0].sval),
					    expression_context_block); ;
    break;}
case 110:
#line 862 "./c-exp.y"
{ yyval.tval = lookup_unsigned_typename (TYPE_NAME(yyvsp[0].tsym.type)); ;
    break;}
case 111:
#line 864 "./c-exp.y"
{ yyval.tval = builtin_type_unsigned_int; ;
    break;}
case 112:
#line 866 "./c-exp.y"
{ yyval.tval = yyvsp[0].tsym.type; ;
    break;}
case 113:
#line 868 "./c-exp.y"
{ yyval.tval = builtin_type_int; ;
    break;}
case 114:
#line 870 "./c-exp.y"
{ yyval.tval = lookup_template_type(copy_name(yyvsp[-3].sval), yyvsp[-1].tval,
						    expression_context_block);
			;
    break;}
case 116:
#line 877 "./c-exp.y"
{
		  yyval.tsym.stoken.ptr = "int";
		  yyval.tsym.stoken.length = 3;
		  yyval.tsym.type = builtin_type_int;
		;
    break;}
case 117:
#line 883 "./c-exp.y"
{
		  yyval.tsym.stoken.ptr = "long";
		  yyval.tsym.stoken.length = 4;
		  yyval.tsym.type = builtin_type_long;
		;
    break;}
case 118:
#line 889 "./c-exp.y"
{
		  yyval.tsym.stoken.ptr = "short";
		  yyval.tsym.stoken.length = 5;
		  yyval.tsym.type = builtin_type_short;
		;
    break;}
case 119:
#line 898 "./c-exp.y"
{ yyval.tvec = (struct type **)xmalloc (sizeof (struct type *) * 2);
		  yyval.tvec[0] = (struct type *)0;
		  yyval.tvec[1] = yyvsp[0].tval;
		;
    break;}
case 120:
#line 903 "./c-exp.y"
{ int len = sizeof (struct type *) * ++(yyvsp[-2].ivec[0]);
		  yyval.tvec = (struct type **)xrealloc ((char *) yyvsp[-2].tvec, len);
		  yyval.tvec[yyval.ivec[0]] = yyvsp[0].tval;
		;
    break;}
case 121:
#line 909 "./c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
case 122:
#line 910 "./c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
case 123:
#line 911 "./c-exp.y"
{ yyval.sval = yyvsp[0].tsym.stoken; ;
    break;}
case 124:
#line 912 "./c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
case 125:
#line 913 "./c-exp.y"
{ yyval.sval = yyvsp[0].ssym.stoken; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 327 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;
      yyerror("parse error");
    }

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 928 "./c-exp.y"


/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (p, len, parsed_float, putithere)
     register char *p;
     register int len;
     int parsed_float;
     YYSTYPE *putithere;
{
  register LONGEST n = 0;
  register LONGEST prevn = 0;
  register int i;
  register int c;
  register int base = input_radix;
  int unsigned_p = 0;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      putithere->dval = atof (p);
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
  if (p[0] == '0')
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;

      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
	    len -= 2;
	  }
	break;

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	n += i = c - '0';
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    n += i = c - 'a' + 10;
	  else if (len == 0 && c == 'l')
	    ;
	  else if (len == 0 && c == 'u')
	    unsigned_p = 1;
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base */
      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  */
      if((prevn >= n) && n != 0)
	 unsigned_p=1;		/* Try something unsigned */
      /* If range checking enabled, portably test for unsigned overflow.  */
      if(RANGE_CHECK && n!=0)
      {	
	 if((unsigned_p && (unsigned)prevn >= (unsigned)n))
	    range_error("Overflow on numeric constant.");	 
      }
      prevn=n;
    }

  if (unsigned_p)
    {
      putithere->ulval = n;
      return UINT;
    }
  else
    {
      putithere->lval = n;
      return INT;
    }
}

struct token
{
  char *operator;
  int token;
  enum exp_opcode opcode;
};

const static struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH}
  };

const static struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_LOGIOR},
    {"&=", ASSIGN_MODIFY, BINOP_LOGAND},
    {"^=", ASSIGN_MODIFY, BINOP_LOGXOR},
    {"++", INCREMENT, BINOP_END},
    {"--", DECREMENT, BINOP_END},
    {"->", ARROW, BINOP_END},
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"::", COLONCOLON, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END}
  };

/* Read one token, getting characters through lexptr.  */

int
yylex ()
{
  register int c;
  register int namelen;
  register unsigned i;
  register char *tokstart;

 retry:

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (!strncmp (tokstart, tokentab3[i].operator, 3))
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (!strncmp (tokstart, tokentab2[i].operator, 2))
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '\'':
      lexptr++;
      c = *lexptr++;
      if (c == '\\')
	c = parse_escape (&lexptr);
      yylval.lval = c;
      c = *lexptr++;
      if (c != '\'')
	error ("Invalid character constant.");
      return CHAR;

    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol. */
      /* FALL THRU into number case.  */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, toktype;
	register char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T' || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }

	for (;; ++p)
	  {
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!hex && !got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
				  && (*p < 'A' || *p > 'Z')))
	      break;
	  }
	toktype = parse_number (tokstart, p - tokstart, got_dot|got_e, &yylval);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    bcopy (tokstart, err_copy, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error ("Invalid number \"%s\".", err_copy);
	  }
	lexptr = p;
	return toktype;
      }

    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '"':
      for (namelen = 1; (c = tokstart[namelen]) != '"'; namelen++)
	if (c == '\\')
	  {
	    c = tokstart[++namelen];
	    if (c >= '0' && c <= '9')
	      {
		c = tokstart[++namelen];
		if (c >= '0' && c <= '9')
		  c = tokstart[++namelen];
	      }
	  }
      yylval.sval.ptr = tokstart + 1;
      yylval.sval.length = namelen - 1;
      lexptr += namelen + 1;
      return STRING;
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error ("Invalid character '%c' in expression.", c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
       c = tokstart[++namelen])
    ;

  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    {
      return 0;
    }

  lexptr += namelen;

  /* Handle the tokens $digits; also $ (short for $0) and $$ (short for $$1)
     and $$digits (equivalent to $<-digits> if you could type that).
     Make token type LAST, and put the number (the digits) in yylval.  */

  if (*tokstart == '$')
    {
      register int negate = 0;
      c = 1;
      /* Double dollar means negate the number and add -1 as well.
	 Thus $$ alone means -1.  */
      if (namelen >= 2 && tokstart[1] == '$')
	{
	  negate = 1;
	  c = 2;
	}
      if (c == namelen)
	{
	  /* Just dollars (one or two) */
	  yylval.lval = - negate;
	  return LAST;
	}
      /* Is the rest of the token digits?  */
      for (; c < namelen; c++)
	if (!(tokstart[c] >= '0' && tokstart[c] <= '9'))
	  break;
      if (c == namelen)
	{
	  yylval.lval = atoi (tokstart + 1 + negate);
	  if (negate)
	    yylval.lval = - yylval.lval;
	  return LAST;
	}
    }

  /* Handle tokens that refer to machine registers:
     $ followed by a register name.  */

  if (*tokstart == '$') {
    for (c = 0; c < NUM_REGS; c++)
      if (namelen - 1 == strlen (reg_names[c])
	  && !strncmp (tokstart + 1, reg_names[c], namelen - 1))
	{
	  yylval.lval = c;
	  return REGNAME;
	}
    for (c = 0; c < num_std_regs; c++)
     if (namelen - 1 == strlen (std_regs[c].name)
	 && !strncmp (tokstart + 1, std_regs[c].name, namelen - 1))
       {
	 yylval.lval = std_regs[c].regnum;
	 return REGNAME;
       }
  }
  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 8:
      if (!strncmp (tokstart, "unsigned", 8))
	return UNSIGNED;
      if (current_language->la_language == language_cplus
	  && !strncmp (tokstart, "template", 8))
	return TEMPLATE;
      break;
    case 6:
      if (!strncmp (tokstart, "struct", 6))
	return STRUCT;
      if (!strncmp (tokstart, "signed", 6))
	return SIGNED_KEYWORD;
      if (!strncmp (tokstart, "sizeof", 6))      
	return SIZEOF;
      break;
    case 5:
      if (!strncmp (tokstart, "union", 5))
	return UNION;
      if (!strncmp (tokstart, "short", 5))
	return SHORT;
      break;
    case 4:
      if (!strncmp (tokstart, "enum", 4))
	return ENUM;
      if (!strncmp (tokstart, "long", 4))
	return LONG;
      if (current_language->la_language == language_cplus
	  && !strncmp (tokstart, "this", 4))
	{
	  static const char this_name[] =
				 { CPLUS_MARKER, 't', 'h', 'i', 's', '\0' };

	  if (lookup_symbol (this_name, expression_context_block,
			     VAR_NAMESPACE, 0, NULL))
	    return THIS;
	}
      break;
    case 3:
      if (!strncmp (tokstart, "int", 3))
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Any other names starting in $ are debugger internal variables.  */

  if (*tokstart == '$')
    {
      yylval.ivar =  lookup_internalvar (copy_name (yylval.sval) + 1);
      return VARIABLE;
    }

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    char *tmp = copy_name (yylval.sval);
    struct symbol *sym;
    int is_a_field_of_this = 0;
    int hextype;

    sym = lookup_symbol (tmp, expression_context_block,
			 VAR_NAMESPACE,
			 current_language->la_language == language_cplus
			 ? &is_a_field_of_this : NULL,
			 NULL);
    if ((sym && SYMBOL_CLASS (sym) == LOC_BLOCK) ||
        lookup_partial_symtab (tmp))
      {
	yylval.ssym.sym = sym;
	yylval.ssym.is_a_field_of_this = is_a_field_of_this;
	return BLOCKNAME;
      }
    if (sym && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
        {
	  yylval.tsym.type = SYMBOL_TYPE (sym);
	  return TYPENAME;
        }
    if ((yylval.tsym.type = lookup_primitive_typename (tmp)) != 0)
	return TYPENAME;

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!sym && 
        ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10) ||
         (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this;
	    return NAME_OR_INT;
	  }
	if (hextype == UINT)
	  {
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this;
	    return NAME_OR_UINT;
	  }
      }

    /* Any other kind of symbol */
    yylval.ssym.sym = sym;
    yylval.ssym.is_a_field_of_this = is_a_field_of_this;
    return NAME;
  }
}

void
yyerror (msg)
     char *msg;
{
  error (msg ? msg : "Invalid syntax in expression.");
}

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.  */

const static struct op_print c_op_print_tab[] =
  {
    {",",  BINOP_COMMA, PREC_COMMA, 0},
    {"=",  BINOP_ASSIGN, PREC_ASSIGN, 1},
    {"||", BINOP_OR, PREC_OR, 0},
    {"&&", BINOP_AND, PREC_AND, 0},
    {"|",  BINOP_LOGIOR, PREC_LOGIOR, 0},
    {"&",  BINOP_LOGAND, PREC_LOGAND, 0},
    {"^",  BINOP_LOGXOR, PREC_LOGXOR, 0},
    {"==", BINOP_EQUAL, PREC_EQUAL, 0},
    {"!=", BINOP_NOTEQUAL, PREC_EQUAL, 0},
    {"<=", BINOP_LEQ, PREC_ORDER, 0},
    {">=", BINOP_GEQ, PREC_ORDER, 0},
    {">",  BINOP_GTR, PREC_ORDER, 0},
    {"<",  BINOP_LESS, PREC_ORDER, 0},
    {">>", BINOP_RSH, PREC_SHIFT, 0},
    {"<<", BINOP_LSH, PREC_SHIFT, 0},
    {"+",  BINOP_ADD, PREC_ADD, 0},
    {"-",  BINOP_SUB, PREC_ADD, 0},
    {"*",  BINOP_MUL, PREC_MUL, 0},
    {"/",  BINOP_DIV, PREC_MUL, 0},
    {"%",  BINOP_REM, PREC_MUL, 0},
    {"@",  BINOP_REPEAT, PREC_REPEAT, 0},
    {"-",  UNOP_NEG, PREC_PREFIX, 0},
    {"!",  UNOP_ZEROP, PREC_PREFIX, 0},
    {"~",  UNOP_LOGNOT, PREC_PREFIX, 0},
    {"*",  UNOP_IND, PREC_PREFIX, 0},
    {"&",  UNOP_ADDR, PREC_PREFIX, 0},
    {"sizeof ", UNOP_SIZEOF, PREC_PREFIX, 0},
    {"++", UNOP_PREINCREMENT, PREC_PREFIX, 0},
    {"--", UNOP_PREDECREMENT, PREC_PREFIX, 0},
    /* C++  */
    {"::", BINOP_SCOPE, PREC_PREFIX, 0},
};

/* These variables point to the objects
   representing the predefined C data types.  */

struct type *builtin_type_void;
struct type *builtin_type_char;
struct type *builtin_type_short;
struct type *builtin_type_int;
struct type *builtin_type_long;
struct type *builtin_type_long_long;
struct type *builtin_type_signed_char;
struct type *builtin_type_unsigned_char;
struct type *builtin_type_unsigned_short;
struct type *builtin_type_unsigned_int;
struct type *builtin_type_unsigned_long;
struct type *builtin_type_unsigned_long_long;
struct type *builtin_type_float;
struct type *builtin_type_double;
struct type *builtin_type_long_double;
struct type *builtin_type_complex;
struct type *builtin_type_double_complex;

struct type ** const (c_builtin_types[]) = 
{
  &builtin_type_int,
  &builtin_type_long,
  &builtin_type_short,
  &builtin_type_char,
  &builtin_type_float,
  &builtin_type_double,
  &builtin_type_void,
  &builtin_type_long_long,
  &builtin_type_signed_char,
  &builtin_type_unsigned_char,
  &builtin_type_unsigned_short,
  &builtin_type_unsigned_int,
  &builtin_type_unsigned_long,
  &builtin_type_unsigned_long_long,
  &builtin_type_long_double,
  &builtin_type_complex,
  &builtin_type_double_complex,
  0
};

const struct language_defn c_language_defn = {
  "c",				/* Language name */
  language_c,
  c_builtin_types,
  range_check_off,
  type_check_off,
  c_parse,
  c_error,
  &BUILTIN_TYPE_LONGEST,	 /* longest signed   integral type */
  &BUILTIN_TYPE_UNSIGNED_LONGEST,/* longest unsigned integral type */
  &builtin_type_double,		/* longest floating point type */ /*FIXME*/
  "0x%x", "0x%", "x",		/* Hex   format, prefix, suffix */
  "0%o",  "0%",  "o",		/* Octal format, prefix, suffix */
  c_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

const struct language_defn cplus_language_defn = {
  "c++",				/* Language name */
  language_cplus,
  c_builtin_types,
  range_check_off,
  type_check_off,
  c_parse,
  c_error,
  &BUILTIN_TYPE_LONGEST,	 /* longest signed   integral type */
  &BUILTIN_TYPE_UNSIGNED_LONGEST,/* longest unsigned integral type */
  &builtin_type_double,		/* longest floating point type */ /*FIXME*/
  "0x%x", "0x%", "x",		/* Hex   format, prefix, suffix */
  "0%o",  "0%",  "o",		/* Octal format, prefix, suffix */
  c_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

void
_initialize_c_exp ()
{
  builtin_type_void =
    init_type (TYPE_CODE_VOID, 1,
	       0,
	       "void", (struct objfile *) NULL);
  builtin_type_char =
    init_type (TYPE_CODE_INT, TARGET_CHAR_BIT / TARGET_CHAR_BIT,
	       0,
	       "char", (struct objfile *) NULL);
  builtin_type_signed_char =
    init_type (TYPE_CODE_INT, TARGET_CHAR_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_SIGNED,
	       "signed char", (struct objfile *) NULL);
  builtin_type_unsigned_char =
    init_type (TYPE_CODE_INT, TARGET_CHAR_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_UNSIGNED,
	       "unsigned char", (struct objfile *) NULL);
  builtin_type_short =
    init_type (TYPE_CODE_INT, TARGET_SHORT_BIT / TARGET_CHAR_BIT,
	       0,
	       "short", (struct objfile *) NULL);
  builtin_type_unsigned_short =
    init_type (TYPE_CODE_INT, TARGET_SHORT_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_UNSIGNED,
	       "unsigned short", (struct objfile *) NULL);
  builtin_type_int =
    init_type (TYPE_CODE_INT, TARGET_INT_BIT / TARGET_CHAR_BIT,
	       0,
	       "int", (struct objfile *) NULL);
  builtin_type_unsigned_int =
    init_type (TYPE_CODE_INT, TARGET_INT_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_UNSIGNED,
	       "unsigned int", (struct objfile *) NULL);
  builtin_type_long =
    init_type (TYPE_CODE_INT, TARGET_LONG_BIT / TARGET_CHAR_BIT,
	       0,
	       "long", (struct objfile *) NULL);
  builtin_type_unsigned_long =
    init_type (TYPE_CODE_INT, TARGET_LONG_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_UNSIGNED,
	       "unsigned long", (struct objfile *) NULL);
  builtin_type_long_long =
    init_type (TYPE_CODE_INT, TARGET_LONG_LONG_BIT / TARGET_CHAR_BIT,
	       0,
	       "long long", (struct objfile *) NULL);
  builtin_type_unsigned_long_long = 
    init_type (TYPE_CODE_INT, TARGET_LONG_LONG_BIT / TARGET_CHAR_BIT,
	       TYPE_FLAG_UNSIGNED,
	       "unsigned long long", (struct objfile *) NULL);
  builtin_type_float =
    init_type (TYPE_CODE_FLT, TARGET_FLOAT_BIT / TARGET_CHAR_BIT,
	       0,
	       "float", (struct objfile *) NULL);
  builtin_type_double =
    init_type (TYPE_CODE_FLT, TARGET_DOUBLE_BIT / TARGET_CHAR_BIT,
	       0,
	       "double", (struct objfile *) NULL);
  builtin_type_long_double =
    init_type (TYPE_CODE_FLT, TARGET_LONG_DOUBLE_BIT / TARGET_CHAR_BIT,
	       0,
	       "long double", (struct objfile *) NULL);
  builtin_type_complex =
    init_type (TYPE_CODE_FLT, TARGET_COMPLEX_BIT / TARGET_CHAR_BIT,
	       0,
	       "complex", (struct objfile *) NULL);
  builtin_type_double_complex =
    init_type (TYPE_CODE_FLT, TARGET_DOUBLE_COMPLEX_BIT / TARGET_CHAR_BIT,
	       0,
	       "double complex", (struct objfile *) NULL);

  add_language (&c_language_defn);
  add_language (&cplus_language_defn);
}
