/*
 *   Code to download a font definition at the beginning of a section.
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
extern int free() ;
extern void numout() ;
extern void mhexout() ;
extern void cmdout() ;
extern long unpack() ;
extern void flip() ;
extern void specialout() ;
extern long getlong() ;
extern void error() ;
extern char *nextstring ;
/*
 *   These are the external variables we access.
 */
extern FILE *bitfile ;
extern fontdesctype *curfnt ;
extern long bytesleft ;
extern quarterword *raster ;
extern Boolean compressed ;
extern integer mag ;
/*
 *   We might use malloc here.
 */
extern char *malloc() ;
/*
 *   We have a routine that downloads an individual character.
 */
void downchar(c, cc)
chardesctype *c ;
shalfword cc ;
{
   register long i, j ;
   register halfword cheight, cwidth ;
   register long k ;
   register quarterword *p ;
   register halfword cmd ;
   register shalfword xoff, yoff ;
   halfword wwidth ;
   register long len ;

   p = c->packptr ;
   cmd = *p++ ;
   if (cmd & 4) {
      if ((cmd & 7) == 7) {
         cwidth = getlong(p) ;
         cheight = getlong(p + 4) ;
         xoff = getlong(p + 8) ;
         yoff = getlong(p + 12) ;
         p += 16 ;
      } else {
         cwidth = p[0] * 256 + p[1] ;
         cheight = p[2] * 256 + p[3] ;
         xoff = p[4] * 256 + p[5] ; /* N.B.: xoff, yoff are signed halfwords */
         yoff = p[6] * 256 + p[7] ;
         p += 8 ;
      }
   } else {
      cwidth = *p++ ;
      cheight = *p++ ;
      xoff = *p++ ;
      yoff = *p++ ;
      if (xoff > 127)
         xoff -= 256 ;
      if (yoff > 127)
         yoff -= 256 ;
   }
   if (compressed) {
      len = getlong(p) ;
      p += 4 ;
   } else {
      wwidth = (cwidth + 15) / 16 ;
      i = 2 * cheight * (long)wwidth ;
      if (i <= 0)
         i = 2 ;
      if (bytesleft < i) {
         if (bytesleft >= RASTERCHUNK)
            (void) free((char *) raster) ;
         if (RASTERCHUNK > i) {
            raster = (quarterword *)malloc(RASTERCHUNK) ;
            bytesleft = RASTERCHUNK ;
         } else {
            raster = (quarterword *)malloc((unsigned)i) ;
            bytesleft = i ;
         }
         if (raster == NULL) {
            error("! out of memory during allocation") ;
         }
      }
      k = i;
      while (k > 0)
         raster[--k] = 0 ;
      unpack(p, raster, cwidth, cheight, cmd) ;
      len = i ;
   }
   if (cheight == 0 || cwidth == 0 || len == 0) {
      cwidth = 1 ;
      cheight = 1 ;
      len = 1 ;
      raster[0] = 0 ;
   }
/*
 *   Now we actually send out the data.
 */
   specialout('[') ;
   specialout('<') ;
   if (compressed)
      mhexout(p, len) ;
   else {
      p = raster ;
      i = (cwidth + 7) / 8 ;
      for (j=0; j<cheight; j++) {
         mhexout(p, i) ;
         p += 2*wwidth ;
      }
   }
   specialout('>') ;
   numout((integer)cwidth) ;
   numout((integer)cheight) ;
   numout((integer)(- xoff)) ;
   numout((integer)(cheight - yoff - 1)) ;
   numout((integer)(c->pixelwidth)) ;
   specialout(']') ;
   numout((integer)cc) ;
   cmdout("dc") ;
}
/*
 *   And the download procedure.
 */
void download(p, psfont)
charusetype *p ;
int psfont ;
{
   register halfword b, bit ;
   register chardesctype *c ;
   int cc ;
   char name[10] ;

   if (psfont < 27)
      (void)sprintf(name, "/f%c", 'a'+psfont-1) ;
   else
      (void)sprintf(name, "/f%d", psfont-27) ;
   cmdout(name) ;
   curfnt = p->fd ;
   curfnt->psname = psfont ;
   if (curfnt->resfont) {
      cmdout("[") ;
      c = curfnt->chardesc ;
      for (cc=0; cc<256; cc++, c++)
         numout((integer)c->pixelwidth) ;
      cmdout("]") ;
      if (curfnt->resfont->specialinstructions) {
            cmdout(curfnt->resfont->specialinstructions) ;
      } else {
	  (void)strcpy(nextstring, "/") ;
	  (void)strcat(nextstring, curfnt->resfont->PSname) ;
	  cmdout(nextstring) ;
      }
      (void)sprintf(nextstring, "%ld", mag) ;
      cmdout(nextstring) ;
      (void)sprintf(nextstring, "%ld", curfnt->scaledsize) ;
      cmdout(nextstring) ;
      cmdout("rf") ;
      return ;
   }
   cmdout("df") ;
   c = curfnt->chardesc ;
   cc = 0 ;
   for (b=0; b<16; b++) {
      for (bit=32768; bit>0; bit>>=1) {
         if (p->bitmap[b] & bit) {
            downchar(c, cc) ;
            c->flags |= EXISTS ;
         } else
            c->flags &= ~EXISTS ;
         c++ ;
         cc++ ;
      }
   }
   cmdout("dfe") ;
}
