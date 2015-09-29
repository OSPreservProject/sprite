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
extern fontdesctype *baseFonts[] ;
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern int actualdpi ;
extern real alpha ;
extern fontmaptype *ffont ;
extern fontdesctype *fonthead ;
extern halfword dpicheck() ;
extern char *malloc() ;
extern integer fsizetol ;
/*
 * newfontdesc creates a new font descriptor with the given parameters and
 * returns a pointer to the new object.
*/
fontdesctype*
newfontdesc(cksum, scsize, dssize, name, area)
integer cksum, scsize, dssize ;
char *name, *area ;
{
   register fontdesctype *fp ;

   fp = (fontdesctype *)malloc(sizeof(fontdesctype)) ;
   if (fp == NULL)
      error("! ran out of memory") ;
   fp->psname = 0 ;
   fp->loaded = 0 ;
   fp->checksum = cksum ;
   fp->scaledsize = scsize ;
   fp->designsize = dssize ;
   fp->thinspace = scsize / 6 ;
   fp->scalename = NULL ;
   fp->psflag = 0 ;
   fp->name = name;
   fp->area = area;
   fp->resfont = NULL ;
   fp->localfonts = NULL ;
   fp->dpi = dpicheck((halfword)((float)mag*(float)fp->scaledsize*DPI/
         ((float)fp->designsize*1000.0)+0.5)) ;
   fp->loadeddpi = fp->dpi ;
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Defining font (%s) %s at %.1fpt\n",
         area, name, (real)scsize/(alpha*0x100000)) ;
#endif /* DEBUG */
   return fp ;
}
/*
 * Try to find a font with a given name and approximate scaled size, returning
 * NULL if unsuccessful.  If scname and the font's scalename are both
 * non-NULL they must match exactly.  If both are NULL, scsize and the
 * font's scaledsize come from the dvi file and should match exactly.
 * Otherwise there can be some slop due to the inaccuracies of sizes
 * read from an included psfile.
 */
fontdesctype *
matchfont(name, area, scsize, scname)
char *area, *name, *scname ;
integer scsize ;
{
   register fontdesctype *fp;
   register integer smin, smax;

   smin = scsize - fsizetol ;
   smax = scsize + fsizetol ;
   for (fp=fonthead; fp; fp=fp->next)
      if (smin < fp->scaledsize && fp->scaledsize < smax &&
            strcmp(name,fp->name)==0 && strcmp(area,fp->area)==0)
         if (scname == NULL) {
            if (fp->scalename!=NULL || scsize==fp->scaledsize)
               break ;
         } else {
            if (fp->scalename==NULL || strcmp(scname,fp->scalename)==0)
               break ;
         }
#ifdef DEBUG
   if (dd(D_FONTS) && fp)
      (void)fprintf(stderr,"(Already known.)\n") ;
#endif /* DEBUG */
   return fp;
}
/*
 *   fontdef takes a font definition in the dvi file and loads the data
 *   into its data structures.
 */
void
fontdef(siz)
int siz ;
{
   register integer i, j, fn ;
   register fontdesctype *fp ;
   register fontmaptype *cfnt ;
   char *name, *area ;
   integer cksum, scsize, dssize ;
   extern void skipover() ;

   fn = dvibyte() ;
   while (siz-- > 1)
      fn = (fn << 8) + dvibyte() ;
   for (cfnt=ffont; cfnt; cfnt = cfnt->next)
      if (cfnt->fontnum == fn) goto alreadydefined ;
   cfnt = (fontmaptype *)malloc(sizeof(fontmaptype)) ;
   if (cfnt==NULL)
      error("! ran out of memory") ;
   cfnt->next = ffont ;
   ffont = cfnt ;
   cfnt->fontnum = fn ;
   cksum = signedquad() ;
   scsize = signedquad() ;
   dssize = signedquad() ;
   i = dvibyte() ; j = dvibyte() ;
   if (nextstring + i + j > maxstring)
      error("! out of string space") ;
   area = nextstring ;
   for (; i>0; i--)
      *nextstring++ = dvibyte() ;
   *nextstring++ = 0 ;
   name = nextstring ;
   for (; j>0; j--)
      *nextstring++ = dvibyte() ;
   *nextstring++ = 0 ;
   fp = matchfont(name, area, scsize, NULL) ;
   if (fp) {
      nextstring = name ;
      fp->checksum = cksum ;
   } else {
      fp = newfontdesc(cksum, scsize, dssize, name, area) ;
      fp->next = fonthead ;
      fonthead = fp ;
   }
   cfnt->desc = fp ;
   if (fn < 256)
      baseFonts[fn] = fp ;
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
    if (cmd>=243 && cmd<=246) fontdef(cmd-242) ;
  return(cmd) ;
}
