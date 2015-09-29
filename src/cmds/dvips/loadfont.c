/*
 *   Here's the code to load a PK file into memory.
 *   Individual bitmaps won't be unpacked until they prove to be needed.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we use.
 */
extern void makefont() ;
extern void error() ;
extern integer scalewidth() ;
extern void tfmload() ;
extern FILE *search() ;
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern long bytesleft ;
extern quarterword *raster ;
extern int actualdpi ;
extern real alpha ;
extern char *pkpath ;
char errbuf[200] ;
/*
 *   We use malloc here.
 */
char *malloc() ;

/*
 *   Now we have some routines to get stuff from the PK file.
 *   Subroutine pkbyte returns the next byte.
 */

static FILE *pkfile ;
static char name[50] ;
void
badpk(s)
   char *s ;
{
   (void)sprintf(errbuf,"! Bad PK file %s: %s",name,s) ;
   error(errbuf);
}

shalfword
pkbyte()
{
   register shalfword i ;

   if ((i=getc(pkfile))==EOF)
      badpk("unexpected eof") ;
   return(i) ;
}

integer
pkquad()
{
   register integer i ;

   i = pkbyte() ;
   if (i > 127)
      i -= 256 ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   return(i) ;
}

integer
pktrio()
{
   register integer i ;

   i = pkbyte() ;
   i = i * 256 + pkbyte() ;
   i = i * 256 + pkbyte() ;
   return(i) ;
}


/*
 *   pkopen opens the pk file.  This is system dependent.
 */
static int dontmakefont = 0 ; /* if makefont fails once we won't try again */

Boolean
pkopen(fd)
        register fontdesctype *fd ;
{
   register char *d, *n ;
   register int del ;
   register halfword givendpi ;

   d = fd->area ;
   n = fd->name ;
   if (*d==0)
      d = pkpath ;
   givendpi = fd->dpi ;
   for (del=0; (real)del/(real)givendpi<=0.01; del = del>0? -del: -del+1 ) {
      (void)sprintf(name, "%s.%dpk", n, givendpi+del) ;
      if (pkfile=search(d, name))
         return(1) ;
   }
   (void)sprintf(name, "%s.%dpk", n, givendpi) ;
   if (d == pkpath && dontmakefont == 0) {
      makefont(n, (int)givendpi, DPI) ;
      if (pkfile = search(d, name))
         return(1) ;
      dontmakefont = 1 ;
   }
   (void)sprintf(errbuf,
      "Font %s%s not found, characters will be left blank.",
      fd->area, name) ;
   error(errbuf) ;
   return(0) ;
}

/*
 *   Now our loadfont routine.
 */
void
loadfont(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   register shalfword cmd ;
   register integer k ;
   register integer length ;
   register shalfword cc ;
   register integer scaledsize = curfnt->scaledsize ;
   register quarterword *tempr ;
   register chardesctype *cd ;

/*
 *   We clear out some pointers:
 */
   for (i=0; i<256; i++) {
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   if (!pkopen(curfnt)) {
      tfmload(curfnt) ;
      return ;
   }
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Loading virtual font %s at %.1fpt\n",
         name, (real)scaledsize/(alpha*0x100000)) ;
#endif /* DEBUG */
   if (pkbyte()!=247)
      badpk("expected pre") ;
   if (pkbyte()!=89)
      badpk("wrong id byte") ;
   for(i=pkbyte(); i>0; i--)
      (void)pkbyte() ;
   k = (integer)(alpha * (real)pkquad()) ;
   if (k > curfnt->designsize + 2 || k < curfnt->designsize - 2) {
      (void)sprintf(errbuf,"Design size mismatch in font %s", name) ;
      error(errbuf) ;
   }
   k = pkquad() ;
   if (k && curfnt->checksum)
      if (k!=curfnt->checksum) {
         (void)sprintf(errbuf,"Checksum mismatch in font %s", name) ;
         error(errbuf) ;
       }
   k = pkquad() ; /* assume that hppp is correct in the PK file */
   k = pkquad() ; /* assume that vppp is correct in the PK file */
/*
 *   Now we get down to the serious business of reading character definitions.
 */
   while ((cmd=pkbyte())!=245) {
      if (cmd < 240) {
         switch (cmd & 7) {
case 0: case 1: case 2: case 3:
            length = (cmd & 7) * 256 + pkbyte() - 3 ;
            cc = pkbyte() ;
            cd = curfnt->chardesc+cc ;
            cd->TFMwidth = scalewidth(pktrio(), scaledsize) ;
            cd->pixelwidth = pkbyte() ;
            break ;
case 4:
            length = pkbyte() * 256 ;
            length = length + pkbyte() - 4 ;
            cc = pkbyte() ;
            cd = curfnt->chardesc+cc ;
            cd->TFMwidth = scalewidth(pktrio(), scaledsize) ;
            i = pkbyte() ;
            cd->pixelwidth = i * 256 + pkbyte() ;
            break ;
case 5: case 6:
            badpk("! character too big") ;
case 7:
            length = pkquad() - 11 ;
            cc = pkquad() ;
            if (cc<0 || cc>255) badpk("character code out of range") ;
            cd = curfnt->chardesc + cc ;
            cd->TFMwidth = scalewidth(pkquad(), scaledsize) ;
            cd->pixelwidth = (pkquad() + 32768) >> 16 ;
            k = pkquad() ;
         }
         if (length <= 0)
            badpk("packet length too small") ;
         if (bytesleft < length) {
#ifdef DEBUG
             if (dd(D_FONTS))
                (void)fprintf(stderr,
                   "Allocating new raster memory (%d req, %d left)\n",
                                length, bytesleft) ;
#endif /* DEBUG */
             if (length > MINCHUNK) {
                tempr = (quarterword *)malloc((unsigned int)length) ;
                bytesleft = 0 ;
             } else {
                raster = (quarterword *)malloc(RASTERCHUNK) ;
                tempr = raster ;
                bytesleft = RASTERCHUNK - length ;
                raster += length ;
            }
            if (tempr == NULL)
               error("! out of memory while allocating raster") ;
         } else {
            tempr = raster ;
            bytesleft -= length ;
            raster += length ;
         }
         cd->packptr = tempr ;
         *tempr++ = cmd ;
         for (length--; length>0; length--)
            *tempr++ = pkbyte() ;
      } else {
         k = 0 ;
         switch (cmd) {
case 243:
            k = pkbyte() ;
            if (k > 127)
               k -= 256 ;
case 242:
            k = k * 256 + pkbyte() ;
case 241:
            k = k * 256 + pkbyte() ;
case 240:
            k = k * 256 + pkbyte() ;
            while (k-- > 0)
               i = pkbyte() ;
            break ;
case 244:
            k = pkquad() ;
            break ;
case 246:
            break ;
default:
            badpk("! unexpected command") ;
         }
      }
   }
   (void)fclose(pkfile) ;
   curfnt->loaded = 1 ;
}
