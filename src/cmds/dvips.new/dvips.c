/*
 *   This is the main routine.
 */
#ifndef DEFRES
#define DEFRES (300)
#endif

#include "structures.h" /* The copyright notice there is included too! */
#ifdef VMS
#define GLOBAL globaldef
#include climsgdef
#include ctype
#include descrip
#endif
/*
 *   First we define some globals.
 */
#ifdef VMS
    static char ofnme[252],infnme[252],pap[40],thh[20];
#endif
fontdesctype *fonthead ;      /* list of all fonts mentioned so far */
fontdesctype *curfnt ;        /* the currently selected font */
sectiontype *sections ;       /* sections to process document in */
Boolean manualfeed ;          /* manual feed? */
Boolean compressed ;          /* compressed? */
Boolean safetyenclose ;       /* enclose in save/restore for stupid spoolers? */
Boolean removecomments ;      /* remove comments from included PS? */
Boolean nosmallchars ;        /* disable small char optimization for X4045? */
int collatedcopies = 1 ;      /* how many collated copies? */
int sectioncopies = 1 ;       /* how many times to repeat each section? */
shalfword linepos = 0 ;       /* where are we on the line being output? */
integer maxpages ;            /* the maximum number of pages */
Boolean notfirst, notlast ;   /* true if a first page was specified */
Boolean sendcontrolD ;        /* should we send a control D at end? */
integer firstpage ;           /* the number of the first page if specified */
integer lastpage ;
integer firstseq ;
integer lastseq ;
int numcopies ;               /* number of copies of each page to print */
char *oname ;                 /* output file name */
char *iname ;                 /* dvi file name */
char *strings ;               /* strings for program */
char *nextstring, *maxstring ; /* string pointers */
FILE *dvifile, *bitfile ;     /* dvi and output files */
quarterword *curpos ;         /* current position in virtual character packet */
quarterword *curlim ;         /* final byte in virtual character packet */
fontmaptype *ffont ;          /* first font in current frame */
real conv ;                   /* conversion ratio, pixels per DVI unit */
real vconv ;                  /* conversion ratio, pixels per DVI unit */
real alpha ;                  /* conversion ratio, DVI unit per TFM unit */
integer mag ;                 /* the magnification of this document */
Boolean overridemag ;         /* substitute for mag value in DVI file? */
int actualdpi = DEFRES ;      /* the actual resolution of the printer */
int vactualdpi = DEFRES ;      /* the actual resolution of the printer */
int maxdrift ;                /* max pixels away from true rounded position */
int vmaxdrift ;                /* max pixels away from true rounded position */
char *paperfmt ;              /* paper format */
int landscape = 0 ;           /* landscape mode */
integer fontmem ;             /* memory remaining in printer */
integer pagecount ;           /* page counter for the sections */
integer pagenum ;             /* the page number we currently look at */
long bytesleft ;              /* number of bytes left in raster */
quarterword *raster ;         /* area for raster manipulations */
integer hh, vv ;              /* horizontal and vertical pixel positions */
char *tfmpath = TFMPATH ;     /* pointer to directories for tfm files */
char *pkpath = PKPATH ;       /* pointer to directories for pk files */
char *vfpath = VFPATH ;       /* pointer to directories for vf files */
char *figpath = FIGPATH ;     /* pointer to directories for figure files */
char *headerpath = HEADERPATH ; /* pointer to directories for header files */
char *configpath = CONFIGPATH;  /* where to find config files */
#ifdef SEARCH_SUBDIRECTORIES
char *fontsubdirpath = FONTSUBDIRPATH ;
#endif
#ifdef FONTLIB
char *flipath = FLIPATH ;     /* pointer to directories for fli files */
char *fliname = FLINAME ;     /* pointer to names of fli files */
#endif
integer swmem ;               /* font memory in the PostScript printer */
int quiet ;                   /* should we only print errors to stderr? */
int filter ;                  /* act as filter default output to stdout,
                                               default input to stdin? */
int prettycolumn ;            /* the column we are at when running pretty */
int totalpages = 0 ;          /* total number of pages */
Boolean reverse ;             /* are we going reverse? */
Boolean usesPSfonts ;         /* do we use local PostScript fonts? */
Boolean usesspecial ;         /* do we use \special? */
Boolean headers_off ;         /* do we send headers or not? */
char *headerfile ;            /* default header file */
char *warningmsg ;            /* a message to write, if set in config file */
Boolean multiplesects ;       /* more than one section? */
Boolean disablecomments ;     /* should we suppress any EPSF comments? */
char *printer ;               /* what printer to send this to? */
char *mfmode ;                /* default MF mode */
frametype frames[MAXFRAME] ;  /* stack for virtual fonts */
fontdesctype *baseFonts[256] ; /* base fonts for dvi file */
integer pagecost;               /* memory used on the page being prescanned */
int delchar;                    /* characters to delete from prescanned page */
integer fsizetol;               /* max dvi units error for psfile font sizes */
Boolean includesfonts;          /* are fonts used in included psfiles? */
fontdesctype *fonthd[MAXFONTHD];/* list headers for included fonts of 1 name */
int nextfonthd;                 /* next unused fonthd[] index */
char xdig[256];                 /* table for reading hexadecimal digits */
char banner[] = BANNER ;        /* our startup message */
Boolean noenv ;                 /* ignore PRINTER envir variable? */
extern int dontmakefont ;
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
extern int add_header() ;
extern char *strcpy() ;
extern void checkenv() ;
extern void getpsinfo() ;
#ifdef FONTLIB
extern void fliload() ;
#endif
/*
 *   This error routine prints an error message; if the first
 *   character is !, it aborts the job.
 */
static char *progname ;
void
error(s)
	char *s ;
{
   extern void exit() ;

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
   extern char *malloc() ;
   int i;
   char *s;

   nextfonthd = 0;
   for (i=0; i<256; i++)
      xdig[i] = 0;
   i = 0;
   for (s="0123456789ABCDEF"; *s!=0; s++)
      xdig[*s] = i++;
   i = 10;
   for (s="abcdef"; *s!=0; s++)
      xdig[*s] = i++;
   strings = malloc(STRINGSIZE) ;
   if (strings == 0)
      error("! no memory for strings") ;
   maxpages = 100000 ;
   numcopies = 1 ;
   nextstring = strings ;
   iname = strings ;
   *nextstring++ = 0 ;
   maxstring = strings + STRINGSIZE - 200 ;
   bitfile = NULL ;
   bytesleft = 0 ;
   swmem = SWMEM ;
   oname = OUTPATH ;
   sendcontrolD = 0 ;
   multiplesects = 0 ;
   disablecomments = 0 ;
   maxdrift = -1 ;
   vmaxdrift = -1 ;
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
#ifdef VMS
main()
#else
void main(argc, argv)
	int argc ;
	char *argv[] ;
#endif
{
   extern void exit() ;
   int i, lastext = -1 ;
   register sectiontype *sects ;

#ifdef VMS
   progname = &thh[0] ;
   strcpy(progname,"DVIPS%ERROR");
#else
   progname = argv[0] ;
/* we sneak a look at the first arg in case it's debugging */
#ifdef DEBUG
   if (argc > 1 && strncmp(argv[1], "-d", 2)==0) {
      if (sscanf(argv[1]+2, "%d", &debug_flag)==0)
         debug_flag = 0 ;
   }
#endif
#endif
   initialize() ;
   checkenv(0) ;
   getdefaults(CONFIGFILE) ;
   getdefaults((char *)0) ;
/*
 *   This next whole big section of code is straightforward; we just scan
 *   the options.  An argument can either immediately follow its option letter
 *   or be separated by spaces.  Any argument not preceded by '-' and an
 *   option letter is considered a file name; the program complains if more
 *   than one file name is given, and uses stdin if none is given.
 */
#ifdef VMS
vmscli();
#else
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
case 'd' :
#ifdef DEBUG
	    if (*p == 0 && argv[i+1])
	       p = argv[++i];
	    if (sscanf(p, "%d", &debug_flag)==0)
	       error("! Bad debug option (-d).");
	    break;
#else
            error("not compiled in debug mode") ;
            break ;
#endif /* DEBUG */
case 'e' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &maxdrift)==0 || maxdrift<0)
               error("! Bad maxdrift option (-e).") ;
	    vmaxdrift = maxdrift;
            break ;
case 'f' :
            filter = (*p != '0') ;
            if (filter)
               oname = "" ;
            noenv = 1 ;
            break ;
case 'h' : case 'H' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (strcmp(p, "-") == 0)
               headers_off = 1 ;
            else
               (void)add_header(p) ;
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
            noenv = 1 ;
            break ;
case 'p' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            switch(sscanf(p, "%d.%d", &firstpage, &firstseq)) {
#else	/* ~SHORTINT */
            switch(sscanf(p, "%ld.%ld", &firstpage, &firstseq)) {
#endif	/* ~SHORTINT */
case 1:        firstseq = 0 ;
case 2:        break ;
default:
               error("! Bad first page option (-p).") ;
            }
            notfirst = 1 ;
            break ;
case 'l':
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
#ifdef SHORTINT
            switch(sscanf(p, "%d.%d", &lastpage, &lastseq)) {
#else	/* ~SHORTINT */
            switch(sscanf(p, "%ld.%ld", &lastpage, &lastseq)) {
#endif	/* ~SHORTINT */
case 1:        lastseq = 0 ;
case 2:        break ;
default:
               error("! Bad last page option (-p).") ;
            }
            notlast = 1 ;
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
            if (strcmp(p, "landscape") == 0)
               landscape = 1;
            else
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
	    vactualdpi = actualdpi;
            break ;
case 'K' :
            removecomments = (*p != '0') ;
            break ;
case 'U' :
            nosmallchars = (*p != '0') ;
            break ;
case 'X' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &actualdpi)==0 || actualdpi < 10 ||
                       actualdpi > 10000)
               error("! Bad dpi parameter (-D).") ;
            break ;
case 'Y' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            if (sscanf(p, "%d", &vactualdpi)==0 || vactualdpi < 10 ||
                       vactualdpi > 10000)
               error("! Bad dpi parameter (-D).") ;
	    vactualdpi = vactualdpi;
            break ;
case 'F' :
            sendcontrolD = (*p != '0') ;
            break ;
case 'M':
            dontmakefont = 1 ;
            break ;
case 'N' :
            disablecomments = (*p != '0') ;
            break ;
case 'P' :
            if (*p == 0 && argv[i+1])
               p = argv[++i] ;
            printer = p ;
            noenv = 1 ;
            getdefaults("") ;
            break ;
case 'R' :
            reverse = 0 ;
            break ;
case 's' :
            safetyenclose = (*p != '0') ;
            break ;
case 'Z' :
            compressed = (*p != '0') ;
            break ;
case '?' :
            (void)fprintf(stderr, banner) ;
            break ;
default:
            error("! Bad option, not one of cefhlmnopqrtxCDFKNPUXYZ?") ;
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
   if (noenv == 0) {
      register char *p ;
      extern char *getenv() ;
      if (p = getenv("PRINTER")) {
         strcpy(nextstring, "config.") ;
         strcat(nextstring, p) ;
         getdefaults(nextstring) ;
      }
   }
#endif
   checkenv(1) ;
   getpsinfo() ;
   if (!quiet)
      (void)fprintf(stderr, banner) ;
   if (oname[0] == '-' && oname[1] == 0)
      oname[0] = 0 ;
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
   (void)fprintf(stderr,"tfm path %s\npk path %s\n", tfmpath, pkpath) ;
   (void)fprintf(stderr,"fig path %s\nvf path %s\n", figpath, vfpath) ;
   (void)fprintf(stderr,"config path %s\nheader path %s\n", 
                  configpath, headerpath) ;
#ifdef FONTLIB
   (void)fprintf(stderr,"fli path %s\nfli names %s\n", flipath, fliname) ;
#endif
   } /* dd(D_PATHS) */
#endif /* DEBUG */
/*
 *   Now we try to open the dvi file.
 */
   if (warningmsg)
      error(warningmsg) ;
   headerfile = (compressed? CHEADERFILE : HEADERFILE) ;
   (void)add_header(headerfile) ;
   if (*iname != 0)
      dvifile = fopen(iname, READBIN) ;
   else if (filter)
      dvifile = stdin;
   else
      error("! No input filename supplied.") ;
   if (dvifile==NULL)
      error("! DVI file can't be opened.") ;
#ifdef FONTLIB
   fliload();    /* read the font libaries */
#endif
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
   if (vmaxdrift < 0) {
      if (vactualdpi <= 599)
         vmaxdrift = vactualdpi / 100 ;
      else if (vactualdpi < 1199)
         vmaxdrift = vactualdpi / 200 + 3 ;
      else
         vmaxdrift = vactualdpi / 400 + 6 ;
   }
   prescanpages() ;
   if (includesfonts)
      (void)add_header(IFONTHEADER) ;
   if (usesPSfonts)
      (void)add_header(PSFONTHEADER) ;
   if (usesspecial)
      (void)add_header(SPECIALHEADER) ;
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
         if (! quiet) {
            if (prettycolumn > 77) {
               fprintf(stderr, "\n") ;
               prettycolumn = 0 ;
            }
            (void)fprintf(stderr, ". ") ;
            prettycolumn += 2 ;
         }
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
#ifdef VMS
#include "vms/vmscli.c"
#endif
