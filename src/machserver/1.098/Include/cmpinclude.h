/*
(c) Copyright Taiichi Yuasa and Masami Hagiya, 1984.  All rights reserved.
Copying of this file is authorized to users who have executed the true and
proper "License Agreement for Kyoto Common LISP" with SIGLISP.
*/
#include <stdio.h>
#include <setjmp.h>
#define	TRUE	1
#define	FALSE	0
typedef int bool;
typedef int fixnum;
typedef float shortfloat;
typedef double longfloat;
typedef union lispunion *object;
#define	OBJNULL	((object)NULL)
struct fixnum_struct {
	short	t, m;
	fixnum	FIXVAL;
};
#define	fix(x)	(x)->FIX.FIXVAL
#define	SMALL_FIXNUM_LIMIT	1024
struct fixnum_struct small_fixnum_table[];
#define	small_fixnum(i)	(object)(small_fixnum_table+SMALL_FIXNUM_LIMIT+(i))
struct shortfloat_struct {
	short		t, m;
	shortfloat	SFVAL;
};
#define	sf(x)	(x)->SF.SFVAL
struct longfloat_struct {
	short		t, m;
	longfloat	LFVAL;
};
#define	lf(x)	(x)->LF.LFVAL
struct character {
	short		t, m;
	unsigned short	ch_code;
	unsigned char	ch_font;
	unsigned char	ch_bits;
};
struct character character_table[];
#define	code_char(c)	(object)(character_table+(c))
#define	char_code(x)	(x)->ch.ch_code
#define	char_font(x)	(x)->ch.ch_font
#define	char_bits(x)	(x)->ch.ch_bits
enum stype {
	stp_ordinary,
	stp_constant,
        stp_special
};
struct symbol {
	short	t, m;
	object	s_dbind;
	int	(*s_sfdef)();
#define	s_fillp		st_fillp
#define	s_self		st_self
	int	s_fillp;
	char	*s_self;
	object	s_gfdef;
	object	s_plist;
	object	s_hpack;
	short	s_stype;
	short	s_mflag;
};
struct cons {
	short	t, m;
	object	c_cdr;
	object	c_car;
};
struct array {
	short	t, m;
	short	a_rank;
	short	a_adjustable;
	int	a_dim;
	int	*a_dims;
	object	*a_self;
	object	a_displaced;
	short	a_elttype;
	short	a_offset;
};
struct vector {
	short	t, m;
	short	v_hasfillp;
	short	v_adjustable;
	int	v_dim;
	int	v_fillp;
	object	*v_self;
	object	v_displaced;
	short	v_elttype;
	short	v_offset;
};
struct string {
	short	t, m;
	short	st_hasfillp;
	short	st_adjustable;
	int	st_dim;
	int	st_fillp;
	char	*st_self;
	object	st_displaced;
};
struct ustring {
	short	t, m;
	short	ust_hasfillp;
	short	ust_adjustable;
	int	ust_dim;
	int	ust_fillp;
	unsigned char
		*ust_self;
	object	ust_displaced;
};
struct bitvector {
	short	t, m;
	short	bv_hasfillp;
	short	bv_adjustable;
	int	bv_dim;
	int	bv_fillp;
	char	*bv_self;
	object	bv_displaced;
	short	bv_elttype;
	short	bv_offset;
};
struct fixarray {
	short	t, m;
	short	fixa_rank;
	short	fixa_adjustable;
	int	fixa_dim;
	int	*fixa_dims;
	fixnum	*fixa_self;
	object	fixa_displaced;
	short	fixa_elttype;
	short	fixa_offset;
};
struct sfarray {
	short	t, m;
	short	sfa_rank;
	short	sfa_adjustable;
	int	sfa_dim;
	int	*sfa_dims;
	shortfloat
		*sfa_self;
	object	sfa_displaced;
	short	sfa_elttype;
	short	sfa_offset;
};
struct lfarray {
	short	t, m;
	short	lfa_rank;
	short	lfa_adjustable;
	int	lfa_dim;
	int	*lfa_dims;
	longfloat
		*lfa_self;
	object	lfa_displaced;
	short	lfa_elttype;
	short	lfa_offset;
};
struct structure {
	short	t, m;
	object	str_name;
	object	*str_self;
	int	str_length;
};
struct cfun {
	short	t, m;
	object	cf_name;
	int	(*cf_self)();
	object	cf_data;
	char	*cf_start;
	int	cf_size;
};
struct cclosure {
	short	t, m;
	object	cc_name;
	int	(*cc_self)();
	object	cc_env;
	object	cc_data;
	char	*cc_start;
	int	cc_size;
	object	*cc_turbo;
};
struct dummy {
	short	t, m;
};
union lispunion {
	struct fixnum_struct
			FIX;
	struct shortfloat_struct
			SF;
	struct longfloat_struct
			LF;
	struct character
			ch;
	struct symbol	s;
	struct cons	c;
	struct array	a;
	struct vector	v;
	struct string	st;
	struct ustring	ust;
	struct bitvector
			bv;
	struct structure
			str;
	struct cfun	cf;
	struct cclosure	cc;
	struct dummy	d;
	struct fixarray	fixa;
	struct sfarray	sfa;
	struct lfarray	lfa;
};
enum type {
	t_cons = 0,
	t_start = t_cons,
	t_fixnum,
	t_bignum,
	t_ratio,
	t_shortfloat,
	t_longfloat,
	t_complex,
	t_character,
	t_symbol,
	t_package,
	t_hashtable,
	t_array,
	t_vector,
	t_string,
	t_bitvector,
	t_structure,
	t_stream,
	t_random,
	t_readtable,
	t_pathname,
	t_cfun,
	t_cclosure,
	t_spice,
	t_end,
	t_contiguous,
	t_relocatable,
	t_other
};
#define	type_of(obje)	((enum type)(((object)(obje))->d.t))
#define	endp(obje)	endp1(obje)
object value_stack[];
#define	vs_org		value_stack
object *vs_limit;
object *vs_base;
object *vs_top;
#define	vs_push(obje)	(*vs_top++ = (obje))
#define	vs_pop		(*--vs_top)
#define	vs_head		vs_top[-1]
#define	vs_mark		object *old_vs_top = vs_top
#define	vs_reset	vs_top = old_vs_top
#define	vs_check	if (vs_top >= vs_limit)  \
				vs_overflow();
#define	vs_check_push(obje)  \
			(vs_top >= vs_limit ?  \
			 (object)vs_overflow() : (*vs_top++ = (obje)))
#define	check_arg(n)  \
			if (vs_top - vs_base != (n))  \
				check_arg_failed(n)
#define	MMcheck_arg(n)  \
			if (vs_top - vs_base < (n))  \
				too_few_arguments();  \
			else if (vs_top - vs_base > (n))  \
				too_many_arguments()
#define vs_reserve(x)	if(vs_base+(x) >= vs_limit)  \
				vs_overflow();
struct bds_bd {
	object	bds_sym;
	object	bds_val;
};
struct bds_bd bind_stack[];
#define bds_org		bind_stack
typedef struct bds_bd *bds_ptr;
bds_ptr bds_limit;
bds_ptr bds_top;
#define	bds_check  \
	if (bds_top >= bds_limit)  \
		bds_overflow()
#define	bds_bind(sym, val)  \
	((++bds_top)->bds_sym = (sym),  \
	bds_top->bds_val = (sym)->s.s_dbind,  \
	(sym)->s.s_dbind = (val))
#define	bds_unwind1  \
	((bds_top->bds_sym)->s.s_dbind = bds_top->bds_val, --bds_top)
typedef struct invocation_history {
	object	ihs_function;
	object	*ihs_base;
} *ihs_ptr;
struct invocation_history ihs_stack[];
#define ihs_org		ihs_stack
ihs_ptr ihs_limit;
ihs_ptr ihs_top;
#define	ihs_check  \
	if (ihs_top >= ihs_limit)  \
		ihs_overflow()
#define ihs_push(function)  \
	(++ihs_top)->ihs_function = (function);  \
	ihs_top->ihs_base = vs_base
#define ihs_pop() 	(ihs_top--)
enum fr_class {
	FRS_CATCH,
	FRS_CATCHALL,
	FRS_PROTECT
};
struct frame {
	jmp_buf		frs_jmpbuf;
	object		*frs_lex;
	bds_ptr		frs_bds_top;
	enum fr_class	frs_class;
	object		frs_val;
	ihs_ptr		frs_ihs;
};
typedef struct frame *frame_ptr;
#define	alloc_frame_id()	alloc_object(t_spice)
struct frame frame_stack[];
#define frs_org		frame_stack
frame_ptr frs_limit;
frame_ptr frs_top;
#define frs_push(class, val)  \
	if (++frs_top >= frs_limit)  \
		frs_overflow();  \
	frs_top->frs_lex = lex_env;\
	frs_top->frs_bds_top = bds_top;  \
	frs_top->frs_class = (class);  \
	frs_top->frs_val = (val);  \
	frs_top->frs_ihs = ihs_top;  \
        setjmp(frs_top->frs_jmpbuf)
#define frs_pop()	frs_top--
bool nlj_active;
frame_ptr nlj_fr;
object nlj_tag;
object *lex_env;
object caar();
object cadr();
object cdar();
object cddr();
object caaar();
object caadr();
object cadar();
object caddr();
object cdaar();
object cdadr();
object cddar();
object cdddr();
object caaaar();
object caaadr();
object caadar();
object caaddr();
object cadaar();
object cadadr();
object caddar();
object cadddr();
object cdaaar();
object cdaadr();
object cdadar();
object cdaddr();
object cddaar();
object cddadr();
object cdddar();
object cddddr();
#define MMcons(a,d)	make_cons((a),(d))
#define MMcar(x)	(x)->c.c_car
#define MMcdr(x)	(x)->c.c_cdr
#define CMPcar(x)	(x)->c.c_car
#define CMPcdr(x)	(x)->c.c_cdr
#define CMPcaar(x)	(x)->c.c_car->c.c_car
#define CMPcadr(x)	(x)->c.c_cdr->c.c_car
#define CMPcdar(x)	(x)->c.c_car->c.c_cdr
#define CMPcddr(x)	(x)->c.c_cdr->c.c_cdr
#define CMPcaaar(x)	(x)->c.c_car->c.c_car->c.c_car
#define CMPcaadr(x)	(x)->c.c_cdr->c.c_car->c.c_car
#define CMPcadar(x)	(x)->c.c_car->c.c_cdr->c.c_car
#define CMPcaddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_car
#define CMPcdaar(x)	(x)->c.c_car->c.c_car->c.c_cdr
#define CMPcdadr(x)	(x)->c.c_cdr->c.c_car->c.c_cdr
#define CMPcddar(x)	(x)->c.c_car->c.c_cdr->c.c_cdr
#define CMPcdddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_cdr
#define CMPcaaaar(x)	(x)->c.c_car->c.c_car->c.c_car->c.c_car
#define CMPcaaadr(x)	(x)->c.c_cdr->c.c_car->c.c_car->c.c_car
#define CMPcaadar(x)	(x)->c.c_car->c.c_cdr->c.c_car->c.c_car
#define CMPcaaddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_car->c.c_car
#define CMPcadaar(x)	(x)->c.c_car->c.c_car->c.c_cdr->c.c_car
#define CMPcadadr(x)	(x)->c.c_cdr->c.c_car->c.c_cdr->c.c_car
#define CMPcaddar(x)	(x)->c.c_car->c.c_cdr->c.c_cdr->c.c_car
#define CMPcadddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_cdr->c.c_car
#define CMPcdaaar(x)	(x)->c.c_car->c.c_car->c.c_car->c.c_cdr
#define CMPcdaadr(x)	(x)->c.c_cdr->c.c_car->c.c_car->c.c_cdr
#define CMPcdadar(x)	(x)->c.c_car->c.c_cdr->c.c_car->c.c_cdr
#define CMPcdaddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_car->c.c_cdr
#define CMPcddaar(x)	(x)->c.c_car->c.c_car->c.c_cdr->c.c_cdr
#define CMPcddadr(x)	(x)->c.c_cdr->c.c_car->c.c_cdr->c.c_cdr
#define CMPcdddar(x)	(x)->c.c_car->c.c_cdr->c.c_cdr->c.c_cdr
#define CMPcddddr(x)	(x)->c.c_cdr->c.c_cdr->c.c_cdr->c.c_cdr
#define CMPfuncall	funcall
#define	cclosure_call	funcall
object simple_lispcall();
object simple_lispcall_no_event();
object simple_symlispcall();
object simple_symlispcall_no_event();
object CMPtemp;
object CMPtemp1;
object CMPtemp2;
object CMPtemp3;
#define	Cnil	((object)&Cnil_body)
#define	Ct	((object)&Ct_body)
struct symbol Cnil_body, Ct_body;
object MF();
object MM();
object Scons;
object siSfunction_documentation;
object siSvariable_documentation;
object siSpretty_print_format;
object Slist;
object keyword_package;
object alloc_object();
object car();
object cdr();
object list();
object listA();
object coerce_to_string();
object elt();
object elt_set();
frame_ptr frs_sch();
frame_ptr frs_sch_catch();
object make_cclosure();
object nth();
object nthcdr();
object make_cons();
object append();
object nconc();
object reverse();
object nreverse();
object number_expt();
object number_minus();
object number_negate();
object number_plus();
object number_times();
object one_minus();
object one_plus();
object get();
object getf();
object putprop();
object remprop();
object string_to_object();
object symbol_function();
object symbol_value();
object make_fixnum();
object make_shortfloat();
object make_longfloat();
object structure_ref();
object structure_set();
object princ();
object prin1();
object print();
object terpri();
object aref();
object aset();
object aref1();
object aset1();
char object_to_char();
int object_to_int();
float object_to_float();
double object_to_double();
int FIXtemp;
#define	CMPmake_fixnum(x) \
((((FIXtemp=(x))+1024)&-2048)==0?small_fixnum(FIXtemp):make_fixnum(FIXtemp))
#define Creturn(v) return((vs_top=vs,(v)))
#define Cexit return((vs_top=vs,0))
double sin(), cos(), tan();
