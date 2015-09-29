/*
 *  Stores the data from a font definition into the global data structures.
 *  A routine skipnop is also included to handle nops and font definitions
 *  between pages.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines we call.
 */
extern shalfword dvibyte() ;
extern integer signedquad() ;
extern void error() ;
/*
 *   The external variables we use:
 */
extern char *nextstring, *maxstring ;
extern integer mag ;
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern int actualdpi ;
extern real alpha ;
extern fontmaptype *ffont ;
extern fontdesctype *fonthead ;
/*
 *   We use malloc here:
 */
char *malloc() ;
/*
 *   fontdef takes a font definition in the dvi file and loads the data
 *   into its data structures.
 */
void
fontdef()
{
   register integer i, j, fn ;
   register fontdesctype *fp, *fpp ;
   register fontmaptype *cfnt ;

   fn = dvibyte() ;
   for (cfnt=ffont; cfnt; cfnt = cfnt->next)
      if (cfnt->fontnum == fn) goto alreadydefined ;
   fp = (fontdesctype *)malloc(sizeof(fontdesctype)) ;
   cfnt = (fontmaptype *)malloc(sizeof(fontmaptype)) ;
   if (fp==NULL || cfnt==NULL)
      error("! ran out of memory") ;
   cfnt->next = ffont ;
   ffont = cfnt ;
   cfnt->fontnum = fn ;
   fp->psname = 0 ;
   fp->loaded = 0 ;
   fp->checksum = signedquad() ;
   fp->scaledsize = signedquad() ;
   fp->designsize = signedquad() ;
   fp->thinspace = fp->scaledsize / 6 ;
   fp->resfont = NULL ;
   fp->localfonts = NULL ;
   fp->dpi = (halfword)((float)mag*(float)fp->scaledsize*DPI/
         ((float)fp->designsize*1000.0)+0.5) ;
   i = dvibyte() ; j = dvibyte() ;
   if (nextstring + i + j > maxstring)
      error("! out of string space") ;
   fp->area = nextstring ;
   for (; i>0; i--)
      *nextstring++ = dvibyte() ;
   *nextstring++ = 0 ;
   fp->name = nextstring ;
   for (; j>0; j--)
      *nextstring++ = dvibyte() ;
   *nextstring++ = 0 ;
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Defining font (%s) %s at %.1fpt\n",
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
   return ;
alreadydefined:
/* A DVI file will not define a font twice; but we may be scanning
 * a font definition twice because a new section has started,
 * or because of collated copies. */
      skipover(12) ;
      skipover(dvibyte()+dvibyte()) ;
}

/*
 *   The next routine handles nops or font definitions between pages in a
 *   dvi file.  Returns the first command that is not a nop or font definition.
 */
int
skipnop()
{
  register int cmd ;
  while ((cmd=dvibyte())==138||cmd==243)
    if (cmd==243) fontdef() ;
  return(cmd) ;
}
