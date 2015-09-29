/*
 *   Here's the code to load a VF file into memory.
 *   Any resemblance between this file and loadfont.c is purely uncoincidental.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we use.
 */
extern void makefont() ;
extern void error() ;
extern integer scalewidth() ;
extern FILE *search() ;
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern long bytesleft ;
extern quarterword *raster ;
extern char *vfpath ;
extern char errbuf[200] ;
extern real conv ;
extern real alpha ;
extern integer mag ;
extern int actualdpi ;
extern char *nextstring, *maxstring ;
extern fontdesctype *fonthead ;
extern real alpha ;
/*
 *   We use malloc here.
 */
char *malloc() ;

/*
 *   Now we have some routines to get stuff from the VF file.
 *   Subroutine vfbyte returns the next byte.
 */

static FILE *vffile ;
static char name[50] ;
void
badvf(s)
   char *s ;
{
   (void)sprintf(errbuf,"! Bad VF file %s: %s",name,s) ;
   error(errbuf);
}

shalfword
vfbyte()
{
   register shalfword i ;

   if ((i=getc(vffile))==EOF)
      badvf("unexpected eof") ;
   return(i) ;
}

integer
vfquad()
{
   register integer i ;

   i = vfbyte() ;
   if (i > 127)
      i -= 256 ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   return(i) ;
}

integer
vftrio()
{
   register integer i ;

   i = vfbyte() ;
   i = i * 256 + vfbyte() ;
   i = i * 256 + vfbyte() ;
   return(i) ;
}

Boolean
vfopen(fd)
        register fontdesctype *fd ;
{
   register char *d, *n ;

   d = fd->area ;
   n = fd->name ;
   if (*d==0)
      d = vfpath ;
   (void)sprintf(name, "%s.vf", n) ;
   if (vffile=search(d, name))
      return(1) ;
   return(0) ;
}

/*
 * The following routine is like fontdef, but for local fontdefs in VF files.
 */
fontmaptype *
vfontdef(s)
      integer s ;
{
   register integer i, j, fn ;
   register fontdesctype *fp, *fpp ;
   register fontmaptype *cfnt ;

   fn = vfbyte() ;
   fp = (fontdesctype *)malloc(sizeof(fontdesctype)) ;
   cfnt = (fontmaptype *)malloc(sizeof(fontmaptype)) ;
   if (fp==NULL || cfnt==NULL)
      error("! ran out of memory") ;
   cfnt->fontnum = fn ;
   fp->psname = 0 ;
   fp->loaded = 0 ;
   fp->checksum = vfquad() ;
   fp->scaledsize = scalewidth(s,vfquad()) ;
   fp->designsize = (integer)(alpha * (real)vfquad()) ;
   fp->thinspace = fp->scaledsize / 6 ;
   fp->resfont = NULL ;
   fp->localfonts = NULL ;
   fp->dpi = (halfword)((float)mag*(float)fp->scaledsize*DPI/
         ((float)fp->designsize*1000.0)+0.5) ;
   i = vfbyte() ; j = vfbyte() ;
   if (nextstring + i + j > maxstring)
      error("! out of string space") ;
   fp->area = nextstring ;
   for (; i>0; i--)
      *nextstring++ = vfbyte() ;
   *nextstring++ = 0 ;
   fp->name = nextstring ;
   for (; j>0; j--)
      *nextstring++ = vfbyte() ;
   *nextstring++ = 0 ;
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Defining localfont (%s) %s at %.1fpt\n",
         fp->area, fp->name, (real)fp->scaledsize/(alpha*0x100000)) ;
#endif /* DEBUG */
   for (fpp=fonthead; fpp; fpp=fpp->next)
      if (fp->scaledsize==fpp->scaledsize &&
            strcmp(fp->name,fpp->name)==0 && strcmp(fp->area,fpp->area)==0) {
#ifdef DEBUG
         if (dd(D_FONTS))
            (void)fprintf(stderr,"(Already known.)\n") ;
#endif /* DEBUG */
         free((char *)fp) ;
         fp = fpp ;
         goto alreadyknown ;
      }
   fp->next = fonthead ;
   fonthead = fp ;
alreadyknown:
   cfnt->desc = fp ;
   return (cfnt) ;
}

/*
 *   Now our virtualfont routine.
 */
Boolean
virtualfont(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   register shalfword cmd ;
   register integer k ;
   register integer length ;
   register shalfword cc ;
   register chardesctype *cd ;
   integer scaledsize = curfnt->scaledsize ;
   register quarterword *tempr ;
   fontmaptype *fm, *newf ;

   if (!vfopen(curfnt))
      return (0) ;
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Loading virtual font %s at %.1fpt\n",
         name, (real)scaledsize/(alpha*0x100000)) ;
#endif /* DEBUG */

/*
 *   We clear out some pointers:
 */
   for (i=0; i<256; i++) {
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   if (vfbyte()!=247)
      badvf("expected pre") ;
   if (vfbyte()!=202)
      badvf("wrong id byte") ;
   for(i=vfbyte(); i>0; i--)
      (void)vfbyte() ;
   k = vfquad() ;
   if (k && curfnt->checksum)
      if (k!=curfnt->checksum) {
         (void)sprintf(errbuf,"Checksum mismatch in font %s", name) ;
         error(errbuf) ;
       }
   k = (integer)(alpha * (real)vfquad()) ;
   if (k > curfnt->designsize + 2 || k < curfnt->designsize - 2) {
      (void)sprintf(errbuf,"Design size mismatch in font %s", name) ;
      error(errbuf) ;
   }
/*
 * Now we look for font definitions.
 */
   fm = NULL ;
   while ((cmd=vfbyte())>=243) {
      if (cmd!=243)
         badvf("unexpected command in preamble") ;
      newf = vfontdef(scaledsize) ;
      if (fm)
         fm->next = newf ;
      else curfnt->localfonts = newf ;
      fm = newf ;
      fm->next = NULL ; /* FIFO */
   }
/*
 *   Now we get down to the serious business of reading character definitions.
 */
   do {
      if (cmd==242) {
         length = vfquad() + 2 ;
         if (length<2) badvf("negative length packet") ;
         if (length>65535) badvf("packet too long") ;
         cc = vfquad() ;
         if (cc<0 || cc>255) badvf("character code out of range") ;
         cd = curfnt->chardesc + cc ;
         cd->TFMwidth = scalewidth(vfquad(), scaledsize) ;
      } else {
         length = cmd + 2;
         cc = vfbyte() ;
         cd = curfnt->chardesc + cc ;
         cd->TFMwidth = scalewidth(vftrio(), scaledsize) ;
      }
      cd->pixelwidth = ((integer)(conv*cd->TFMwidth+0.5)) ;
      cd->flags = EXISTS ;
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
      length -= 2 ;
      *tempr++ = length / 256 ;
      *tempr++ = length % 256 ;
         for (; length>0; length--)
            *tempr++ = vfbyte() ;
      cmd = vfbyte() ;
   } while (cmd < 243) ;
   if (cmd != 248)
      badvf("missing postamble") ;
   (void)fclose(vffile) ;
   curfnt->loaded = 2 ;
   return (1) ;
}
