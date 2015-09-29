/*
 *   This is the main routine.
 */
#ifndef DEFRES
#define DEFRES (300)
#endif
#ifndef DEFPFMT
#define DEFPFMT "letter"
#endif

#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   First we define some globals.
 */
fontdesctype *fonthead ;      /* list of all fonts mentioned so far */
fontdesctype *curfnt ;        /* the currently selected font */
sectiontype *sections ;       /* sections to process document in */
Boolean manualfeed ;          /* manual feed? */
Boolean compressed ;          /* compressed? */
int collatedcopies = 1 ;      /* how many collated copies? */
int sectioncopies = 1 ;       /* how many times to repeat each section? */
shalfword linepos = 0 ;       /* where are we on the line being output? */
integer maxpages ;            /* the maximum number of pages */
Boolean notfirst ;            /* true if a first page was specified */
Boolean sendcontrolD ;        /* should we send a control D at end? */
integer firstpage ;           /* the number of the first page if specified */
int numcopies ;               /* number of copies of each page to print */
char *oname ;                 /* output file name */
char *iname ;                 /* dvi file name */
char strings[STRINGSIZE] ;    /* strings for program */
char *nextstring, *maxstring ; /* string pointers */
FILE *dvifile, *bitfile ;     /* dvi and output files */
quarterword *curpos ;         /* current position in virtual character packet */
quarterword *curlim ;         /* final byte in virtual character packet */
fontmaptype *ffont ;          /* first font in current frame */
real conv ;                   /* conversion ratio, pixels per DVI unit */
real alpha ;                  /* conversion ratio, DVI unit per TFM unit */
integer mag ;                 /* the magnification of this document */
Boolean overridemag ;         /* substitute for mag value in DVI file? */
int actualdpi = DEFRES ;      /* the actual resolution of the printer */
int maxdrift ;                /* max pixels away from true rounded position */
char *paperfmt = DEFPFMT ;    /* paper format */
integer fontmem ;             /* memory remaining in printer */
integer pagecount ;           /* page counter for the sections */
integer pagenum ;             /* the page number we currently look at */
long bytesleft ;              /* number of bytes left in raster */
quarterword *raster ;         /* area for raster manipulations */
integer hh, vv ;              /* horizontal and vertical pixel positions */
char *tfmpath ;               /* pointer to directories for tfm files */
char *pkpath ;                /* pointer to directories for pk files */
char *vfpath ;                /* pointer to directories for vf files */
char *figpath ;               /* pointer to directories for figure files */
integer swmem ;               /* font memory in the PostScript printer */
int quiet ;                   /* should we only print errors to stderr? */
int filter ;                  /* act as filter default output to stdout,
                                               default input to stdin? */
int totalpages = 0 ;          /* total number of pages */
Boolean reverse ;             /* are we going reverse? */
Boolean usesPSfonts ;         /* do we use local PostScript fonts? */
Boolean usesspecial ;         /* do we use \special? */
Boolean headers_off ;         /* do we send headers or not? */
char *headerfile ;            /* default header file */
Boolean multiplesects ;       /* more than one section? */
Boolean disablecomments ;     /* should we suppress any EPSF comments? */
char *printer ;               /* what printer to send this to? */
char *mfmode ;                /* default MF mode */
frametype frames[MAXFRAME] ;  /* stack for virtual fonts */

#ifdef DEBUG
integer debug_flag = 0;
#endif /* DEBUG */
/*
 *   This routine calls the following externals:
 */
extern void outbangspecials() ;
extern void prescanpages() ;
extern void initprinter() ;
extern void cleanprinter() ;
extern void dosection() ;
extern void getdefaults() ;
extern void cmdout() ;
extern void numout() ;
extern void add_header() ;
extern char *strcpy() ;
/*
 *   This error routine prints an error message; if the first
 *   character is !, it aborts the job.
 */
static char *progname ;
void
error(s)
	char *s ;
{
   (void)fprintf(stderr, "%s: %s\n", progname, s) ;
   if (*s=='!') {
      if (bitfile != NULL) {
         cleanprinter() ;
      }
      exit(1) ; /* fatal */
   }
}

/*
 *   Initialize sets up all the globals and data structures.
 */
void
initialize()
{
   fonthead = NULL ;
   sections = NULL ;
   maxpages = 100000 ;
   firstpage = 0 ;
   notfirst = 0 ;
   overridemag = 0 ;
   numcopies = 1 ;
   nextstring = strings ;
   iname = strings ;
   *nextstring++ = 0 ;
   maxstring = strings + STRINGSIZE - 200 ;
   bitfile = NULL ;
   bytesleft = 0 ;
   pkpath = PKPATH ;
   vfpath = VFPATH ;
   tfmpath = TFMPATH ;
   figpath = FIGPATH ;
   swmem = SWMEM ;
   oname = OUTPATH ;
   sendcontrolD = 0 ;
   multiplesects = 0 ;
   disablecomments = 0 ;
   maxdrift = -1 ;
}
/*
 *   This routine copies a string into the string `pool', safely.
 */
char *
newstring(s)
   char *s ;
{
   int l ;

   if (s == NULL)
      return(NULL) ;
   l = strlen(s) ;
   if (nextstring + l >= maxstring)
      error("! out of string space") ;
   (void)strcpy(nextstring, s) ;
   s = nextstring ;
   nextstring += l + 1 ;
   return(s) ;
}
/*
 *   Finally, our main routine.
 */
main(argc, argv)
	int argc ;
	char *argv[] ;
{
   int i, lastext = -1 ;
   register sectiontype *sects ;
   char *getenv();

   progname = argv[0] ;
   initialize() ;

   printer = getenv("PRINTER");
   getdefaults("ps") ;
   if(printer)
      getdefaults(printer);

/*
 *   This next whole big section of code is straightforward; we just scan
 *   the options.  An argument can either immediately follow its option letter
 *   or be separated by spaces.  Any argument not preceded by '-' and an
 *   option letter is considered a file name; the program complains if more
 *   than one file name is given, and uses stdin if none is given.
 */
   for (i=1; i<argc; i++) {
      if (*argv[i]=='-') {
         char *p=argv[i]+2 ;
         char c=argv[i][1] ;
         switch (c) {
case 'c' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &numcopies)==0)
               error("! Bad number of copies option (-c).") ;
            break ;
#ifdef DEBUG
case 'd' :
	    if (*p == 0 && argv[i+1])
	       p = argv[++i];
	    if (sscanf(p, "%d", &debug_flag)==0)
	       error("! Bad debug option (-d).");
	    break;
#endif /* DEBUG */
case 'e' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &maxdrift)==0 || maxdrift<0)
               error("! Bad maxdrift option (-e).") ;
            break ;
case 'f' :
            filter = (*p != '0') ;
            if (filter)
               oname = "" ;
            break ;
case 'h' : case 'H' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (strcmp(p, "-") == 0)
               headers_off = 1 ;
            else
               add_header(p) ;
            break ;
case 'm' :
            manualfeed = (*p != '0') ;
            break ;
case 'n' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            if (sscanf(p, "%ld", &maxpages)==0)
#else	/* ~SHORTINT */
            if (sscanf(p, "%d", &maxpages)==0)
#endif	/* ~SHORTINT */
               error("! Bad number of pages option (-n).") ;
            break ;
case 'o' : case 'O' :
            if (*p == 0 && argv[i+1] && *argv[i+1]!='-')
               p = argv[++i] ;
            oname = p ;
            break ;
case 'p' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            if (sscanf(p, "%ld", &firstpage)==0)
#else	/* ~SHORTINT */
            if (sscanf(p, "%d", &firstpage)==0)
#endif	/* ~SHORTINT */
               error("! Bad first page option (-p).") ;
            notfirst = 1 ;
            break ;
case 'q' : case 'Q' :
            quiet = (*p != '0') ;
            break ;
case 'r' :
            reverse = (*p != '0') ;
            break ;
case 't' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            paperfmt = p ;
            break ;
case 'x' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &mag)==0 || mag < 10 ||
                       mag > 100000)
               error("! Bad magnification parameter (-x).") ;
            overridemag = 1 ;
            break ;
case 'C' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &collatedcopies)==0)
               error("! Bad number of collated copies option (-C).") ;
            break ;
case 'D' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &actualdpi)==0 || actualdpi < 10 ||
                       actualdpi > 10000)
               error("! Bad dpi parameter (-D).") ;
            break ;
case 'F' :
            sendcontrolD = (*p != '0') ;
            break ;
case 'N' :
            disablecomments = (*p != '0') ;
            break ;
case 'P' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            printer = p ;
	    getdefaults("ps") ;
	    getdefaults(printer);
            break ;
case 'R' :
            reverse = 0 ;
            break ;
case 'Z' :
            compressed = (*p != '0') ;
            break ;
case '?' :
            (void)fprintf(stderr, BANNER) ;
            break ;
default:
            error("! Bad option, not one of cefhmnopqrtxCDFNPZ?") ;
         }
      } else {
         if (*iname == 0) {
            register char *p ;

            lastext = 0 ;
            iname = nextstring ;
            p = argv[i] ;
            while (*p) {
               *nextstring = *p++ ;
               if (*nextstring == '.')
                  lastext = nextstring - iname ;
               else if (*nextstring == '/' || *nextstring == ':')
                  lastext = 0 ;
               nextstring++ ;
            }
            if (lastext == 0) {
               lastext = nextstring - iname ;
               *nextstring++ = '.' ;
               *nextstring++ = 'd' ;
               *nextstring++ = 'v' ;
               *nextstring++ = 'i' ;
            }
            *nextstring++ = 0 ;
         } else
            error("! Two input file names specified.") ;
      }
   }

   if (!quiet)
      (void)fprintf(stderr, BANNER) ;
   if (*oname == 0 && ! filter) {
      oname = nextstring ;
      for (i=0; i<=lastext; i++)
         *nextstring++ = iname[i] ;
      *nextstring++ = 'p' ;
      *nextstring++ = 's' ;
      *nextstring++ = 0 ;
   }
#ifdef DEBUG
   if (dd(D_PATHS)) {
#ifdef SHORTINT
	(void)fprintf(stderr,"input file %s output file %s swmem %ld\n",
#else /* ~SHORTINT */
   	(void)fprintf(stderr,"input file %s output file %s swmem %d\n",
#endif /* ~SHORTINT */
           iname, oname, swmem) ;
   (void)fprintf(stderr,"tfm path %s pk path %s\n", tfmpath, pkpath) ;
   } /* dd(D_PATHS) */
#endif /* DEBUG */
/*
 *   Now we try to open the dvi file.
 */
   headerfile = (compressed? CHEADERFILE : HEADERFILE) ;
   add_header(headerfile) ;
   if (*iname != 0)
      dvifile = fopen(iname, "r") ;
   else if (filter)
      dvifile = stdin;
   else
      error("! No input filename supplied.") ;
   if (dvifile==NULL)
      error("! DVI file can't be opened.") ;
/*
 *   Now we do our main work.
 */
   if (maxdrift < 0) {
      if (actualdpi <= 599)
         maxdrift = actualdpi / 100 ;
      else if (actualdpi < 1199)
         maxdrift = actualdpi / 200 + 3 ;
      else
         maxdrift = actualdpi / 400 + 6 ;
   }
   prescanpages() ;
   if (usesPSfonts)
      add_header(PSFONTHEADER) ;
   if (usesspecial)
      add_header(SPECIALHEADER) ;
   sects = sections ;
   if (sects == NULL || sects->next == NULL) {
      sectioncopies = collatedcopies ;
      collatedcopies = 1 ;
   } else {
      totalpages *= collatedcopies ;
      multiplesects = 1 ;
   }
   initprinter() ;
   outbangspecials() ;
   for (i=0; i<collatedcopies; i++) {
      sects = sections ;
      while (sects != NULL) {
         if (! quiet)
            (void)fprintf(stderr, ". ") ;
         (void)fflush(stderr) ;
         dosection(sects, sectioncopies) ;
         sects = sects->next ;
      }
   }
   cleanprinter() ;
   if (! quiet)
      (void)fprintf(stderr, "\n") ;
   exit(0) ;
   /*NOTREACHED*/
}
