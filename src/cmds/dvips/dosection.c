/*
 *  Code to output PostScript commands for one section of the document.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
/*
 *   These are the external routines we call.
 */
extern void dopage() ;
extern void download() ;
extern integer signedquad() ;
extern void skipover() ;
extern void cmdout() ;
extern void numout() ;
extern void newline() ;
/*
 *   These are the external variables we access.
 */
extern FILE *dvifile ;
extern FILE *bitfile ;
extern integer pagenum ;
extern long bytesleft ;
extern quarterword *raster ;
extern int quiet ;
extern Boolean reverse, multiplesects, disablecomments ;
extern int actualdpi ;
static int psfont ;
/*
 *   Now we have the main procedure.
 */
void
dosection(s, c)
        sectiontype *s ;
        int c ;
{
   charusetype *cu ;
   integer prevptr ;
   int np ;

   cmdout("TeXDict") ;
   cmdout("begin") ;
   numout((integer)DPI) ;
   cmdout("@start") ;
   if (multiplesects)
      cmdout("bos") ;
/*
 *   We insure raster is even-word aligned, because download might want that.
 */
   if (bytesleft & 1) {
      bytesleft-- ;
      raster++ ;
   }
   cu = (charusetype *) (s + 1) ;
   psfont = 1 ;
   while (cu->fd)
      download(cu++, psfont++) ;
   setup() ;
   while (c > 0) {
      c-- ;
      prevptr = s->bos ;
      if (! reverse)
         (void)fseek(dvifile, (long)prevptr, 0) ;
      np = s->numpages ;
      while (np-- != 0) {
         if (reverse)
            (void)fseek(dvifile, (long)prevptr, 0) ;
         pagenum = signedquad() ;
         if (! quiet) {
#ifdef SHORTINT
            (void)fprintf(stderr, "[%ld", pagenum) ;
#else  /* ~SHORTINT */
            (void)fprintf(stderr, "[%d", pagenum) ;
#endif /* ~SHORTINT */
            (void)fflush(stderr) ;
         }
         skipover(36) ;
         prevptr = signedquad()+1 ;
         dopage() ;
         if (! quiet) {
            (void)fprintf(stderr, "] ") ;
            (void)fflush(stderr) ;
         }
         if (! reverse)
            (void)skipnop() ;
      }
   }
   if (!disablecomments) {
      newline() ;
      (void)fprintf(bitfile, "%%%%Trailer\n") ;
   }
   if (multiplesects)
      cmdout("eos") ;
   cmdout("end") ;
}
