/*
 *   These routines do most of the communicating with the printer.
 *
 *   LINELENGTH tells the maximum line length to send out.
 */
#define LINELENGTH (78)
#include "structures.h" /* The copyright notice in that file is included too! */
#include <ctype.h>
/*
 *   The external routines called here:
 */
extern void error() ;
extern void send_headers() ;
extern FILE *search(), *popen();
/*
 *   These are the external variables used by these routines.
 */
extern integer hh, vv ;
extern fontdesctype *curfnt ;
extern FILE *bitfile;
extern char *oname ;
extern Boolean reverse ;
extern Boolean sendcontrolD, disablecomments, multiplesects ;
extern Boolean usesPSfonts, headers_off ;
extern int numcopies ;
extern int totalpages ;
extern integer pagenum ;
extern Boolean manualfeed ;
extern int quiet ;
extern int actualdpi ;
extern char *iname ;
extern char *paperfmt ;
extern char *getenv() ;
extern char errbuf[] ;
extern shalfword linepos ;
/*
 *   We need a few statics to take care of things.
 */
static integer rhh, rvv ;
static Boolean instring ;
static Boolean lastspecial = 1 ;
static shalfword d ;
static int lastfont ;
static void chrcmd();                   /* just a forward declaration */

/*
 *   Some very low level primitives to send output to the printer.
 */

/*
 *   This routine copies a file down the pipe.  Search path uses the
 *   header path.
 */
static int infigure ;
void
copyfile(s)
        char *s ;
{
   FILE *f ;
   int c, prevc = '\n' ;

   if (infigure) {
      f = search(FIGPATH, s) ;
      (void)sprintf(errbuf, "Couldn't find figure file %s; continuing", s) ;
   } else {
      f = search(HEADERPATH, s) ;
      (void)sprintf(errbuf, "! Couldn't find header file %s", s) ;
   }
   if (f==NULL)
      error(errbuf) ;
   else {
      if (! quiet) {
         (void)fprintf(stderr, "[%s]", s) ;
         (void)fflush(stderr) ;
      }
      if (linepos != 0)
         (void)putc('\n', bitfile) ;
      if (! disablecomments)
         (void)fprintf(bitfile, "%%%%BeginDocument: %s\n", s) ;
      while ((c=getc(f))!=EOF) {
         if (c == '%' && prevc == '\n') { /* skip comments */
            while ((c=getc(f))!=EOF) {
               if (c=='\n')
                  break ;
            }
         } else
            (void)putc(c, bitfile) ;
         prevc = c ;
      }
      if (prevc != '\n')
         (void)putc('\n', bitfile) ;
      linepos = 0 ;
      (void)fclose(f) ;
      if (!disablecomments)
         (void)fprintf(bitfile, "%%%%EndDocument\n") ;
   }
}

/*
 *   For included PostScript graphics, we use the above routine, but
 *   with no fatal error message.
 */
void figcopyfile(s)
char *s ;
{
   infigure = 1 ;
   copyfile(s) ;
   infigure = 0 ;
}

/*
 *   This next routine writes out a `special' character.  In this case,
 *   we simply put it out, since any special character terminates the
 *   preceding token.
 */
void
specialout(c)
        char c ;
{
   if (linepos >= LINELENGTH) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
   }
   (void)putc(c, bitfile) ;
   linepos++ ;
   lastspecial = 1 ;
}

void
scout(c)   /* string character out */
        char c ;
{
   if (! instring) {
/*
 *   If we are within 5, we send a newline before starting the string.
 *   This eliminates extraneous backslash-newlines.
 */
      if (linepos + 5 > LINELENGTH) {
         (void)putc('\n', bitfile) ;
         linepos = 0 ;
      }
      (void)putc('(', bitfile) ;
      linepos++ ;
      lastspecial = 1 ;
      instring = 1 ;
   }
   if (c<' ' || c>126 || (c=='%' && linepos + 2 > LINELENGTH)) {
      if (linepos + 5 > LINELENGTH) {
         (void)putc('\\', bitfile) ;
         (void)putc('\n', bitfile) ;
         linepos = 0 ;
      }
      (void)putc('\\', bitfile) ;
      linepos++ ;
      (void)putc('0' + ((c >> 6) & 3), bitfile) ;
      linepos++ ;
      (void)putc('0' + ((c >> 3) & 7), bitfile) ;
      linepos++ ;
      (void)putc('0' + (c & 7), bitfile) ;
      linepos++ ;
   } else if (c == '(' || c == ')' || c == '\\') {
      if (linepos + 3 > LINELENGTH) {
         (void)putc('\\', bitfile) ;
         (void)putc('\n', bitfile) ;
         linepos = 0 ;
      }
      (void)putc('\\', bitfile) ;
      linepos++ ;
      (void)putc(c, bitfile) ;
      linepos++ ;
   } else {
      if (linepos + 2 > LINELENGTH) {
         (void)putc('\\', bitfile) ;
         (void)putc('\n', bitfile) ;
         linepos = 0 ;
      }
      (void)putc(c, bitfile) ;
      linepos++ ;
   }
}

void
stringend()
{
   instring = 0 ;
   specialout(')') ;
}

void
cmdout(s)
        char *s ;
{
   int l ;

   /* hack added by dorab */
   if (instring) {
        stringend();
        chrcmd('p');
   }
   l = strlen(s) ;
   if (linepos + l >= LINELENGTH) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
      lastspecial = 1 ;
   }
   if (! lastspecial) {
      (void)putc(' ', bitfile) ;
      linepos++ ;
   }
   while (*s != 0) {
      (void)putc(*s++, bitfile) ;
   }
   linepos += l ;
   lastspecial = 0 ;
}

static void
chrcmd(c)
        char c ;
{
   if (linepos + 2 > LINELENGTH) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
      lastspecial = 1 ;
   }
   if (! lastspecial) {
      (void)putc(' ', bitfile) ;
      linepos++ ;
   }
   (void)putc(c, bitfile) ;
   linepos++ ;
   lastspecial = 0 ;
}

void
numout(n)
        integer n ;
{
   char buf[10] ;

   (void)sprintf(buf, "%ld", n) ;
   cmdout(buf) ;
}

void mhexout(p, len)
register unsigned char *p ;
register long len ;
{
   register char *hexchar = "0123456789ABCDEF" ;
   register int linep ;
   register int n ;

   linep = linepos ;
   while (len > 0) {
      if (linep > LINELENGTH - 2) {
         (void)putc('\n', bitfile) ;
         linep = 0 ;
      }
      n = *p++ ;
      (void)putc(hexchar[n >> 4], bitfile) ;
      (void)putc(hexchar[n & 15], bitfile) ;
      linep += 2 ;
      len-- ;
   }
   linepos = linep ;
}

void
fontout(n)
        int n ;
{
   char buf[6] ;

   if (instring) {
      stringend() ;
      chrcmd('p') ;
   }
   if (n < 27)
      (void)sprintf(buf, "f%c", 'a'+n-1) ;
   else
      (void)sprintf(buf, "f%d", n-27) ;
   cmdout(buf) ;
}

void
hvpos()
{
   if (rvv != vv) {
      if (instring) {
         stringend() ;
         numout(hh) ;
         numout(vv) ;
         chrcmd('y') ;
      } else if (rhh != hh) {
         numout(hh) ;
         numout(vv) ;
         chrcmd('a') ;
      } else { /* hard to get this case, but it's there when you need it! */
         numout(vv - rvv) ;
         chrcmd('x') ;
      }
      rvv = vv ;
   } else if (rhh != hh) {
      if (instring) {
         stringend() ;
         if (hh - rhh < 5 && rhh - hh < 5) {
            chrcmd('p' + hh - rhh) ;
         } else if (hh - rhh < d + 5 && rhh - hh < 5 - d) {
            chrcmd('g' + hh - rhh - d) ;
            d = hh - rhh ;
         } else {
            numout(hh - rhh) ;
            chrcmd('b') ;
            d = hh - rhh ;
         }
      } else {
         numout(hh - rhh) ;
         chrcmd('w') ;
      }
   }
   rhh = hh ;
}

/*
 *   initprinter opens the bitfile and writes the initialization sequence
 *   to it.
 */
void newline()
{
   if (linepos != 0) {
      (void)fprintf(bitfile, "\n") ;
      linepos = 0 ;
   }
}
void
initprinter()
{
   if (*oname != 0) {
/*
 *   We check to see if the first character is a exclamation
 *   point, and popen if so.
 */
      if (*oname == '!' || *oname == '|') {
         if ((bitfile=popen(oname+1, "w"))==NULL)
            error("! couldn't open output pipe") ;
      } else {
         if ((bitfile=fopen(oname,"w"))==NULL)
            error("! couldn't open PostScript file") ;
      }
   } else {
      bitfile = stdout ;
   }
   if (disablecomments)
      (void)fprintf(bitfile,
             "%%!PS (but not EPSF; comments have been disabled)\n") ;
   else {
      if (multiplesects)
         (void)fprintf(bitfile,
             "%%!PS (but not EPSF because of memory limits)\n") ;
      else  (void)fprintf(bitfile, "%%!PS-Adobe-2.0\n") ;
      (void)fprintf(bitfile, "%%%%Creator: dvips by Radical Eye Software\n") ;
      if (*iname)
         (void)fprintf(bitfile, "%%%%Title: %s\n", iname) ;
      (void)fprintf(bitfile, "%%%%Pages: %d %d\n", totalpages, 1 - 2*reverse) ;
      if (strcmp(paperfmt,"letter")==0)
         (void)fprintf(bitfile, "%%%%BoundingBox: 0 0 612 792\n") ; /*8.5x11*/
      else if (strcmp(paperfmt,"landscape")==0)
         (void)fprintf(bitfile, "%%%%BoundingBox: 0 0 792 612\n") ;
      else if (strcmp(paperfmt,"a4")==0)
         (void)fprintf(bitfile, "%%%%BoundingBox: 0 0 612 842\n") ;
      else (void)fprintf(bitfile, "%%%%BoundingBox: 0 0 612 1008\n"); /*8.5x14*/
      (void)fprintf(bitfile, "%%%%EndComments\n") ;
   }
   if (! headers_off)
      send_headers() ;
}

static int endprologsent ;
setup() {
   cmdout("end") ;
   newline() ;
   if (endprologsent == 0 && !disablecomments) {
#if 0
      (void)fprintf(bitfile, "%%%%EndProlog\n") ;
#endif
      (void)fprintf(bitfile, "%%%%BeginSetup\n") ;
      (void)fprintf(bitfile, "%%%%Feature: *Resolution %d\n", DPI) ;
   }
   cmdout("TeXDict") ;
   cmdout("begin") ;
   {  char pft[100] ;
      strcpy(pft, "@") ;
      strcat(pft, paperfmt) ;
      cmdout(pft) ;
   }
   if (manualfeed) cmdout("@manualfeed") ;
   if (numcopies != 1) {
      numout((integer)numcopies) ;
      cmdout("@copies") ;
   }
   if (endprologsent == 0 && !disablecomments) {
      newline() ;
      endprologsent = 1 ;
      (void)fprintf(bitfile, "%%%%EndSetup\n") ;
   }

}
/*
 *   cleanprinter is the antithesis of the above routine.
 */
void
cleanprinter()
{
   (void)fprintf(bitfile, "\n") ;
   if (!disablecomments)
      (void)fprintf(bitfile, "%%%%EOF\n") ;
   if (sendcontrolD)
      (void)putc(4, bitfile) ;
   (void)fclose(bitfile) ;
   bitfile = NULL ;
}

/*
 *   pageinit initializes the output variables.
 */
static int thispage = 0 ;
void
pageinit()
{
   rhh = rvv = -3141592653 ;
   newline() ;
   if (!disablecomments)
      (void)fprintf(bitfile, "%%%%Page: %ld %d\n", pagenum, ++thispage) ;
   linepos = 0 ;
   cmdout("bop") ;
   lastfont = -1 ;
   d = 0 ;
}

/*
 *   This routine ends a page.
 */
void
pageend()
{
   if (instring) {
      stringend() ;
      chrcmd('p') ;
   }
   cmdout("eop") ;
}

/*
 *   drawrule draws a rule at the specified position.
 */
void
drawrule(rw, rh)
        shalfword rw, rh ;
{
   hvpos() ;
   if (instring) {
      stringend() ;
      chrcmd('p') ;
   }
   numout((integer)rw) ;
   numout((integer)rh) ;
   chrcmd('v') ;
}

/*
 *   drawchar draws a character at the specified position.
 */
void
drawchar(c, cc)
        chardesctype *c ;
        int cc ;
{
   hvpos() ;
   if (lastfont != curfnt->psname) {
      fontout((int)curfnt->psname) ;
      lastfont = curfnt->psname ;
   }
   scout(cc) ;
   rhh = hh + c->pixelwidth ; /* rvv = rv */
}
