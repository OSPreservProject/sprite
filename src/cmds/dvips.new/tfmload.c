/*
 *   Loads a tfm file.  It marks the characters as undefined.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   These are the external routines it calls:
 */
extern void error() ;
extern integer scalewidth() ;
extern FILE *search() ;
/*
 *   Here are the external variables we use:
 */
extern real conv ;
extern real vconv ;
extern real alpha ;
extern char *tfmpath ;
extern char errbuf[] ;
extern integer fsizetol ;
/*
 *   Our static variables:
 */
static FILE *tfmfile ; 
static char name[50] ;

/*
 *   Tries to open a tfm file.  Aborts job if unsuccessful.
 */
void
tfmopen(fd)
        register fontdesctype *fd ;
{
   register char *d, *n ;

   d = fd->area ;
   n = fd->name ;
   if (*d==0)
      d = tfmpath ;
   (void)sprintf(name, "%s.tfm", n) ;
   if ((tfmfile=search(d, name, READBIN))==NULL) {
      (void)sprintf(errbuf, "! Can't open font metric file %s%s",
             fd->area, name) ;
      error(errbuf) ;
   }
}

shalfword
tfmbyte ()
{
  return(getc(tfmfile)) ;
}

halfword
tfm16 ()
{
  register halfword a ; 
  a = tfmbyte () ; 
  return ( a * 256 + tfmbyte () ) ; 
} 

integer
tfm32 ()
{
  register integer a ; 
  a = tfm16 () ; 
  if (a > 32767) a -= 65536 ;
  return ( a * 65536 + tfm16 () ) ; 
} 

void
tfmload(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   register integer li ;
   integer scaledsize ;
   shalfword nw, hd ;
   shalfword bc, ec ;
   integer scaled[256] ;
   halfword chardat[256] ;

   tfmopen(curfnt) ;
/*
 *   Next, we read the font data from the tfm file, and store it in
 *   our own arrays.
 */
   li = tfm16() ; hd = tfm16() ;
   bc = tfm16() ; ec = tfm16() ;
   nw = tfm16() ;
   li = tfm32() ; li = tfm32() ; li = tfm32() ; li = tfm16() ;
   li = tfm32() ;
   if (li && curfnt->checksum)
      if (li!=curfnt->checksum) {
         (void)sprintf(errbuf,"Checksum mismatch in %s", name) ;
         error(errbuf) ;
       }
   li = (integer)(alpha * (real)tfm32()) ;
   if (li > curfnt->designsize + fsizetol ||
       li < curfnt->designsize - fsizetol) {
      (void)sprintf(errbuf,"Design size mismatch in %s", name) ;
      error(errbuf) ;
   }
   for (i=2; i<hd; i++)
      li = tfm32() ;
   for (i=0; i<256; i++)
      chardat[i] = 256 ;
   for (i=bc; i<=ec; i++) {
      chardat[i] = tfmbyte() ;
      li = tfm16() ;
      li = tfmbyte() ;
   }
   scaledsize = curfnt->scaledsize ;
   for (i=0; i<nw; i++)
      scaled[i] = scalewidth(tfm32(), scaledsize) ;
   (void)fclose(tfmfile) ;
   for (i=0; i<256; i++)
      if (chardat[i]!= 256) {
         li = scaled[chardat[i]] ;
         curfnt->chardesc[i].TFMwidth = li ;
         curfnt->chardesc[i].pixelwidth = ((integer)(conv*li+0.5)) ;
         curfnt->chardesc[i].flags = (curfnt->resfont ? EXISTS : 0) ;
      }
   curfnt->loaded = 1 ;
}
