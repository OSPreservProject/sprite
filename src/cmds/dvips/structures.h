/*
 *   This is dvips, a freely redistributable PostScript driver
 *   for dvi files.  It is (C) Copyright 1986-90 by Tomas Rokicki.
 *   You may modify and use this program to your heart's content,
 *   so long as you send modifications to Tomas Rokicki.  It can
 *   be included in any distribution, commercial or otherwise, so
 *   long as the banner string defined below is not modified (except
 *   for the version number) and this banner is printed on program
 *   invocation, or can be printed on program invocation with the -? option.
 */

/*   This file is the header for dvips's global data structures. */

#define BANNER "This is dvips, version 5.01 (C) 1986-90 Radical Eye Software\n" 
#include <stdio.h>
#if defined(lint) && defined(sun)
extern char *sprintf() ;
#endif
#include "paths.h"
#include "debug.h"
/*
 *   Constants, used to increase or decrease the capacity of this program.
 */
#define STRINGSIZE (15000)  /* maximum number of strings in program */
#define RASTERCHUNK (8192)  /* size of chunk of raster */
#define MINCHUNK (256)      /* minimum size char to get own raster */
#define STACKSIZE (100)     /* maximum stack size for dvi files */
#define MAXFRAME (5)        /* maximum depth of virtual font recursion */
/*
 *   Other constants, which define printer-dependent stuff.
 */
#define SWMEM (180000)      /* available virtual memory in PostScript printer */
#define DPI (actualdpi)     /* dots per inch */
#define FONTCOST (2300)     /* overhead cost of each sw font */
#define PSFONTCOST (11000)   /* overhead cost for PostScript fonts */
#define CHARCOST (50)       /* overhead cost for each character */
#define STRINGCOST (10)     /* the cost of a string */
#define OVERCOST (30000)    /* cost of overhead */
/*
 *   Type declarations.  integer must be a 32-bit signed; shalfword must
 *   be a sixteen-bit signed; halfword must be a sixteen-bit unsigned;
 *   quarterword must be an eight-bit unsigned.
 */
typedef long integer;
typedef char boolean;
typedef double real;
typedef short shalfword ;
typedef unsigned short halfword ;
typedef unsigned char quarterword ;
typedef short Boolean ;
/*
 *   If the machine has a default integer size of 16 bits, and 32-bit
 *   integers must be manipulated with %ld, set the macro SHORTINT.
 */
#ifdef XENIX
#define SHORTINT
#else
#undef SHORTINT
#endif
/*
 *   This is the structure definition for resident fonts.  We use
 *   a small and simple hash table to handle these.  We don't need
 *   a big hash table.
 */
#define RESHASHPRIME (23)
struct resfont {
   char *PSname ;
   char *specialinstructions ;
   struct resfont *next ;
} ;

/*
 *   A chardesc describes an individual character.  Before the fonts are
 *   downloaded, the flags indicate that the character has already been used
 *   with the following meanings:
 */
typedef struct {
   integer TFMwidth ;
   shalfword pixelwidth ;
   quarterword *packptr ;
   quarterword flags ;
} chardesctype ;
#define EXISTS (1)
#define PREVPAGE (2)
#define THISPAGE (4)
#define TOOBIG (8) /* not used at the moment */
#define REPACKED (16)

/*
 *   A fontdesc describes a font.  The name and area point into the string pool.
 */
typedef struct tfd {
   integer checksum, scaledsize, designsize, thinspace ;
   halfword dpi ;
   halfword psname ;
   char loaded ;
   chardesctype chardesc[256] ;
   char *name, *area ;
   struct resfont *resfont ;
   struct tft *localfonts ;
   struct tfd *next ;
} fontdesctype ;

/*  A fontmap associates a fontdesc with a font number.
 */
typedef struct tft {
   integer fontnum ;
   fontdesctype *desc ;
   struct tft *next ;
} fontmaptype ;

/*   Virtual fonts require a `macro' capability that is implemented by
 *   using a stack of `frames'. 
 */
typedef struct {
   quarterword *curp, *curl ;
   fontdesctype *curf ;
   fontmaptype *ff ;
} frametype ;

/*
 *   The next type holds the font usage information in a 256-bit table;
 *   there's a 1 for each character that was used in a section.
 */
typedef struct {
   fontdesctype *fd ;
   halfword bitmap[16] ;
} charusetype ;

/*   Next we want to record the relevant data for a section.  A section is
 *   a largest portion of the document whose font usage does not overflow
 *   the capacity of the printer.  (If a single page does overflow the
 *   capacity all by itself, it is made into its own section and a warning
 *   message is printed; the page is still printed.)
 *
 *   The sections are in a linked list, built during the prescan phase and
 *   processed in proper order (so that pages stack correctly on output) during
 *   the second phase.
 */
typedef struct t {
   integer bos ;
   struct t *next ;
   halfword numpages ;
} sectiontype ;

/*
 *   Sections are actually represented not by sectiontype but by a more
 *   complex data structure of variable size, having the following layout:
 *      sectiontype sect ;
 *      charusetype charuse[numfonts] ;
 *      fontdesctype *sentinel = NULL ;
 *   (Here numfonts is the number of bitmap fonts currently defined.)
 *    Since we can't declare this or take a sizeof it, we build it and
 *   manipulate it ourselves (see the end of the prescan routine).
 */
