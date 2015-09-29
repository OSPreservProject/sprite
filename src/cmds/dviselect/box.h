/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/* Box structures */

/* Magic constants.  Note that these are stored into `scaled' types. */
#define RunningRule (-(1 << 30))
#define MFracDefaultThickness (1 << 31)	/* => default rule thickness */

/* Magic penalty values */
#define InfPenalty 10000	/* infinity---no break at all */
#define EjectPenalty (-10000)	/* negative infinity---forced break */

/* Ordinary list types */
#define CharNode	0	/* character; also math char */
#define HListNode	1	/* a horizontal list */
#define VListNode	2	/* a vertical list */
#define RuleNode	3	/* a rule */
#define InsertNode	4	/* an insertion */
#define MarkNode	5	/* a mark */
#define AdjustNode	6	/* a \vadjust */
#define LigatureNode	7	/* a ligature */
#define DiscretionaryNode 8	/* a discretionary break */
#define WhatsitNode	9	/* a "whatsit" */
#define BeginMathNode	10	/* a begin-math */
#define EndMathNode	11	/* an end-math */
#define GlueNode	12	/* link to glue */
#define KernNode	13	/* kerning */
#define PenaltyNode	14	/* penalty */
#define UnsetAlignNode	15	/* \halign or \valign that is incomplete */

/* Types arising only in math lists */
#define MathAtom	16	/* math atom */
#define MathStyle	17	/* math style change */
#define MathFraction	18	/* math fraction */
#define MathLeft	19	/* math left delimiter \left */
#define MathRight	20	/* math right delimiter \right */
#define MathChoice	21	/* math 4-way choice */
#define MathSubList	22	/* math list, possibly empty */
#define MathSubBox	23	/* math box (\hlist or \vlist) */

/* Kinds of math atoms */
enum MKind {
	MKindOrd, MKindOp, MKindOpLimits, MKindOpNoLimits, MKindBin, MKindRel,
	MKindOpen, MKindClose, MKindPunct, MKindInner, MKindOver, MKindUnder,
	MKindAcc, MKindRad, MKindVCent
};

/* Math text styles */
#define MStyleCramped	1	/* added to others to get cramped style */
#define MStyleDisplay	0	/* display style */
#define MStyleText	2	/* text style */
#define MStyleScript	4	/* script style */
#define MStyleSScript	6	/* scriptscript style */

/*
 * A font and character.  Note that these limit the number of fonts and
 * characters/font to 256.
 */
struct fontchar {
	char	fc_font;		/* font index */
	char	fc_char;		/* character index within the font */
};

/* a box */
struct box {
	scaled	bx_width;		/* width of the box */
	scaled	bx_depth;		/* depth of the box */
	scaled	bx_height;		/* height of the box */
	scaled	bx_offset;		/* offset (lower if h, right if v) */
	struct	node *bx_contents;	/* contents of the box */
	int	bx_glueorder;		/* -3=>minus hfilll; 3=>plus hfilll */
	float	bx_glueset;		/* glue setting ratio */
};

/* a rule */
struct rule {
	scaled	ru_width;		/* width of the rule */
	scaled	ru_depth;		/* depth of the rule */
	scaled	ru_height;		/* height of the rule */
};

/* an insert */
struct insert {
	int	ins_number;		/* 0..254 (I hope) */
	i32	ins_floatingpenalty;	/* \floatingpenalty for this insert */
	scaled	ins_naturalheightplusdepth;	/* what more can I say? */
	scaled	ins_splitmaxdepth;	/* \splitmaxdepth for this insert */
	struct	node *ins_splittopskip;	/* \splittopskip for this insert */
	struct	node *ins_contents;	/* insertion contents */
};

/* a ligature */
struct ligature {
	struct	fontchar lig_fc;	/* the ligature character */
	struct	node *lig_orig;		/* the original set of characters */
};

/* a discretionary break */
struct discret {
	struct	node *disc_prebrk;	/* characters to precede break */
	struct	node *disc_postbrk;	/* characters to follow break */
	int	disc_replaces;		/* replaces this many nodes */
};

/* a whatsit */
struct whatsit {
	int	wh_type;		/* type of whatsit */
	char	*wh_other;		/* other stuff */
};

/*
 * A glue specification.  There may be multiple pointers to this glue spec,
 * so we have a reference count attached.
 */
struct gluespec {
	int	gl_refcount;		/* # pointers to here */
	scaled	gl_width;		/* normal width */
	scaled	gl_stretch;		/* strechability */
	scaled	gl_shrink;		/* shrinkability */
	short	gl_stretchorder;	/* 0..3 for <>, fil, fill, filll */
	short	gl_shrinkorder;		/* same */
};

/* a link-to-glue node */
#define GlueIsNormal	0		/* ordinary glue */
#define GlueIsSkip	1		/* \hskip or \vskip */
#define GlueIsNonScript	2		/* \nonscript */
#define GlueIsMuGlue	3		/* mu glue */
#define GlueIsALeaders	4		/* aligned leaders */
#define GlueIsCLeaders	5		/* centered leaders */
#define GlueIsXLeaders	6		/* expanded leaders */
struct gluenode {
	int	gn_type;		/* special glue types */
	struct	node *gn_glue;		/* the glue itself */
	struct	BoundName *gn_skip;	/* skip parameter that generated it */
	struct	box *gn_leaderbox;	/* leader box if leaders */
};

/* a kern */
#define ImplicitKern	0		/* kerning for things like "AV" */
#define ExplicitKern	1		/* \kern or italic correction */
#define MKern		2		/* \mkern */
struct kern {
	int	kern_type;		/* type of kern */
	scaled	kern_width;		/* width of kern */
};

/*
 * An unset (incomplete) alignment (\halign or \valign).
 * This looks to be the largest node type, so I am using 'char' and 'short'
 * to try to keep it small...
 */
struct unsetalign {
	scaled	unset_width;		/* width of the unset box */
	scaled	unset_depth;		/* depth of the unset box */
	scaled	unset_height;		/* height of the unset box */
	struct	node *unset_contents;	/* contents of the unset box */
	scaled	unset_glueshrink;	/* total glue shrink */
	scaled	unset_gluestretch;	/* total glue stretch */
	char	unset_shrinkorder;	/* glue shrink order */
	char	unset_stretchorder;	/* glue stretch order */
	short	unset_spancount;	/* the number of spanned columns */
};

/* a token list */
struct toklist {
	int	tok_refcount;		/* the number of references */
	struct	node *tok_list;		/* the tokens or the macro def'n */
};

/* Math delimiter */
struct delim {
	char	smallfam;		/* family number for small char */
	char	smallchar;		/* character in small family */
	char	largefam;		/* family number for large char */
	char	largechar;		/* character in large family */
};

/* Math atoms, radicals, \left s, \right s, accents */
struct matom {
	enum	MKind kind;		/* atom kind (if not \left, \right) */
	struct	node *nucleus;		/* nucleus */
	struct	node *supscr;		/* superscript (optional) */
	struct	node *subscr;		/* subscript (optional) */
	struct	delim delimiter;	/* delimiter(s) (optional) */
	/* accents are stored in the small delim */
};

/* Math fraction */
struct mfrac {
	struct	node *numerator;	/* numerator */
	struct	node *denominator;	/* denominator */
	scaled	thickness;		/* dividing rule thickness */
	struct	delim leftdelim;	/* left delimiter (optional) */
	struct	delim rightdelim;	/* right delimiter (optional) */
};

/* Math choice */
struct mchoice {
	struct	node *ifdisplay;
	struct	node *iftext;
	struct	node *ifscript;
	struct	node *ifsscript;
};

/*
 * This structure exists mainly for `sizeof'.  Character nodes are by far the
 * most common nodes, so to save memory space, nodes of type CharNode are kept
 * in a separate free list and do not get a full size node.  This means that
 * node lists must be handled carefully, but that is already necessary, so it
 * is no loss.
 */
struct charnode {
	struct	node *next;
	short	type;
	struct	fontchar fc;
};

/*
 * A node in a list.  Note that the first part of this structure is shared
 * with the struct charnode.
 */
struct node {
	struct	node *next;		/* next in linked list */
	short	type;			/* type of this node */
	struct	fontchar fc;		/* value iff char */
	union {
		struct	box un_box;		/* value iff box */
		struct	rule un_rule;		/* value iff rule */
		struct	insert un_ins;		/* value iff insert */
		struct	toklist *un_mark;	/* value iff mark */
		struct	ligature un_lig;	/* value iff ligature */
		struct	discret un_disc;	/* value iff discret. brk */
		struct	whatsit un_whatsit;	/* value iff whatsit */
		struct	gluespec un_gluespec;	/* value iff glue */
		struct	gluenode un_gluenode;	/* value iff link to glue */
		struct	kern un_kern;		/* value iff kern */
		i32	un_integer;		/* value if  integer */
		struct	unsetalign un_unset;	/* value iff unset alignment */
		struct	matom un_matom;		/* value if  math atom */
		struct	mfrac un_mfrac;		/* value iff math fraction */
		struct	mchoice un_mchoice;	/* value iff math choice */
		struct	node *un_node;		/* value if  node list */
	} node_un;
};

/* Shorthand */
#define FC   fc
#define Font fc.fc_font
#define Char fc.fc_char

#define BoxWidth     node_un.un_box.bx_width
#define BoxDepth     node_un.un_box.bx_depth
#define BoxHeight    node_un.un_box.bx_height
#define BoxOffset    node_un.un_box.bx_offset
#define BoxContents  node_un.un_box.bx_contents
#define BoxGlueOrder node_un.un_box.bx_glueorder
#define BoxGlueSet   node_un.un_box.bx_glueset

#define RuleWidth  node_un.un_rule.ru_width
#define RuleDepth  node_un.un_rule.ru_depth
#define RuleHeight node_un.un_rule.ru_height

#define InsertNumber                 node_un.un_ins.ins_number
#define InsertFloatingPenalty        node_un.un_ins.ins_floatingpenalty
#define InsertNaturalHeightPlusDepth node_un.un_ins.ins_naturalheightplusdepth
#define InsertSplitMaxDepth          node_un.un_ins.ins_splitmaxdepth
#define InsertSplitTopSkip           node_un.un_ins.ins_splittopskip
#define InsertContents               node_un.un_ins.ins_contents

#define Mark node_un.un_mark

#define LigatureFC   node_un.un_lig.lig_fc
#define LigatureOrig node_un.un_lig.lig_orig

#define DiscretPreBreak  node_un.un_disc.disc_prebrk
#define DiscretPostBreak node_un.un_disc.disc_postbrk
#define DiscretReplaces  node_un.un_disc.disc_replaces

#define WhatsitType  node_un.un_whatsit.wh_type
#define WhatsitOther node_un.un_whatsit.wh_other

#define GlueRefCount     node_un.un_gluespec.gl_refcount
#define GlueWidth        node_un.un_gluespec.gl_width
#define GlueStretch      node_un.un_gluespec.gl_stretch
#define GlueShrink       node_un.un_gluespec.gl_shrink
#define GlueStretchOrder node_un.un_gluespec.gl_stretchorder
#define GlueShrinkOrder  node_un.un_gluespec.gl_shrinkorder

#define Glue          node_un.un_gluenode.gn_glue
#define GlueSkip      node_un.un_gluenode.gn_skip
#define GlueType      node_un.un_gluenode.gn_type
#define GlueLeaderBox node_un.un_gluenode.gn_leaderbox

#define KernWidth node_un.un_kern.kern_width
#define KernType  node_un.un_kern.kern_type

#define Penalty node_un.un_integer

#define UnsetWidth        node_un.un_unset.unset_width
#define UnsetDepth        node_un.un_unset.unset_depth
#define UnsetHeight       node_un.un_unset.unset_height
#define UnsetContents     node_un.un_unset.unset_contents
#define UnsetGlueShrink   node_un.un_unset.unset_glueshrink
#define UnsetGlueStretch  node_un.un_unset.unset_gluestretch
#define UnsetShrinkOrder  node_un.un_unset.unset_shrinkorder
#define UnsetStretchOrder node_un.un_unset.unset_stretchorder
#define UnsetSpanCount    node_un.un_unset.unset_spancount

#define MAtomKind      node_un.un_matom.kind
#define MAtomNucleus   node_un.un_matom.nucleus
#define MAtomSupscr    node_un.un_matom.supscr
#define MAtomSubscr    node_un.un_matom.subscr
#define MAtomDelimiter node_un.un_matom.delimiter
#define MAtomAccFam    node_un.un_matom.delimiter.smallfam
#define MAtomAccChar   node_un.un_matom.delimiter.smallchar

#define MFracNumerator   node_un.un_mfrac.numerator
#define MFracDenominator node_un.un_mfrac.denominator
#define MFracThickness   node_un.un_mfrac.thickness
#define MFracLeftDelim   node_un.un_mfrac.leftdelim
#define MFracRightDelim  node_un.un_mfrac.rightdelim

#define MChoiceDisplay node_un.un_mchoice.ifdisplay
#define MChoiceText    node_un.un_mchoice.iftext
#define MChoiceScript  node_un.un_mchoice.ifscript
#define MChoiceSScript node_un.un_mchoice.ifsscript

#define MStyle node_un.un_integer

#define MSub node_un.un_node

struct	node *FreeNodes;
struct	node *FreeChars;

struct	node *MoreNodes();
struct	node *MoreChars();

/*
 * These macros quickly allocate and dispose of nodes.  Watch out; the
 * arguments to NewNode and NewCharNode should be pointers that are to
 * be filled in.  Moreover, the argument is accessed twice, so beware
 * side effects!
 */
#define NewNode(n) \
	if (((n) = FreeNodes) == NULL) \
		(n) = MoreNodes(); \
	else \
		FreeNodes = (n)->next

#define NewCharNode(n) \
	if (((n) = FreeChars) == NULL) \
		(n) = MoreChars(); \
	else \
		FreeChars = (n)->next

#define FreeNode(n)	((n)->next = FreeNodes, FreeNodes = (n))
#define FreeChar(n)	((n)->next = FreeChars, FreeChars = (n))

/* To free either a CharNode or a Node, use FreeEither! */
#define FreeEither(n)	((n)->type ? FreeNode(n) : FreeChar(n))

#define DelTokRef(p)	(--(p)->tok_refcount < 0 ? FlushList(p) : NULL)
#define DelGlueRef(p)	(--(p)->GlueRefCount < 0 ? FreeNode(p) : NULL)

/* These determine the granularity of node allocation */
#define NodeAllocSize	300	/* Nodes per MoreNodes() */
#define CharAllocSize	2500	/* CharNodes per MoreChars() */

struct	node *CopyGlueSpec();
struct	node *LinkToSkipParam();
struct	node *LinkToGlueSpec();
struct	node *LinkToCopyOfSkipParam();

#define NodeListDisplay(p) (s_addc('.'), ShowNodeList(p), s_delc())

/*
 * The ordering of the types is important here, because it allows simple
 * decisions for common operations.
 */
#define CanBreakAtGlue(n)	((n)->type <  BeginMathNode)
#define CanDiscardAfterBreak(n) ((n)->type >= BeginMathNode)

struct	node ZeroGlue;		/* the zero-amount glue (0pt) */
struct	node FilGlue;		/* 0pt plus1fil minus0pt */
struct	node FillGlue;		/* 0pt plus1fill minus0pt */
struct	node SSGlue;		/* 0pt plus1fil minus1fil */
struct	node FilNegGlue;	/* 0pt plus-1fil minus0pt */
i32	ShowBoxMaxDepth;	/* max nesting depth in box displays */
i32	ShowBoxMaxBreadth;	/* max number of items per list level */
int	FontInShortDisplay;	/* the "current font" during a ShortDisplay */
