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

typedef union  {
	unsigned int	num;
	struct { char *str; int len; }
			string;
	symbolType      *sym;
        operandType	operand;
	exprType	*expr;
} YYSTYPE;
extern YYSTYPE yylval;
