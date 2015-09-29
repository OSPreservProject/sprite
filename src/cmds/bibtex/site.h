/*
 * Master configuration file for WEB to C.
 *
 * Tim Morgan  2/13/88
 *	Last updated 10/25/88
 */

/*
 * Default editor command string: %d expands to the line number where
 * TeX or Metafont found an error and %s expands to the name of the file.
 */
#define	EDITOR		"/sprite/cmds/vi +%d %s"

/*
 * If the type "int" is at least 32 bits (including a sign bit), this
 * symbol should be #undef'd; otherwise, it should be #define'd.  If
 * your compiler uses 16-bit int's, arrays larger than 32kb may give
 * you problems, especially if indices are automatically cast to int's.
 */
#undef	SIXTEENBIT

/*
 * Our character set is 7-bit ASCII unless NONASCII is defined.
 * For other character sets, make sure that first_text_char and
 * last_text_char are defined correctly in TeX (they're 0 and 127,
 * respectively, by default).  In the file tex.defines, change
 * the indicated range of type "char" to be the same as
 * first_text_char..last_text_char, re-tangle, and "#define NONASCII".
 */
#undef	NONASCII

/* Define if we're running under System V */
#undef	SYSV

/* Define if we're running under 4.2 or 4.3 BSD */
#define	BSD

/*
 * Define this if web2c will be compiled with an ANSI-C compiler.
 * It also changes web2c so that by default it produces ANSI-C as
 * its output.
 */
#undef	ANSI

/* Define this if web2c will be running under DOS with Microsoft C */
#undef	MS_DOS

/*
 * The type "schar" should be defined here to be the smallest signed
 * type available.  ANSI C compilers may need to use "signed char".
 * If you don't have signed characters, then define schar to be the
 * type "short".
 */
typedef	char schar;

/*
 * The type "integer" must be a signed integer capable of holding at
 * least the range of numbers (-2^31)..(2^32-1).
 * The ANSI draft C standard says that "long" meets this requirement.
 */
typedef long integer;

/* Boolean can by any convenient type */
typedef char boolean;

/*
 * The type glueratio should be a floating-point type which won't
 * unnecessarily increase the size of the memoryword structure.
 * This is the basic requirement.  On most machines, if you're
 * building a normal-sized TeX, then glueratio must probably meet
 * the following restriction: sizeof(glueratio) <= sizeof(integer).
 * Usually, then, glueratio must be of type "float".  But
 * if you build a BIG TeX, you can (on most machines) and should
 * make it "double" to avoid loss of precision and conversions
 * to and from double during calculations.
 */
typedef double glueratio;

/*
 * Type "real" can by any convenient floating-point type; usually double
 * is best since it gives more precision and most C compilers convert
 * float to double anyway.
 */
typedef double real;

/*
 * Define MAXPATHLENGTH to be the maximum number of characters in a
 * search path.  This is used to size the buffers for TEXINPUTS,
 * MFBASES, etc.
 */
#define	MAXPATHLENGTH	1024

/*
 * TeX search paths: This is what we use at UCI, which probably won't be
 * what you use.
 */
#define	TEXINPUTS	".:/sprite/lib/tex"
#define	TEXFONTS	".:/sprite/lib/fonts/tfm"
#define	TEXPOOL		".:/sprite/lib/tex"
#define	TEXFORMATS	".:/sprite/lib/tex"

/* Metafont search paths */
#define	MFINPUTS	".:/usr/local/lib/mf84"
#define	MFBASES		".:/usr/local/lib/mf84/bases"
#define	MFPOOL		".:/usr/local/lib/mf84"

/*
 * BibTeX search path for .bib files.
 * NB: TEXINPUTS is used by BibTeX to search for .bst files.
 */
#define	BIBINPUTS	"."

/*
 * Metafont Window Support: More than one may be defined, as long
 * as you don't try to have X10 and X11 support in the same binary
 * (because there are conflicting routine names in the libraries).
 * If you define one or more of these windowing systems, BE SURE
 * you update the top-level Makefile accordingly.
 */
#undef	SUNWIN			/* SunWindows support */
#undef	X10WIN			/* X Version 10 support */
#undef	X11WIN			/* X Version 11 support */
#undef	HP2627WIN		/* HP 2627 support */
#undef	TEKTRONIXWIN		/* Tektronix 4014 support */

/* NB: You can't define X10WIN and X11WIN simultaneously */
#if defined(X10WIN) && defined(X11WIN)
syntax error
#endif

/*
 * The maximum length of a filename including a directory specifier.
 * This value is also defined in the change files for tex, mf, and bibtex,
 * so it appears in the relevant .h as "filenamesize".  It can be safely
 * changed in these files if necessary, although you should really edit
 * the files tex/ctex.ch and mf/cmf.ch.  Most sites should not have to
 * change this value anyway, since it doesn't hurt much if it's too large.
 * If you change it here, then change it in both the changefiles to match!
 */
#define	FILENAMESIZE	1024

/*
 * Define the variable REGFIX if you want TeX to be compiled with local
 * variables declared as "register".  On SunOS 3.2 and 3.4, and possibly
 * with other compilers, this will cause problems, so use it only if
 * the trip test is passed when it's turned on.
 */
#undef	REGFIX

/*
 * Define VAX4_2 if you are running 4.2 or 4.3 BSD on a VAX.  This will
 * cause Metafont to use some assembly language routines in place of
 * some generic C routines.
 */
#undef	VAX4_2
