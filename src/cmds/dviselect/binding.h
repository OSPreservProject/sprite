/* C-TeX function and variable bindings */

/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifdef StringSizeof
/*
 * If StringSizeof is defined, we assume that (sizeof "string_const") is
 * equal to (strlen("string_const") + 1); see the DefXXX macros below.
 */
#endif

/*
 * Here are the various flags that can be in the b_flags field of a struct
 * BoundName.
 */
#define BIsSpecial	1	/* a special int or dimen, or a font;
				   assignments to these are always
				   global */
#define BIsOuter	2	/* \outer, may not appear in macro def'n */
#define BIsLong		4	/* \long, may contain \outer s anyway */
#define BIsUndef	8	/* not defined anymore - used when a def'n
				   local to a group is destroyed by exiting
				   the group */

/*
 * SPECIAL
 * 1. Integers:
 *	\spacefactor \prevgraf \deadcycles \insertpenalties
 * 2. Dimens:
 *	\prevdepth \pagegoal \pagetotal \pagestretch \pagefilstretch
 *	\pagefillstretch \pagefilllstretch \pageshrink \pagedepth
 */

enum BindingKind {
	ProcBound, VarBound, IntBound, DimenBound, CodeBound,
	FontBound
};

union BindUnion {
	int	(*b_proc)();		/* iff ProcBound */
	struct	node *b_var;		/* iff VarBound */
	i32	*b_int;			/* iff IntBound */
	scaled	*b_dimen;		/* iff DimenBound */
	i32	b_code;			/* iff CodeBound */
	struct	FontInfo *b_font;	/* iff FontBound */
};

/*
 * The basic entity of C-TeX is the `BoundName'.  It is either a wired-in
 * procedure (e.g. \let, \def, \vskip) or variable (\hsize, \parskip), or a
 * user-defined macro/variable/whatnot.  These are indistinguishable from
 * one another (except that only wired procedures are ProcBound).
 *
 * There are also `CodeBound' things, which have no b_name and no b_flags.
 * In fact, they are really BoundNameCodes, but they need similar treatment,
 * so they get a similar structure.
 */
struct BoundName {
	enum	BindingKind b_binding;	/* the kind of thing it is */
	union	BindUnion b_bound;	/* the current binding */
	struct	SavedVal *b_AfterGroupRst;
					/* The saved value for restoration
					   after the current group.  This
					   is a pointer into the current
					   AfterGroupRst list. */
	struct	string b_name;		/* the name of this thing */
	int	b_flags;		/* various flags */
};

/*
 * \catcodes, \lccodes, etc are CodeBound, and are really arrays of these:
 */
struct BoundNameCode {
	enum	BindingKind b_binding;
	union	BindUnion b_bound;
	struct	SavedVal *b_AfterGroupRst;
};

/*
 * Saved values (changes that are local to a group) are stored in a doubly
 * linked list; this is so that constructs like
 *	{\advance\count0 by1 \global\advance\count0 by1}
 * can easily delete restorations that are no longer needed (having been
 * overridden by a \global operation).  In other words, we never have
 * any `save stack buildup'.  This requires the `spaghetti' below: a
 * current group level (so if b_AfterGroupRst is set, we can tell whether
 * the saved val is for this level) and moreover any inner saves for this
 * BoundName.
 *
 * To illustrate what is going on, suppose that we have count1==0.  Now we
 * begin group number 1, then 2.  Now we set count1 to 5, and since it has
 * no AfterGroupRst we save the 0 at level 2.  Now we increment count1, and
 * since it has an AfterGroupRst, we check the level.  It is 2; everything
 * is fine, and count1 is set to 6.  Now we begin group 3, and again increment
 * count1.  The level is 2, so we make a new SavedVal and set its level to
 * 3.  (We now have count1==7, save==6 when exiting level 3, save==0 when
 * exiting 2.)  Now the user does a \global\count1=42, so alas!, all those
 * saved values are useless.  To get rid of them, we delete the current
 * SavedVal, and its inner, and its inner's inner, and so forth, then clear
 * the AfterGroupRst pointer.  Voila!  A global \count1, set to 42.
 *
 * Note that we save CodeBound changes here too.  Since CodeBound objects
 * have no flags, some care is needed during restoration.
 */
struct SavedVal {
	int	sv_level;		/* the level to which this belongs */
	struct	SavedVal *sv_next;	/* linked list */
	struct	SavedVal *sv_prev;	/* or more precisely, queue */
	struct	SavedVal *sv_inner;	/* the inner saved value (from the
					   previous group that saved this) */
	union	BindUnion sv_val;	/* the saved value (note that saved
					   values are always the same type
					   as current values---i.e., types
					   are fixed) */
	struct	BoundName *sv_b;	/* the BoundName to which it belongs */
	int	sv_flags;		/* the saved b_flags (if applicable) */
};

struct	BoundName **NewNames;	/* during initialization, each internal
				   BoundName is stashed in the table to
				   which this points */

int	CurrentGroup;		/* the group level number of the current
				   group; incremented for { and decremented
				   for }, etc */

struct	SavedVal *AfterGroupRst;/* the current list for restoring locally
				   modified BoundNames */

/*
 * The following hackery will make the compiler complain in the event
 * of a dropped semicolon, without making lint complain.
 */
#ifdef lint
#define do_nothing	(void) rand()
#else
#define do_nothing	0
#endif

/*
 * Save a binding.
 */
#define SaveB(b) \
	if ((b)->b_AfterGroupRst == NULL || \
	    (b)->b_AfterGroupRst->sv_level != CurrentGroup) \
		DoSave(b); \
	else \
		do_nothing

/*
 * Undo a save.
 */
#define UnSaveB(b) \
	if ((b)->b_AfterGroupRst) \
		DoUnsave(b); \
	else \
		do_nothing

#ifdef StringSizeof
#define InitStrAndLen(str,cstr) (str.s_len = sizeof cstr-1, str.s_str = cstr)
#else
#define InitStrAndLen(str,cstr) (str.s_len = strlen(str.s_str = cstr))
#endif

#define SetBFlags(f)	NewNames[-1].b_flags |= (f)

#define DefIntVar(addr, name) { \
	static struct BoundName _b; \
	InitStrAndLen(_b.b_name, name); \
	_b.b_binding = IntBound; \
	_b.b_bound.b_int = addr; \
	*NewNames++ = &_b; \
}

#define DefDimenVar(addr, name) { \
	static struct BoundName _b; \
	InitStrAndLen(_b.b_name, name); \
	_b.b_binding = DimenBound; \
	_b.b_bound.b_dimen = addr; \
	*NewNames++ = &_b; \
}

#define DefProc(name, proc, flags) { \
	static struct BoundName _b; \
	InitStrAndLen(_b.b_name, name); \
	_b.b_binding = ProcBound; \
	_b.b_bound.b_proc = proc; \
	_b.b_bound.b_flags = flags; \
	*NewNames++ = &_b; \
}
