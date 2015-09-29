/*
 *   This routine handles special commands;
 *   predospecial() is for the prescan, dospecial() for the real thing.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif

/*
 *   These are the external routines called:
 */
/**/
#ifdef TPIC
extern void setPenSize();
extern void flushPath();
extern void flushDashed();
extern void flushDashed();
extern void addPath();
extern void arc();
extern void flushSpline();
extern void shadeLast();
extern void whitenLast();
extern void blackenLast();
#endif
extern shalfword dvibyte() ;
extern void add_header() ;
extern void hvpos() ;
extern void figcopyfile() ;
extern char *malloc() ;
extern void cmdout() ;
extern void numout() ;
extern void scout() ;
extern void stringend() ;
extern void error() ;
extern char errbuf[] ;
extern shalfword linepos;
extern Boolean usesspecial ;
extern char *paperfmt ;
extern char *nextstring;
extern char *maxstring;
extern char *oname;
extern FILE *bitfile;
extern int quiet;
extern fontdesctype *curfnt ;
extern int actualdpi ;
extern real conv ;
#ifdef DEBUG
extern integer debug_flag;
#endif

struct bangspecial {
   struct bangspecial *next ;
   char actualstuff[1] ; /* more space will actually be allocated */
} *bangspecials = NULL ;

static trytobreakout(p)
register char *p ;
{
   register int i ;
   register int instring = 0 ;
   int lastc = 0 ;

   i = 0 ;
   while (*p) {
      if (i > 65 && *p == ' ' && instring == 0) {
         (void)putc('\n', bitfile) ;
         i = 0 ;
      } else {
         (void)putc(*p, bitfile) ;
         i++ ;
      }
      if (*p == '(' && lastc != '\\')
         instring = 1 ;
      else if (*p == ')' && lastc != '\\')
         instring = 0 ;
      lastc = *p ;
      p++ ;
   }
}

static dobs(q)
register struct bangspecial *q ;
{
   if (q) {
      dobs(q->next) ;
      trytobreakout(q->actualstuff) ;
   }
}

void
outbangspecials() {
   if (bangspecials) {
      cmdout("TeXDict") ;
      cmdout("begin") ;
      cmdout("@defspecial\n") ;
      dobs(bangspecials) ;
      cmdout("\n@fedspecial") ;
      cmdout("end") ;
   }
}

/* We recommend that new specials be handled by the following general
 * (and extensible) scheme, in which the user specifies one or more
 * `key=value' pairs separated by spaces.
 * The known keys are given in KeyTab; they take values
 * of one of the following types:
 *
 * None: no value, just a keyword (in which case the = sign is omitted)
 * String: the value should be "<string without double-quotes"
 *                          or '<string without single-quotes'
 * Integer: the value should be a decimal integer (%d format)
 * Number: the value should be a decimal integer or real (%f format)
 * Dimension: like Number, but will be multiplied by the scaledsize
 *       of the current font and converted to default PostScript units
 * (Actually, strings are allowed in all cases; the delimiting quotes
 *  are simply stripped off if present.)
 *
 */

typedef enum {None, String, Integer, Number, Dimension} ValTyp;
typedef struct {
   char    *Entry;
   ValTyp  Type;
} KeyDesc;

#define NKEYS    (sizeof(KeyTab)/sizeof(KeyTab[0]))

KeyDesc KeyTab[] = {{"psfile",  String}, /* j==0 in the routine below */
                    {"ifffile", String}, /* j==1 */
                    {"tekfile", String}, /* j==2 */
                    {"hsize",   Number},
                    {"vsize",   Number},
                    {"hoffset", Number},
                    {"voffset", Number},
                    {"hscale",  Number},
                    {"vscale",  Number},
                    {"angle",   Number},
                    {"llx", Number},
                    {"lly", Number},
                    {"urx", Number},
                    {"ury", Number},
                    {"rwi", Number}};

/*
 * compare strings, ignore case
 */
char Tolower(c)
register char c ;
{
   if ('A' <= c && c <= 'Z')
      return(c+32) ;
   else
      return(c) ;
}
int IsSame(a, b)
char *a, *b;
{
   for( ; *a != '\0'; )
      if( Tolower(*a++) != Tolower(*b++) ) 
         return( 0 );
      return( *b == '\0' );
}

char KeyStr[STRINGSIZE], ValStr[STRINGSIZE] ; /* Key and String values found */
long ValInt ; /* Integer value found */
float ValNum ; /* Number or Dimension value found */

char  *GetKeyVal(str,tno) /* returns NULL if none found, else next scan point */
   char *str ; /* starting point for scan */
   int  *tno ; /* table entry number of keyword, or -1 if keyword not found */
{
   register char *s, *k ;
   register int i ;
   register char t ;
   char c = '\0' ;

   for (s=str; *s <= ' ' && *s; s++) ; /* skip over blanks */
   if (*s == '\0')
      return (NULL) ;
   for (k=KeyStr; *s>' ' && *s!='='; *k++ = *s++) ; /* copy the keyword */
   *k = '\0' ;

   for(i=0; i<NKEYS; i++)
      if( IsSame(KeyStr, KeyTab[i].Entry) )
         goto found ;
   *tno = -1;
   return (s) ;

found: *tno = i ;
   ValStr[0] = '\0' ;
   if (KeyTab[i].Type == None)
      return (s) ;

   for (; *s <= ' ' && *s; s++) ; /* now look for the value part */
   if ( *s == '=' ) {
      for (s++; *s <= ' ' && *s; s++) ;
      if (*s=='\'' || *s=='\"')
         t = *s++ ;               /* get string delimiter */
      else t = ' ' ;
      for (k=ValStr; *s!=t && *s; *k++ = *s++) ; /* copy the value portion */
      if (*s == t )
            s++ ;                    /* advance past matching delimiter */
      *k = '\0' ;
   }
   switch (KeyTab[i].Type) {
 case Integer:
      if(sscanf(ValStr,"%ld%c",&ValInt,&c)!=1 || c!='\0') {
          sprintf(errbuf,"Non-integer value (%s) given for keyword %s",
              ValStr, KeyStr) ;
          error(errbuf) ;
          ValInt = 0 ;
      }
      break ;
 case Number:
 case Dimension:
      if(sscanf(ValStr,"%f%c",&ValNum,&c)!=1 || c!='\0') {  
          sprintf(errbuf,"Non-numeric value (%s) given for keyword %s",
              ValStr, KeyStr) ;
          error(errbuf) ;
          ValNum = 0 ;
      }
      if (KeyTab[i].Type==Dimension) {
         if (curfnt==NULL)
            error("! No font selected") ;
         ValNum = ValNum * ((double)curfnt->scaledsize) * conv * 72 / DPI ;
      }
      break ;
 default: break ;
   }
   return (s) ;
}

/*
 *   Now our routines.  We get the number of bytes specified and place them
 *   into the string buffer, and then parse it. Numerous conventions are
 *   supported here for historical reasons.
 */

void predospecial(numbytes)
integer numbytes ;
{
   register char *p = nextstring ;
   register int i = 0 ;

   if (nextstring + i > maxstring)
      error("! out of string space in predospecial") ;
   for (i=numbytes; i>0; i--)
      *p++ = (char)dvibyte() ;
   while (p[-1] <= ' ' && p > nextstring)
      p-- ; /* trim trailing blanks */
   if (p==nextstring) return ; /* all blank is no-op */
   *p = 0 ;
   p = nextstring ;
   while (*p <= ' ')
      p++ ;
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "Preprocessing special: %s\n", p) ;
#endif

   if (strcmp(p, "landscape")==0) {
      paperfmt = "landscape" ;
      return ;
   }
   usesspecial = 1 ;  /* now the special prolog will be sent */
   if (strncmp(p, "header", 6)==0) {
      char *q ;
      p += 6 ;
      while ((*p <= ' ' || *p == '=' || *p == '(') && *p != 0)
         p++ ;
      q = p ;  /* we will remove enclosing parentheses */
      p = p + strlen(p) - 1 ;
      while ((*p <= ' ' || *p == ')') && p >= q)
         p-- ;
      p[1] = 0 ;
      if (p >= q)
         add_header(q) ;
   }
   else if (*p == '!') {
      register struct bangspecial *q ;
      p++ ;
      q = (struct bangspecial *)malloc((unsigned)
                         (sizeof(struct bangspecial) + strlen(p))) ;
      if (q == NULL)
         error("! out of memory in predospecial") ;
      (void)strcpy(q->actualstuff, p) ;
      q->next = bangspecials ;
      bangspecials = q ;
   }
}

void dospecial(numbytes)
integer numbytes ;
{
   register char *p = nextstring ;
   register int i = 0 ;
   int j ;
   char psfile[100] ; 
   char cmdbuf[100] ; 
   register char *q ;
   Boolean psfilewanted = 1 ;

   if (nextstring + i > maxstring)
      error("! out of string space in dospecial") ;
   for (i=numbytes; i>0; i--)
      *p++ = (char)dvibyte() ;
   while (p[-1] <= ' ' && p > nextstring)
      p-- ; /* trim trailing blanks */
   if (p==nextstring) return ; /* all blank is no-op */
   *p = 0 ;
   p = nextstring ;
   while (*p <= ' ')
      p++ ;
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "Processing special: %s\n", p) ;
#endif

   if (strncmp(p, "ps:", 3)==0) {
        hvpos() ;
        if (p[3]==':') {
           if (strncmp(p+4, "[begin]", 7) == 0)
              cmdout(&p[11]);
           else if (strncmp(p+4, "[end]", 5) == 0)
              cmdout(&p[9]);
           else cmdout(&p[4]);
        } else if (strncmp(p+3, " plotfile ", 10) == 0) {
           char *sfp ;
           p += 13;
           for (sfp = p; *sfp && *sfp != ' '; sfp++) ;
           *sfp = '\0';
           figcopyfile (p);
        } else
           cmdout(&p[3]);
        return;
   }
   if (strcmp(p, "landscape")==0 || strncmp(p, "header", 6)==0 || *p=='!')
      return ; /* already handled in prescan */
#ifdef TPIC
   if (strncmp(p, "pn ", 3) == 0) {setPenSize(p+2); return;}
   if (strcmp(p, "fp") == 0) {flushPath(); return;}
   if (strncmp(p, "da ", 3) == 0) {flushDashed(p+2, 0); return;}
   if (strncmp(p, "dt ", 3) == 0) {flushDashed(p+2, 1); return;}
   if (strncmp(p, "pa ", 3) == 0) {addPath(p+2); return;}
   if (strncmp(p, "ar ", 3) == 0) {arc(p+2); return;}
   if (strcmp(p, "sp") == 0) {flushSpline(); return;}
   if (strcmp(p, "sh") == 0) {shadeLast(); return;}
   if (strcmp(p, "wh") == 0) {whitenLast(); return;}
   if (strcmp(p, "bk") == 0) {blackenLast(); return;}
   if (strncmp(p, "tx ", 3) == 0) {SetShade(p+3); return;}
#endif
   if (*p == '"') {
      hvpos();
      cmdout("@beginspecial") ;
      cmdout("@setspecial\n") ;
      trytobreakout(p+1) ;
      cmdout("\n@endspecial") ;
      return ;
   }

/* At last we get to the key/value conventions */
   psfile[0] = '\0';
   hvpos();
   cmdout("@beginspecial");

   while( (p=GetKeyVal(p,&j)) != NULL )
      switch (j) {
 case -1: /* for compatability with old conventions, we allow a file name
           * to be given without the 'psfile=' keyword */
         if (!psfile[0] && access(KeyStr,4)==0) /* yes we can read it */
             (void)strcpy(psfile,KeyStr) ;
         else {
           sprintf(errbuf, "Unknown keyword (%s) in \\special will be ignored",
                              KeyStr) ;
           error(errbuf) ;
         }
         break ;
 case 0: /* psfile */
         if (psfile[0]) {
           sprintf(errbuf, "More than one \\special psfile given; %s ignored", 
                              ValStr) ;
           error(errbuf) ;
         }
         else (void)strcpy(psfile,ValStr) ;
         break ;
 case 1: case 2:
         sprintf(errbuf, 
            "Sorry, there's presently no \\special support for %s", KeyStr) ;
         error(errbuf) ;
         psfilewanted = 0 ;
         break ;
 default: /* most keywords are output as PostScript procedure calls */
         if (KeyTab[j].Type == Integer)
            numout(ValInt);
         else if (KeyTab[j].Type == String)
            for (q=ValStr; *q; q++)
               scout(*q) ;
         else if (KeyTab[j].Type == None) ;
         else { /* Number or Dimension */
            ValInt = (integer)(ValNum<0? ValNum-0.5 : ValNum+0.5) ;
            if (ValInt-ValNum < 0.001 && ValInt-ValNum > -0.001)
                numout(ValInt) ;
            else {
               (void)sprintf(cmdbuf, "%f", ValNum) ;
               cmdout(cmdbuf) ;
            }
         }
      (void)sprintf(cmdbuf, "@%s", KeyStr);
      cmdout(cmdbuf) ;
      }

   cmdout("@setspecial");

   if(psfile[0]) {
      figcopyfile(psfile);
   } else if (psfilewanted)
      error("No \\special psfile was given; figure will be blank") ;

   cmdout("@endspecial");
}
