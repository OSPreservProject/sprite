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
extern FILE *search() ;
extern char *getenv() ;
extern void fonttableout() ;
extern void makepsname() ;
/*
 *   These are the external variables used by these routines.
 */
extern integer hh, vv ;
extern fontdesctype *curfnt ;
extern FILE *bitfile ;
extern char *oname ;
extern Boolean reverse ;
extern Boolean removecomments ;
extern Boolean sendcontrolD, disablecomments, multiplesects ;
extern Boolean usesPSfonts, headers_off ;
extern Boolean safetyenclose ;
extern int numcopies ;
extern int totalpages ;
extern integer pagenum ;
extern Boolean manualfeed ;
extern int landscape ;
extern int quiet ;
extern int prettycolumn ;
extern int actualdpi ;
extern char *iname ;
extern char *paperfmt ;
extern char *headerpath ;
extern char errbuf[] ;
extern shalfword linepos ;
extern char *figpath ;
extern struct header_list *ps_fonts_used ;
extern char banner[] ;
/*
 *   We need a few statics to take care of things.
 */
static integer rhh, rvv ;
static int instring ;
static Boolean lastspecial = 1 ;
static shalfword d ;
static Boolean popened = 0 ;
int lastfont ; /* exported to dospecial to fix rotate.tex problem */
static void chrcmd();                   /* just a forward declaration */
static char strbuffer[LINELENGTH + 20], *strbp = strbuffer ;
static struct {
    char *format;
    int width, height;
} paper_types[] = {
    { "letter",  612,  792 },		/* 8.5 x 11 */
    { "legal",   612, 1008 },		/* 8.5 x 14 */
    { "ledger",  792, 1224 },		/* 11 x 17 */
    { "a4",	 596,  843 },		/* 210mm x 297mm */
    { "a3",      843, 1192 } };		/* 297mm x 420mm */
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

   switch (infigure) {
   case 1:
      f = search(figpath, s, READ) ;
      (void)sprintf(errbuf, "Couldn't find figure file %s; continuing", s) ;
      break ;
   default:
      f = search(headerpath, s, READ) ;
      (void)sprintf(errbuf, "! Couldn't find header file %s", s) ;
      break ;
#ifndef VMS
#ifndef MSDOS
   case 2:
      f = popen(s, "r") ;
      (void)sprintf(errbuf, "Failure to execute %s; continuing", s) ;
      break;
#endif
#endif
   }
   if (f==NULL)
      error(errbuf) ;
   else {
      if (! quiet) {
         if (strlen(s) + prettycolumn > 76) {
            fprintf(stderr, "\n") ;
            prettycolumn = 0 ;
         }
         (void)fprintf(stderr, "<%s>", s) ;
         (void)fflush(stderr) ;
         prettycolumn += 2 + strlen(s) ;
      }
      if (linepos != 0)
         (void)putc('\n', bitfile) ;
      if (! disablecomments)
         if (infigure)
            (void)fprintf(bitfile, "%%%%BeginDocument: %s\n", s) ;
         else
            (void)fprintf(bitfile, "%%%%BeginProcSet: %s\n", s) ;
      while ((c=getc(f))!=EOF && c != 4) {
         if (! removecomments && c == '%' && prevc == '\n') {/* skip comments */
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
#ifndef VMS
#ifndef MSDOS
      if (infigure == 2)
         (void)pclose(f) ;
      else
#endif
#endif
         (void)fclose(f) ;
      if (!disablecomments)
         if (infigure)
            (void)fprintf(bitfile, "%%%%EndDocument\n") ;
         else
            (void)fprintf(bitfile, "%%%%EndProcSet\n") ;
   }
}

/*
 *   For included PostScript graphics, we use the above routine, but
 *   with no fatal error message.
 */
void figcopyfile(s, systemtype)
char *s ;
int systemtype ;
{
   infigure = systemtype ? 2 : 1 ;
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
stringend()
{
   if (linepos + instring >= LINELENGTH - 2) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
   }
   (void)putc('(', bitfile) ;
   *strbp = 0 ;
   (void)fputs(strbuffer, bitfile) ;
   (void)putc(')', bitfile) ;
   linepos += instring + 2 ;
   lastspecial = 1 ;
   instring = 0 ;
   strbp = strbuffer ;
}

void
scout(c)   /* string character out */
        char c ;
{
/*
 *   Is there room in the buffer?  LINELENGTH-6 is used because we
 *   need room for (, ), and a possible four-byte string \000, for
 *   instance.  If it is too long, we send out the string.
 */
   if (instring > LINELENGTH-6) {
      stringend() ;
      chrcmd('p') ;
   }
   if (c<' ' || c>126 || c=='%') {
      *strbp++ = '\\' ;
      *strbp++ = '0' + ((c >> 6) & 3) ;
      *strbp++ = '0' + ((c >> 3) & 7) ;
      *strbp++ = '0' + (c & 7) ;
      instring += 4 ;
   } else if (c == '(' || c == ')' || c == '\\') {
      *strbp++ = '\\' ;
      *strbp++ = c ;
      instring += 2 ;
   } else {
      *strbp++ = c ;
      instring++ ;
   }
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
   if ((! lastspecial && linepos >= LINELENGTH - 20) ||
           linepos + l >= LINELENGTH) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
      lastspecial = 1 ;
   } else if (! lastspecial) {
      (void)putc(' ', bitfile) ;
      linepos++ ;
   }
   (void)fputs(s, bitfile) ;
   linepos += l ;
   lastspecial = 0 ;
}


static void
chrcmd(c)
        char c ;
{
   if ((! lastspecial && linepos >= LINELENGTH - 20) ||
       linepos + 2 > LINELENGTH) {
      (void)putc('\n', bitfile) ;
      linepos = 0 ;
      lastspecial = 1 ;
   } else if (! lastspecial) {
      (void)putc(' ', bitfile) ;
      linepos++ ;
   }
   (void)putc(c, bitfile) ;
   linepos++ ;
   lastspecial = 0 ;
}

void
floatout(n)
        float n ;
{
   char buf[20] ;

   (void)sprintf(buf, "%.2f", n) ;
   cmdout(buf) ;
}

void
numout(n)
        integer n ;
{
   char buf[10] ;

   (void)sprintf(buf, "%ld", n) ;
   cmdout(buf) ;
}

void
mhexout(p, len)
register unsigned char *p ;
register long len ;
{
   register char *hexchar = "0123456789ABCDEF" ;
   register int n, k ;

   while (len > 0) {
      if (linepos > LINELENGTH - 2) {
         (void)putc('\n', bitfile) ;
         linepos = 0 ;
      }
      k = (LINELENGTH - linepos) >> 1 ;
      if (k > len)
         k = len ;
      len -= k ;
      linepos += (k << 1) ;
      while (k--) {
         n = *p++ ;
         (void)putc(hexchar[n >> 4], bitfile) ;
         (void)putc(hexchar[n & 15], bitfile) ;
      }
   }
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
   makepsname(buf, n) ;
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
            chrcmd((char)('p' + hh - rhh)) ;
         } else if (hh - rhh < d + 5 && rhh - hh < 5 - d) {
            chrcmd((char)('g' + hh - rhh - d)) ;
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
   lastspecial = 1 ;
}

void
nlcmdout(s)
        char *s ;
{
   newline() ;
   cmdout(s) ;
   newline() ;
}

void
initprinter()
{
   void tell_needed_fonts() ;
   int i;
   if (*oname != 0) {
/*
 *   We check to see if the first character is a exclamation
 *   point, and popen if so.
 */
      if (*oname == '!' || *oname == '|') {
#ifdef MSDOS
            error("! can't open output pipe") ;
#else
#ifdef VMS
            error("! can't open output pipe") ;
#else
         if ((bitfile=popen(oname+1, "w"))==NULL)
            error("! couldn't open output pipe") ;
         else
            popened = 1 ;
#endif
#endif
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
      (void)fprintf(bitfile, "%%%%Creator: %s", banner + 8) ;
      if (*iname)
         (void)fprintf(bitfile, "%%%%Title: %s\n", iname) ;
      (void)fprintf(bitfile, "%%%%Pages: %d %d\n", totalpages, 1 - 2*reverse) ;
      if (paperfmt && *paperfmt) {
         for (i=0; i<sizeof(paper_types)/sizeof(paper_types[0]); ++i)
            if (strcmp(paperfmt,paper_types[i].format)==0)
                (void)fprintf(bitfile, "%%%%BoundingBox: 0 0 %d %d\n",
                    paper_types[i].width, paper_types[i].height) ;
      } else
         fprintf(bitfile, "%%%%BoundingBox: 0 0 612 792\n") ;
      tell_needed_fonts() ;
      (void)fprintf(bitfile, "%%%%EndComments\n") ;
   }
   if (safetyenclose)
      (void)fprintf(bitfile, "/SafetyEnclosure save def\n") ;
   if (! headers_off)
      send_headers() ;
}

static int endprologsent ;
void setup() {
   newline() ;
   if (endprologsent == 0 && !disablecomments) {
      fonttableout() ;
      (void)fprintf(bitfile, "%%%%EndProlog\n") ;
      (void)fprintf(bitfile, "%%%%BeginSetup\n") ;
      (void)fprintf(bitfile, "%%%%Feature: *Resolution %d\n", DPI) ;
      if (manualfeed)
         (void)fprintf(bitfile, "%%%%Feature: *ManualFeed True\n") ;
   }
   cmdout("TeXDict") ;
   cmdout("begin") ;
   {
      char pft[100] ;
      strcpy(pft, "@") ;
      if (landscape)
         strcat(pft, "landscape ") ;
      else if (paperfmt)
         strcat(pft, paperfmt) ;
      else
         pft[0] = 0 ;
/*    if (paperfmt) {         doing this has many bad effects!!!
         strcat(pft, " /") ;
         strcat(pft, paperfmt) ;
         strcat(pft, " where {pop ") ;
         strcat(pft, paperfmt) ;
         strcat(pft, "} if") ;
      } */
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
   (void)fprintf(bitfile, "userdict /end-hook known{end-hook}if\n") ;
   if (safetyenclose)
      (void)fprintf(bitfile, "SafetyEnclosure restore\n") ;
   if (!disablecomments)
      (void)fprintf(bitfile, "%%%%EOF\n") ;
   if (sendcontrolD)
      (void)putc(4, bitfile) ;
#ifndef MSDOS
#ifndef VMS
   if (popened)
      (void)pclose(bitfile) ;
#endif
#endif
   if (popened == 0)
      (void)fclose(bitfile) ;
   bitfile = NULL ;
}

/* this tells dvips that it has no clue where it is. */
static int thispage = 0 ;
static integer rulex, ruley ;
void psflush() {
   rulex = ruley = rhh = rvv = -314159265 ;
   lastfont = -1 ;
}
/*
 *   pageinit initializes the output variables.
 */
void
pageinit()
{
   psflush() ;
   newline() ;
   if (!disablecomments && !multiplesects)
      (void)fprintf(bitfile, "%%%%Page: %ld %d\n", pagenum, ++thispage) ;
   linepos = 0 ;
   cmdout("bop") ;
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
 *   It does nothing to save/restore the current position,
 *   or even draw the current string.  (Rules are normally
 *   set below the baseline anyway, so this saves us on
 *   output size almost always.)
 */
void
drawrule(rw, rh)
        integer rw, rh ;
{
   numout((integer)hh) ;
   numout((integer)vv) ;
   if (rw == rulex && rh == ruley)
      chrcmd('V') ;
   else {
      numout((integer)rw) ;
      numout((integer)rh) ;
      chrcmd('v') ;
      rulex = rw ;
      ruley = rh ;
   }
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
/*
 *   This routine sends out the document fonts comment.
 */
void tell_needed_fonts() {
   struct header_list *hl = ps_fonts_used ;
   char *q ;
   int roomleft = -1 ;
   extern char *get_name() ;

   if (hl == 0)
      return ;
   while (q=get_name(&hl)) {
      if ((int)strlen(q) >= roomleft) {
         if (roomleft != -1) {
            fprintf(bitfile, "\n%%%%+") ;
            roomleft = LINELENGTH - 3 ;
         } else {
            fprintf(bitfile, "%%%%DocumentFonts:") ;
            roomleft = LINELENGTH - 16 ;
         }
      }
      fprintf(bitfile, " %s", q) ;
      roomleft -= strlen(q) + 1 ;
   }
   fprintf(bitfile, "\n") ;
}
