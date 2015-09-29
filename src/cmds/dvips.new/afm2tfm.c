/*
 *   This program converts AFM files to TeX TFM files, and optionally
 *   to TeX VPL files that retain all kerning and ligature information.
 *   Both files make the characters not normally encoded by TeX available
 *   by character codes greater than 127.
 */

/*   (Modified by Don Knuth from Tom Rokicki's pre-VPL version.) */

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#ifdef VMS
#include <string.h>
#else
#include <strings.h>
#endif
#endif
#include <math.h>

#ifdef MSDOS
#define WRITEBIN "wb"
#else
#define WRITEBIN "w"
#endif

char *texencoding[] = {
   "Gamma", "Delta", "Theta", "Lambda", "Xi", "Pi", "Sigma",
   "Upsilon", "Phi", "Psi", "Omega", "arrowup", "arrowdown", "quotesingle",
   "exclamdown", "questiondown", "dotlessi", "dotlessj", "grave", "acute",
   "caron", "breve", "macron", "ring", "cedilla", "germandbls", "ae", "oe",
   "oslash", "AE", "OE", "Oslash", "space", "exclam", "quotedbl", "numbersign",
   "dollar", "percent", "ampersand", "quoteright", "parenleft", "parenright",
   "asterisk", "plus", "comma", "hyphen", "period", "slash", "zero", "one",
   "two", "three", "four", "five", "six", "seven", "eight", "nine", "colon",
   "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C",
   "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R",
   "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft", "backslash",
   "bracketright", "circumflex", "underscore", "quoteleft", "a", "b", "c", "d",
   "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
   "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar", "braceright",
   "tilde", "dieresis" } ;
/*
 *   The above layout corresponds to TeX Typewriter Type and is compatible
 *   with TeX Text because the position of ligatures is immaterial.
 */

/*
 *   This is what we store Adobe data in.
 */
struct adobeinfo {
   struct adobeinfo *next ;
   int adobenum, texnum, width ;
   char *adobename ;
   int llx, lly, urx, ury ;
   struct lig *ligs ;
   struct kern *kerns ;
   struct pcc *pccs ;
   int wptr, hptr, dptr, iptr ;
} *adobechars, *adobeptrs[256], *texptrs[256],
  *uppercase[256], *lowercase[256] ;
struct lig {
   struct lig *next ;
   char *succ, *sub ;
} ;
struct kern {
   struct kern *next ;
   char *succ ;
   int delta ;
} ;
struct pcc {
   struct pcc *next ;
   char * partname ;
   int xoffset, yoffset ;
} ;

FILE *afmin, *vplout, *tfmout ;
char inname[200], outname[200] ; /* names of input and output files */
char buffer[255]; /* input buffer (modified while parsing) */
char obuffer[255] ; /* unmodified copy of input buffer */
char *param ; /* current position in input buffer */
char *fontname = "Unknown" ;
char *codingscheme = "Unspecified" ;
float italicangle = 0.0 ;
char fixedpitch ;
char makevpl ;
int xheight = 400 ;
int fontspace ;
int bc, ec ;
long cksum ;
float efactor = 1.0, slant = 0.0 ;
double newslant ;
char titlebuf[100] ;

void
error(s)
register char *s ;
{
   extern void exit() ;

   (void)fprintf(stderr, "%s\n", s) ;
   if (obuffer[0]) {
      (void)fprintf(stderr, "%s\n", obuffer) ;
      while (param > buffer) {
         (void)fprintf(stderr, " ") ;
         param-- ;
      }
      (void)fprintf(stderr, "^\n") ;
   }
   if (*s == '!')
      exit(1) ;
}

int
transform(x,y)
   register int x,y ;
{
   register double acc ;
   acc = efactor * x + slant *y ;
   return (int)(acc>=0? acc+0.5 : acc-0.5 ) ;
}

int
getline() {
   register char *p ;
   register int c ;

   param = buffer ;
   for (p=buffer; (c=getc(afmin)) != EOF && c != 10;)
      *p++ = c ;
   *p = 0 ;
   (void)strcpy(obuffer, buffer) ;
   if (p == buffer && c == EOF)
      return(0) ;
   else
      return(1) ;
}

char *interesting[] = { "FontName", "ItalicAngle", "IsFixedPitch",
   "XHeight", "C", "KPX", "CC", "EncodingScheme", NULL} ; 
#define FontName (0)
#define ItalicAngle (1)
#define IsFixedPitch (2)
#define XHeight (3)
#define C (4)
#define KPX (5)
#define CC (6)
#define EncodingScheme (7)
#define NONE (-1)
int
interest(s)
char *s ;
{
   register char **p ;
   register int n ;

   for (p=interesting, n=0; *p; p++, n++)
      if (strcmp(s, *p)==0)
         return(n) ;
   return(NONE) ;
}

char *
mymalloc(len)
int len ;
{
   register char *p ;
   extern char *malloc() ;

   p = malloc((unsigned)len) ;
   if (p==NULL)
      error("! out of memory") ;
   return(p) ;
}

char *
paramnewstring() {
   register char *p, *q ;

   p = param ;
   while (*p > ' ')
      p++ ;
   q = mymalloc((int)(p-param+1)) ;
   if (*p != 0)
      *p++ = 0 ;
   (void)strcpy(q, param) ;
   while (*p && *p <= ' ')
      p++ ;
   param = p ;
   return(q) ;
}

char *
paramstring() {
   register char *p, *q ;

   p = param ;
   while (*p > ' ')
      p++ ;
   q = param ;
   if (*p != 0)
      *p++ = 0 ;
   while (*p && *p <= ' ')
      p++ ;
   param = p ;
   return(q) ;
}

int
paramnum() {
   register char *p ;
   int i ;

   p = paramstring() ;
   if (sscanf(p, "%d", &i) != 1)
      error("! integer expected") ;
   return(i) ;
}

float
paramfloat() {
   register char *p ;
   float i ;

   p = paramstring() ;
   if (sscanf(p, "%f", &i) != 1)
      error("! number expected") ;
   return(i) ;
}

struct adobeinfo *
newchar() {
   register struct adobeinfo *ai ;

   ai = (struct adobeinfo *)mymalloc(sizeof(struct adobeinfo)) ;
   ai->adobenum = -1 ;
   ai->texnum = -1 ;
   ai->width = -1 ;
   ai->adobename = NULL ;
   ai->llx = -1 ;
   ai->lly = -1 ;
   ai->urx = -1 ;
   ai->ury = -1 ;
   ai->ligs = NULL ;
   ai->kerns = NULL ;
   ai->pccs = NULL ;
   ai->next = adobechars ;
   adobechars = ai ;
   return(ai) ;
}

struct kern *
newkern() {
   register struct kern *nk ;

   nk = (struct kern *)mymalloc(sizeof(struct kern)) ;
   nk->next = NULL ;
   nk->succ = NULL ;
   nk->delta = 0 ;
   return(nk) ;
}

struct pcc *
newpcc() {
   register struct pcc *np ;

   np = (struct pcc *)mymalloc(sizeof(struct pcc)) ;
   np->next = NULL ;
   np->partname = NULL ;
   np->xoffset = 0 ;
   np->yoffset = 0 ;
   return(np) ;
}

struct lig *
newlig() {
   register struct lig *nl ;

   nl = (struct lig *)mymalloc(sizeof(struct lig)) ;
   nl->next = NULL ;
   nl->succ = NULL ;
   nl->sub = NULL ;
   return(nl) ;
}

void
expect(s)
char *s ;
{
   if (strcmp(paramstring(), s) != 0) {
      (void)fprintf(stderr, "%s expected: ", s) ;
      error("! syntax error") ;
   }
}

void
handlechar() { /* an input line beginning with C */
   register struct adobeinfo *ai ;
   register struct lig *nl ;

   ai = newchar() ;
   ai->adobenum = paramnum() ;
   expect(";") ;
   expect("WX") ;
   ai->width = transform(paramnum(),0) ;
   if (ai->adobenum >= 0 && ai->adobenum < 256) {
      adobeptrs[ai->adobenum] = ai ;
      cksum = (cksum<<1) ^ ai->width ;
   }
   expect(";") ;
   expect("N") ;
   ai->adobename = paramnewstring() ;
   expect(";") ;
   expect("B") ;
   ai->llx = paramnum() ;
   ai->lly = paramnum() ;
   ai->llx = transform(ai->llx, ai->lly) ;
   ai->urx = paramnum() ;
   ai->ury = paramnum() ;
   ai->urx = transform(ai->urx, ai->ury) ;
/* We need to avoid negative heights or depths. They break accents in
   math mode, among other things.  */
   if (ai->lly > 0)
      ai->lly = 0 ;
   if (ai->ury < 0)
      ai->ury = 0 ;
   expect(";") ;
/* Now look for ligatures (which aren't present in fixedpitch fonts) */
   while (*param == 'L') {
      expect("L") ;
      nl = newlig() ;
      nl->succ = paramnewstring() ;
      nl->sub = paramnewstring() ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
      expect(";") ;
   }
   if (strcmp(ai->adobename, "space")==0) {
      fontspace = ai->width ;
      nl = newlig() ;        /* space will act as zero-width Polish crossbar */
      nl->succ = "l" ;       /* when used by plain TeX's \l or \L macros */
      nl->sub = "lslash" ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
      nl = newlig() ;
      nl->succ = "L" ;
      nl->sub = "Lslash" ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
   } else if (strcmp(ai->adobename, "question")==0) {
      nl = newlig() ;
      nl->succ = "quoteleft" ;
      nl->sub = "questiondown" ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
   } else if (strcmp(ai->adobename, "exclam")==0) {
      nl = newlig() ;
      nl->succ = "quoteleft" ;
      nl->sub = "exclamdown" ;
      nl->next = ai->ligs ;
      ai->ligs = nl ;
   } else if (! fixedpitch) {
      if (strcmp(ai->adobename, "hyphen")==0) {
         nl = newlig() ;
         nl->succ = "hyphen" ;
         nl->sub = "endash" ;
         nl->next = ai->ligs ;
         ai->ligs = nl ;
      } else if (strcmp(ai->adobename, "endash")==0) {
         nl = newlig() ;
         nl->succ = "hyphen" ;
         nl->sub = "emdash" ;
         nl->next = ai->ligs ;
         ai->ligs = nl ;
      } else if (strcmp(ai->adobename, "quoteleft")==0) {
         nl = newlig() ;
         nl->succ = "quoteleft" ;
         nl->sub = "quotedblleft" ;
         nl->next = ai->ligs ;
         ai->ligs = nl ;
      } else if (strcmp(ai->adobename, "quoteright")==0) {
         nl = newlig() ;
         nl->succ = "quoteright" ;
         nl->sub = "quotedblright" ;
         nl->next = ai->ligs ;
         ai->ligs = nl ;
      }
   }
}

struct adobeinfo *
findadobe(p)
char *p ;
{
   register struct adobeinfo *ai ;

   for (ai=adobechars; ai; ai = ai->next)
      if (strcmp(p, ai->adobename)==0)
         return(ai) ;
   return(NULL) ;
}

/* We ignore kerns before and after space characters, because (1) TeX
 * is using the space only for Polish ligatures, and (2) TeX's
 * boundarychar mechanisms are not oriented to kerns (they apply
 * to both spaces and punctuation) so we don't want to use them. */
void
handlekern() { /* an input line beginning with KPX */
   register struct adobeinfo *ai ;
   register char *p ;
   register struct kern *nk ;

   p = paramstring() ;
   if (strcmp(p,"space")==0) return ;
   ai = findadobe(p) ;
   if (ai == NULL)
      error("! kern char not found") ;
   if (ai->adobenum<'0' || ai->adobenum>'9') {
/* Ignore kerns after digits because they would mess up our nice tables */
      nk = newkern() ;
      nk->succ = paramnewstring() ;
      if (strcmp(nk->succ,"space")==0) return ;
      nk->delta = transform(paramnum(),0) ;
      nk->next = ai->kerns ;
      ai->kerns = nk ;
    }
}

void
handleconstruct() { /* an input line beginning with CC */
   register struct adobeinfo *ai ;
   register char *p ;
   register struct pcc *np ;
   register int n ;
   struct pcc *npp = NULL;

   p = paramstring() ;
   ai = findadobe(p) ;
   if (ai == NULL)
      error("! composite character name not found") ;
   n = paramnum() ;
   expect(";") ;
   while (n--) {
      if (strcmp(paramstring(),"PCC") != 0) return ;
        /* maybe I should expect("PCC") instead, but I'm playing it safe */
      np = newpcc() ;
      np->partname = paramnewstring() ;
      if (findadobe(np->partname)==NULL) return ;
      np->xoffset = paramnum() ;
      np->yoffset = paramnum() ;
      np->xoffset = transform(np->xoffset, np->yoffset) ;
      if (npp) npp->next = np ;
      else ai->pccs = np ;
      npp = np ;
      expect(";") ;
   }
}

void
makeaccentligs() {
   register struct adobeinfo *ai, *aci ;
   register char *p ;
   register struct lig *nl ;
   for (ai=adobechars; ai; ai=ai->next) {
      p = ai->adobename ;
      if (strlen(p)>2)
         if ((aci=findadobe(p+1)) && (aci->adobenum > 127)) {
            nl = newlig() ;
            nl->succ = mymalloc(2) ;
            *(nl->succ + 1) = 0 ;
            *(nl->succ) = *p ;
            nl->sub = ai->adobename ;
            nl->next = aci->ligs ;
            aci->ligs = nl ;
         }
   }
}

void
readadobe() {
   while (getline()) {
      switch(interest(paramstring())) {
case FontName:
         fontname = paramnewstring() ;
         break ;
case EncodingScheme:
         codingscheme = paramnewstring() ;
         break ;
case ItalicAngle:
         italicangle = paramfloat() ;
         break ;
case IsFixedPitch:
         if (*param == 't' || *param == 'T')
            fixedpitch = 1 ;
         else
            fixedpitch = 0 ;
         break ;
case XHeight:
         xheight = paramnum() ;
         break ;
case C:
         handlechar() ;
         break ;
case KPX:
         handlekern() ;
         break ;
case CC:
         handleconstruct() ;
         break ;
default:
         break ;
      }
   }
   makeaccentligs() ;
}

void
assignchars() {
   register char **p ;
   register int i ;
   register struct adobeinfo *ai ;
   int nextfree = 128 ;

/*
 *   First, we assign all those that match perfectly.
 */
   for (i=0, p=texencoding; i<128; i++, p++)
      if ((ai=findadobe(*p)) && ai->adobenum >= 0) {
         ai->texnum = i ;
         texptrs[i] = ai ;
      }
/*
 *   Next, we assign all the others whose codes are above 127, retaining
 *   the adobe positions. */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->adobenum > 127 && ai->texnum<0) {
         ai->texnum = ai->adobenum ;
         texptrs[ai->adobenum] = ai ;
      }
/*   Finally, we map all remaining characters into free locations beginning
 *   with 128, if we know how to construct those characters. */
   for (ai=adobechars; ai; ai=ai->next)
      if (ai->texnum<0 && (ai->adobenum>=0 || ai->pccs !=NULL)) {
         while (texptrs[nextfree]) {
            nextfree=(nextfree+1)&255 ;
            if (nextfree==128) return ; /* all slots full */
         }
         ai->texnum = nextfree ;
         texptrs[nextfree] = ai ;
      }
}

void
upmap() { /* Compute uppercase mapping, when making a small caps font */
   register struct adobeinfo *ai, *Ai ;
   register char *p, *q ;
   register struct pcc *np, *nq ;
   char lwr[50] ;

   for (Ai=adobechars; Ai; Ai=Ai->next) {
      p = Ai->adobename ;
      if (*p>='A' && *p<='Z') {
         q = lwr ;
         for (; *p; p++)
            *q++ = ((*p>='A' && *p<='Z') ? *p+32 : *p) ;
         *q = 0;
         if (ai=findadobe(lwr)) {
            if (ai->texnum>=0) uppercase[ai->texnum] = Ai ;
            if (Ai->texnum>=0) lowercase[Ai->texnum] = ai ;
         }
      }
   }
/* Note that, contrary to the normal true/false conventions,
 * uppercase[i] is NULL and lowercase[i] is non-NULL when i is the
 * ASCII code of an uppercase letter; and vice versa for lowercase letters */

   if (ai=findadobe("germandbls"))
      if (Ai=findadobe("S")) { /* we also construct SS */
         uppercase[ai->texnum] = ai ;
         ai->width = Ai->width << 1 ;
         ai->llx = Ai->llx ;
         ai->lly = Ai->lly ;
         ai->urx = Ai->width + Ai->urx ;
         ai->ury = Ai->ury ;
         ai->kerns = Ai->kerns ;
         np = newpcc() ;
         np->partname = "S" ;
         nq = newpcc() ;
         nq->partname = "S" ;
         nq->xoffset = Ai->width ;
         np->next = nq ;   
         ai->pccs = np ;
      }
   if (ai=findadobe("dotlessi"))
      uppercase[ai->texnum] = findadobe("I") ;
   if (ai=findadobe("dotlessj"))
      uppercase[ai->texnum] = findadobe("J") ;
}
/* The logic above seems to work well enough, but it leaves useless characters
 * like `fi' and `fl' in the font if they were present initially,
 * and it omits characters like `dotlessj' if they are absent initially */

/* Now we turn to computing the TFM file */

int lf, lh, nw, nh, nd, ni, nl, nk, ne, np ;

void
write16(what)
register short what ;
{
   (void)fputc(what >> 8, tfmout) ;
   (void)fputc(what & 255, tfmout) ;
}

void
writearr(p, n)
register long *p ;
register int n ;
{
   while (n) {
      write16((short)(*p >> 16)) ;
      write16((short)(*p & 65535)) ;
      p++ ;
      n-- ;
   }
}

void
makebcpl(p, s, n)
register long *p ;
register char *s ;
register int n ;
{
   register long t ;
   register long sc ;

   if (strlen(s) < n)
      n = strlen(s) ;
   t = ((long)n) << 24 ;
   sc = 16 ;
   while (n > 0) {
      t |= ((long)(*(unsigned char *)s++)) << sc ;
      sc -= 8 ;
      if (sc < 0) {
         *p++ = t ;
         t = 0 ;
         sc = 24 ;
      }
      n-- ;
   }
   *p++ = t ;
}

int source[257] ;
int unsort[257] ;

/*
 *   Next we need a routine to reduce the number of distinct dimensions
 *   in a TFM file. Given an array what[0]...what[oldn-1], we want to
 *   group its elements into newn clusters, in such a way that the maximum
 *   difference between elements of a cluster is as small as possible.
 *   Furthermore, what[0]=0, and this value must remain in a cluster by
 *   itself. Data such as `0 4 6 7 9' with newn=3 shows that an iterative
 *   scheme in which 6 is first clustered with 7 will not work. So we
 *   borrow a neat algorithm from METAFONT to find the true optimum.
 *   Memory location what[oldn] is set to 0x7fffffffL for convenience.
 */
long nextd ; /* smallest value that will give a different mincover */
int
mincover(what,d) /* tells how many clusters result, given max difference d */
register long d ;
long *what ;
{
   register int m ;
   register long l ;
   register long *p ;

   nextd = 0x7fffffffL ;
   p = what+1 ;
   m = 1 ;
   while (*p<0x7fffffffL) {
      m++ ;
      l = *p ;
      while (*++p <= l+d) ;
      if (*p-l < nextd) nextd = *p-l ;
   }
   return (m) ;
}

void
remap(what, oldn, newn)
long *what ;
int oldn, newn ;
{
   register int i, j ;
   register long d, l ;

   what[oldn] = 0x7fffffffL ;
   for (i=oldn-1; i>0; i--) {
      d = what[i] ;
      for (j=i; what[j+1]<d; j++) {
         what[j] = what[j+1] ;
         source[j] = source[j+1] ;
      }
      what[j] = d ;
      source[j] = i ;
   } /* Tom, don't let me ever catch you using bubblesort again! -- Don */

   i = mincover(what, 0L) ;
   d = nextd ;
   while (mincover(what,d+d)>newn) d += d ;
   while (mincover(what,d)>newn) d = nextd ;

   i = 1 ;
   j = 0 ;
   while (i<oldn) {
      j++ ;
      l = what[i] ;
      unsort[source[i]] = j ;
      while (what[++i] <= l+d) {
         unsort[source[i]] = j ;
         if (i-j == oldn-newn) d = 0 ;
      }
      what[j] = (l+what[i-1])/2 ;
   }
}

/*
 *   The next routine simply scales something.
 *   Input is in 1000ths of an em.  Output is in FIXFACTORths of 1000.
 */
#define FIXFACTOR (0x100000L) /* 2^{20}, the unit fixnum */
long
scale(what)
long what ;
{
   return(((what / 1000) << 20) +
          (((what % 1000) << 20) + 500) / 1000) ;
}

long *header, *charinfo, *width, *height, *depth, *ligkern, *kern, *tparam,
     *italic ;
long tfmdata[10000] ;

void
buildtfm() {
   register int i, j ;
   register struct adobeinfo *ai ;

   header = tfmdata ;
   header[0] = cksum ;
   header[1] = 0xa00000 ; /* 10pt design size */
   makebcpl(header+2, codingscheme, 39) ;
   makebcpl(header+12, fontname, 19) ;
   lh = 17 ;
   charinfo = header + lh ;

   for (i=0; i<256 && adobeptrs[i]==NULL; i++) ;
   bc = i ;
   for (i=255; i>=0 && adobeptrs[i]==NULL; i--) ;
   ec = i;
   if (ec < bc)
      error("! no Adobe characters") ;

   width = charinfo + (ec - bc + 1) ;
   width[0] = 0 ;
   nw++ ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         width[nw]=ai->width ;
         for (j=1; width[j]!=ai->width; j++) ;
         ai->wptr = j ;
         if (j==nw)
            nw++ ;
      }
   if (nw>256)
      error("! 256 chars with different widths") ;
   depth = width + nw ;
   depth[0] = 0 ;
   nd = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         depth[nd] = -ai->lly ;
         for (j=0; depth[j]!=-ai->lly; j++) ;
         ai->dptr = j ;
         if (j==nd)
            nd++ ;
      }
   if (nd > 16) {
      remap(depth, nd, 16) ;
      nd = 16 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->dptr = unsort[ai->dptr] ;
   }
   height = depth + nd ;
   height[0] = 0 ;
   nh = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         height[nh]=ai->ury ;
         for (j=0; height[j]!=ai->ury; j++) ;
         ai->hptr = j ;
         if (j==nh)
            nh++ ;
      }
   if (nh > 16) {
      remap(height, nh, 16) ;
      nh = 16 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->hptr = unsort[ai->hptr] ;
   }
   italic  = height + nh ;
   italic[0] = 0 ;
   ni = 1 ;
   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i]) {
         italic[ni] = ai->urx - ai->width ;
         if (italic[ni]<0)
            italic[ni] = 0 ;
         for (j=0; italic[j]!=italic[ni]; j++) ;
         ai->iptr = j ;
         if (j==ni)
            ni++ ;
      }
   if (ni > 64) {
      remap(italic, ni, 64) ;
      ni = 64 ;
      for (i=bc; i<=ec; i++)
         if (ai=adobeptrs[i])
            ai->iptr = unsort[ai->iptr] ;
   }

   for (i=bc; i<=ec; i++)
      if (ai=adobeptrs[i])
         charinfo[i-bc] = ((long)(ai->wptr)<<24) +
                           ((long)(ai->hptr)<<20) +
                            ((long)(ai->dptr)<<16) +
                             ((long)(ai->iptr)<<10) ;

   ligkern = italic + ni ;
   nl = 0 ; /* ligatures and kerns omitted from raw Adobe font */
   kern = ligkern + nl ;
   nk = 0 ;

   newslant = (double)slant - efactor * tan(italicangle*(3.1415926535/180.0)) ;
   tparam = kern + nk ;
   tparam[0] = (long)(FIXFACTOR * newslant + 0.5) ;
   tparam[1] = scale((long)(fontspace*efactor+0.5)) ;
   tparam[2] = (fixedpitch ? 0 : scale((long)(300*efactor+0.5))) ;
   tparam[3] = (fixedpitch ? 0 : scale((long)(100*efactor+0.5))) ;
   tparam[4] = scale((long)xheight) ;
   tparam[5] = scale((long)(1000*efactor+0.5)) ;
   np = 6 ;
}

void
writesarr(what, len)
long *what ;
int len ;
{
   register long *p ;
   int i ;

   p = what ;
   i = len ;
   while (i) {
      *p = scale(*p) ;
      p++ ;
      i-- ;
   }
   writearr(what, len) ;
}

void
writetfm() {
   lf = 6 + lh + (ec - bc + 1) + nw + nh + nd + ni + nl + nk + ne + np ;
   write16(lf) ;
   write16(lh) ;
   write16(bc) ;
   write16(ec) ;
   write16(nw) ;
   write16(nh) ;
   write16(nd) ;
   write16(ni) ;
   write16(nl) ;
   write16(nk) ;
   write16(ne) ;
   write16(np) ;
   writearr(header, lh) ;
   writearr(charinfo, ec-bc+1) ;
   writesarr(width, nw) ;
   writesarr(height, nh) ;
   writesarr(depth, nd) ;
   writesarr(italic, ni) ;
   writearr(ligkern, nl) ;
   writesarr(kern, nk) ;
   writearr(tparam, np) ;
}

/* OK, the TFM file is done! Now for our next trick, the VPL file. */

/* For TeX we want to compute a character height that works properly
 * with accents. The following list of accents doesn't need to be complete. */
char *accents[] = { "acute", "tilde", "caron", "dieresis", NULL} ;
int
texheight(ai)
register struct adobeinfo *ai ;
{
   register char **p;
   register struct adobeinfo *aci, *acci ;
   if (*(ai->adobename + 1)) return (ai->ury) ; /* that was the simple case */
   for (p=accents; *p; p++)  /* otherwise we look for accented letters */
      if (aci=findadobe(*p)) {
         strcpy(buffer,ai->adobename) ;
         strcat(buffer,*p) ;
         if (acci=findadobe(buffer)) return (acci->ury - aci->ury + xheight) ;
      }
   return (ai->ury) ;
}

/* modified tgr to eliminate varargs problems */

#define vout(s)  fprintf(vplout, s)
int level ; /* the depth of parenthesis nesting in VPL file being written */

void vlevout() {
   register int l = level ;
   while (l--) vout("   ") ;
}

void vlevnlout() {
   vout("\n") ;
   vlevout() ;
}

#define voutln(str) {fprintf(vplout,"%s\n",str);vlevout();}
/*
void
voutln(s)
char *s ;
{
   register char l ;
   fprintf(vplout,"%s\n", s) ;
   for (l=level; l; l--) vout("   ") ;
}
*/
#define voutln2(f,s) {fprintf(vplout,f,s);vlevnlout();}
/*
void
voutln2(f,s)
char *f, *s ;
{
   register char l ;
   (void) sprintf(buffer, f, s) ;
   fprintf(vplout,"%s\n", buffer) ;
   for (l=level; l; l--) vout("   ") ;
}
*/
#define voutln3(f,a,b) {fprintf(vplout,f,a,b);vlevnlout();}
/*
void
voutln3(f,s1,s2)
char *f, *s1, *s2 ;
{
   register char l ;
   (void) sprintf(buffer, f, s1, s2) ;
   fprintf(vplout,"%s\n", buffer) ;
   for (l=level; l; l--) vout("   ") ;
}
*/
void
vleft()
{
   level++ ;
   vout("(") ;
}

void
vright()
{
   level-- ;
   voutln(")") ;
}

char vcharbuf[6] ;
char *vchar(c)
int c ;
{
   if ((c>='0' && c<='9')||(c>='A' && c<='Z')||(c>='a' && c<='z'))
      (void) sprintf(vcharbuf,"C %c", c) ;
   else (void) sprintf(vcharbuf,"O %o", c) ;
   return (vcharbuf) ;
}

void
writevpl()
{
   register int i ;
   register struct adobeinfo *ai ;
   register struct lig *nlig ;
   register struct kern *nkern ;
   register struct pcc *npcc ;
   struct adobeinfo *asucc, *asub, *api ;
   int xoff, yoff, ht ;
   char unlabeled ;

   voutln2("(VTITLE Created by %s)", titlebuf) ;
   voutln("(COMMENT Please edit that VTITLE if you edit this file)") ;
   (void)sprintf(obuffer, "TeX-%s%s%s%s", outname,
      (efactor==1.0? "" : "-E"), (slant==0.0? "" : "-S"),
                 (makevpl==1? "" : "-CSC")) ;
   if (strlen(obuffer)>19) { /* too long, will retain first 9 and last 10 */
      register char *p, *q ;
      for (p = &obuffer[9], q = &obuffer[strlen(obuffer)-10] ; p<&obuffer[19];
              p++, q++) *p = *q ;
      obuffer[19] = '\0' ;
   }
   voutln2("(FAMILY %s)" , obuffer) ;
   voutln2("(CODINGSCHEME TeX text + %s)", codingscheme) ;
   voutln("(DESIGNSIZE R 10.0)") ;
   voutln("(DESIGNUNITS R 1000)") ;
   voutln("(COMMENT DESIGNSIZE (1 em) IS IN POINTS)") ;
   voutln("(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/1000)") ;
   voutln2("(CHECKSUM O %lo)",cksum ^ 0xffffffff) ;
   vleft() ; voutln("FONTDIMEN") ;
   if (newslant)
      voutln2("(SLANT R %f)", newslant) ;
   voutln2("(SPACE D %d)", transform(fontspace,0)) ;
   if (! fixedpitch) {
      voutln2("(STRETCH D %d)", transform(200,0)) ;
      voutln2("(SHRINK D %d)", transform(100,0)) ;
   }
   voutln2("(XHEIGHT D %d)", xheight) ;
   voutln2("(QUAD D %d)", transform(1000,0)) ;
   voutln2("(EXTRASPACE D %d)", transform(fixedpitch ? fontspace : 111, 0)) ;
   vright() ;
   vleft() ; voutln("MAPFONT D 0");
   voutln2("(FONTNAME %s)", outname) ;
   voutln2("(FONTCHECKSUM O %lo)", cksum) ;
   vright() ;
   if (makevpl>1) {
      vleft() ; voutln("MAPFONT D 1");
      voutln2("(FONTNAME %s)", outname) ;
      voutln("(FONTAT D 800)") ;
      voutln2("(FONTCHECKSUM O %lo)", cksum) ;
      vright() ;
   }

   for (i=0; i<256 && texptrs[i]==NULL; i++) ;
   bc = i ;
   for (i=255; i>=0 && texptrs[i]==NULL; i--) ;
   ec = i;

   vleft() ; voutln("LIGTABLE") ;
   for (i=bc; i<=ec; i++)
      if (ai=texptrs[i]) {
         unlabeled = 1 ;
         if (uppercase[i]==NULL) /* omit ligatures from smallcap lowercase */
            for (nlig=ai->ligs; nlig; nlig=nlig->next)
               if (asucc=findadobe(nlig->succ))
                  if (asucc->texnum>=0)
                     if (asub=findadobe(nlig->sub))
                        if (asub->texnum>=0) {
                           if (unlabeled) {
                              voutln2("(LABEL %s)", vchar(i)) ;
                              unlabeled = 0 ;
                           }
                           voutln3("(LIG %s O %o)", vchar(asucc->texnum),
                                asub->texnum) ;
                        }
         for (nkern = (uppercase[i] ? uppercase[i]->kerns : ai->kerns);
                    nkern; nkern=nkern->next)
            if (asucc=findadobe(nkern->succ))
               if (asucc->texnum>=0)
                  if (uppercase[asucc->texnum]==NULL) {
                     if (unlabeled) {
                        voutln2("(LABEL %s)", vchar(i)) ;
                        unlabeled = 0 ;
                     }
                     if (uppercase[i]) {
                        if (lowercase[asucc->texnum]) {
                           voutln3("(KRN %s R %.1f)",
                                 vchar(lowercase[asucc->texnum]->texnum),
                                 0.8*nkern->delta) ;
                        } else voutln3("(KRN %s R %.1f)",
                                 vchar(asucc->texnum), 0.8*nkern->delta) ;
                     } else {
                        voutln3("(KRN %s R %d)", vchar(asucc->texnum),
                                nkern->delta) ;
                        if (lowercase[asucc->texnum])
                           if (lowercase[asucc->texnum]->texnum>=0)
                              voutln3("(KRN %s R %.1f)",
                                vchar(lowercase[asucc->texnum]->texnum),
                                0.8*nkern->delta) ;
                     }
                  }
         if (! unlabeled) voutln("(STOP)") ;
      }
   vright() ;

   for (i=bc; i<=ec; i++)
      if (ai=texptrs[i]) {
         vleft() ; fprintf(vplout, "CHARACTER %s", vchar(i)) ;
         if (*vcharbuf=='C') {
            voutln("") ;
         } else
            voutln2(" (comment %s)", ai->adobename) ;
         if (uppercase[i]) {
            ai=uppercase[i] ;
            voutln2("(CHARWD R %.1f)", 0.8 * (ai->width)) ;
            if (ht=texheight(ai)) voutln2("(CHARHT R %.1f)", 0.8 * ht) ;
            if (ai->lly) voutln2("(CHARDP R %.1f)", -0.8 * ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %.1f)", 0.8 * (ai->urx - ai->width)) ;
         } else {
            voutln2("(CHARWD R %d)", ai->width) ;
            if (ht=texheight(ai)) voutln2("(CHARHT R %d)", ht) ;
            if (ai->lly) voutln2("(CHARDP R %d)", -ai->lly) ;
            if (ai->urx > ai->width)
               voutln2("(CHARIC R %d)", ai->urx - ai->width) ;
         }
         if (ai->adobenum != i || uppercase[i]) {
            vleft() ; voutln("MAP") ;
            if (uppercase[i]) voutln("(SELECTFONT D 1)") ;
            if (ai->pccs) {
               xoff = 0 ; yoff = 0 ;
               for (npcc = ai->pccs; npcc; npcc=npcc->next)
                  if (api=findadobe(npcc->partname))
                     if (api->texnum>=0) {
                        if (npcc->xoffset != xoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVERIGHT R %.1f)",
                                      0.8 * (npcc->xoffset - xoff)) ;
                           } else voutln2("(MOVERIGHT R %d)",
                                      npcc->xoffset - xoff) ;
                           xoff = npcc->xoffset ;
                        }
                        if (npcc->yoffset != yoff) {
                           if (uppercase[i]) {
                              voutln2("(MOVEUP R %.1f)",
                                      0.8 * (npcc->yoffset - yoff)) ;
                           } else voutln2("(MOVEUP R %d)",
                                      npcc->yoffset - yoff) ;
                           yoff = npcc->yoffset ;
                        }
                        voutln2("(SETCHAR O %o)", api->adobenum) ;
                        xoff += texptrs[api->texnum]->width ;
                     }
            } else voutln2("(SETCHAR O %o)", ai->adobenum) ;
            vright() ;
         }
         vright() ;
      }
   if (level) error("! I forgot to match the parentheses") ;
}

void
openfiles(argc, argv)
int argc ;
char *argv[] ;
{
   register int lastext ;
   register int i ;
   extern void exit() ;
   tfmout = (FILE *)NULL ;

   if (argc == 1) {
      (void)printf("afm2tfm 5.36, Copyright 1990 by Radical Eye Software\n") ;
      (void)
        printf("Usage: afm2tfm foo[.afm] [-v|-V bar[.vpl]] [foo[.tfm]]\n") ;
     exit(0) ;
   }

   (void)sprintf(titlebuf, "%s %s", argv[0], argv[1]) ;
   (void)strcpy(inname, argv[1]) ;
   lastext = -1 ;
   for (i=0; inname[i]; i++)
      if (inname[i] == '.')
         lastext = i ;
      else if (inname[i] == '/' || inname[i] == ':')
         lastext = -1 ;
   if (lastext == -1) (void)strcat(inname, ".afm") ;
   if ((afmin=fopen(inname, "r"))==NULL)
      error("! can't open afm input file") ;

   while (argc>3 && *argv[2]=='-') {
      switch (argv[2][1]) {
case 'V': makevpl++ ;
case 'v': makevpl++ ;
         (void)strcpy(outname, argv[3]) ;
         lastext = -1 ;
         for (i=0; outname[i]; i++)
            if (outname[i] == '.')
               lastext = i ;
            else if (outname[i] == '/' || outname[i] == ':')
               lastext = -1 ;
         if (lastext == -1) (void)strcat(outname, ".vpl") ;
         if ((vplout=fopen(outname, WRITEBIN))==NULL)
            error("! can't open vpl output file") ;
         break ;
case 'e': if (sscanf(argv[3], "%f", &efactor)==0 || efactor<0.01)
            error("! Bad extension factor") ;
         break ;
case 's': if (sscanf(argv[3], "%f", &slant)==0)
            error("! Bad slant parameter") ;
         break ;
case 't':  /* get tfm name if present */
         (void)strcpy(outname, argv[3]) ;
         lastext = -1 ;
         for (i=0; outname[i]; i++)
            if (outname[i] == '.')
               lastext = i ;
            else if (outname[i] == '/' || outname[i] == ':')
                    lastext = -1 ;
        if (lastext == -1) {
           lastext = strlen(outname) ;
           (void)strcat(outname, ".tfm") ;
        }
        if ((tfmout=fopen(outname, WRITEBIN))==NULL)
           error("! can't open tfm output file") ;
        break ;
default: (void)fprintf(stderr, "Unknown option %s %s will be ignored.\n",
                         argv[2], argv[3]) ;
      }
      (void)sprintf(titlebuf, "%s %s %s", titlebuf, argv[2], argv[3]) ;
      argv += 2 ;
      argc -= 2 ;
   }

   if (argc>3 || (argc==3 && *argv[2]=='-'))
      error("! Usage: afm2tfm foo[.afm] [-v|-V bar[.vpl]] [foo[.tfm]]") ;
         
   if (argc == 2) (void)strcpy(outname, inname) ;
   else (void)strcpy(outname, argv[2]) ;

   lastext = -1 ;
   for (i=0; outname[i]; i++)
      if (outname[i] == '.')
         lastext = i ;
      else if (outname[i] == '/' || outname[i] == ':' || outname[i] == '\\')
         lastext = -1 ;
   if (argc == 2) {
      outname[lastext] = 0 ;
      lastext = -1 ;
   }
   if (lastext == -1) {
      lastext = strlen(outname) ;
      (void)strcat(outname, ".tfm") ;
   }
   if (tfmout == NULL && (tfmout=fopen(outname, WRITEBIN))==NULL)
      error("! can't open tfm output file") ;
   outname[lastext] = 0 ;
/*
 *   Now we strip off any directory information, so we only use the
 *   base name in the vf file.  We accept any of /, :, or \ as directory
 *   delimiters, so none of these are available for use inside the
 *   base name; this shouldn't be a problem.
 */
   for (i=0, lastext=0; outname[i]; i++)
      if (outname[i] == '/' || outname[i] == ':' || outname[i] == '\\')
         lastext = i + 1 ;
   if (lastext)
      strcpy(outname, outname + lastext) ;
}

#ifndef VMS
void
#endif
main(argc, argv)
int argc ;
char *argv[] ;
{
   extern void exit() ;

   openfiles(argc, argv) ;
   readadobe() ;
   buildtfm() ;
   writetfm() ;
   if (makevpl) {
      assignchars() ;
      if (makevpl>1) upmap() ;
      writevpl() ;
   }
   exit(0) ;
   /*NOTREACHED*/
}
