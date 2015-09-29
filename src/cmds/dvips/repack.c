/*
 *   Compressed TeX fonts in PostScript.  Copyright (C) 1989
 *   by Radical Eye Software.  All Rights Reserved.
 *   (Slight mods by Don Knuth in December 89.)
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#ifdef DEBUG
extern integer debug_flag;
#endif /* DEBUG */

/*   Given a raster that has been unpacked from PK format,
 *   we compress it by another scheme that is suitable for
 *   a PostScript-oriented unpacking. We write instructions for
 *   a little interpreter whose one-byte instructions have the form
 *   18*opcode+parameter. The interpreter forms each row based
 *   on simple transformations to the previous row. */

#define MAXOUT (18)
#define CMD(n) (MAXOUT*(n))
#define ADVXCHG1 CMD(0)
#define ADVXCHG1END CMD(1)
#define CHGX CMD(2)-1
#define CHGXEND CMD(3)-1
#define ADVXLSH CMD(4)
#define ADVXLSHEND CMD(5)
#define ADVXRSH CMD(6)
#define ADVXRSHEND CMD(7)
#define ADVX CMD(8)-1
#define REPX CMD(9)-1
#define SETX CMD(10)-1
#define CLRX CMD(11)-1
#define ADVXCHG2 CMD(12)
#define ADVXCHG2END CMD(13)
#define END CMD(14)

extern char *malloc() ;
extern free() ;
extern void error() ;
extern long getlong() ;
extern long unpack() ;

static int rowlength = 0 ;
static unsigned char *specdata ;
static long tslen = 0 ;
static unsigned char *tempstore, *tsp, *tsend ;

/* First, a routine that appends one byte to the compressed output. */

#define addtse(n) {lcm=tsp-tempstore;addts(n);} /* mark END option position */

static long addts(what)
register unsigned char what ;
{
   register unsigned char *p, *q ;

   if (tsp >= tsend) {
      if (tempstore == NULL) {
         tslen = 4096 ;
         tempstore = (unsigned char *)malloc((unsigned)tslen) ;
         if (tempstore == NULL)
            error("! out of memory") ;
         tsp = tempstore ;
      } else {
         tslen = 2 * tslen ;
         tsp = (unsigned char *)malloc((unsigned)tslen) ;
         if (tsp == NULL)
            error("! out of memory") ;
         for (p=tempstore, q=tsp; p<tsend; p++, q++)
            *q = *p ;
         free((char *)tempstore) ;
         tempstore = tsp ;
         tsp = q ;
      }
      tsend = tempstore + tslen ;
   }
   *tsp++ = what ;
}

/* Next, a routine that discovers how to do the compression. */

#define rsh(a,b) ( ((a)==0) ? ((b)==128) : ( ((a)==255) ? ((b)==127) :\
                                    ((b)==(((a)>>1)|((a)&128))) ))
#define lsh(a,b) ( ((a)==0) ? ((b)==1) : ( ((a)==255) ? ((b)==254) :\
                                    ((b)==((((a)<<1)&255)|((a)&1))) ))
#define DIFFERENT (1)
#define LSHPOSSIB (2)
#define RSHPOSSIB (4)
#define BLKPOSSIB (8)
#define WHTPOSSIB (16)
#define ENDROW (32)
#define NOPOSSIB(n) ((n&(LSHPOSSIB|RSHPOSSIB|BLKPOSSIB|WHTPOSSIB))==0)
#define NOSHIFT(n) ((n&(LSHPOSSIB|RSHPOSSIB))==0)
/*
 *   Our input bytes are packed to the 16-bit word.  On output,
 *   they're packed to bytes. Thus, we may have to skip a byte at
 *   the end of each row.
 */
void dochar(from, width, height)
unsigned char *from ;
short width, height ; /* in bytes */
{
   register int i ;
   register unsigned char *f, *t, *d ;
   register unsigned char *e ;
   register int accum ;
   int j, k ;
   int diffrow ;
   int repeatcount ;
   int lit, pos, cmd ;
   long lcm ;
   int widthc ;

   widthc = width + (width & 1) ; /* halfword correction */
   lcm = -1 ;
   if (width > rowlength) {
      if (rowlength)
         free((char *)specdata) ;
      rowlength = 2 * widthc ;
      specdata = (unsigned char *)malloc((unsigned)(rowlength + 1)) ;
      if (specdata == NULL)
         error("! out of memory") ;
   }
   for (i=0, t=specdata; i<widthc; i++, t++)
      *t = 0 ;
   f = specdata ;
   repeatcount = 0 ;
   for (j=0; j<height; j++, f = from, from += widthc) {
      diffrow = 0 ;
      for (i=0, t=from, d=specdata; i<width; i++, f++, t++, d++) {
         if (*f == *t) {
            if (*t == 0)
               *d = WHTPOSSIB ;
            else if (*t == 255)
               *d = BLKPOSSIB ;
            else
               *d = 0 ;
         } else {
            accum = DIFFERENT ;
            if (rsh(*f, *t))
               accum |= RSHPOSSIB ;
            else if (lsh(*f, *t))
               accum |= LSHPOSSIB ;
            if (*t == 0)
               accum |= WHTPOSSIB ;
            else if (*t == 255)
               accum |= BLKPOSSIB ;
            *d = accum ;
            diffrow++ ;
         }
      } /* end 'for i' */
      *d = ENDROW ;
      if (diffrow == 0) {
         repeatcount++ ;
      } else {
         if (repeatcount) {
            while (repeatcount > MAXOUT) {
               addts((unsigned char)(REPX+MAXOUT)) ;
               repeatcount -= MAXOUT ;
            }
            addts((unsigned char)(REPX+repeatcount)) ;
            repeatcount = 0 ;
         }
         pos = 0 ;
         for (i=0, d=specdata, f=t-width; i<width;) {
            if ((*d & DIFFERENT) == 0) {
               i++ ;
               d++ ;
               f++ ;
            } else {
               accum = 0 ;
               if (pos != i)
                  lit = NOSHIFT(*d) ;
               else /* N.B.: 'lit' does not imply literate programming here */
                  lit = NOPOSSIB(*d) ;
               for (e=d; ;e++) {
                  if (NOPOSSIB(*e))
                     lit = 1 ;
                  if ((*e & DIFFERENT) == 0)
                     break ;
                  if ((*e & WHTPOSSIB) &&
                      (e[1] & WHTPOSSIB)) {
                     while (*e & WHTPOSSIB) {
                        e++ ;
                        accum++ ;
                     }
                     cmd = CLRX ;
                     e -= accum ;
                     break ;
                  } else if ((*e & BLKPOSSIB) &&
                      (e[1] & BLKPOSSIB)) {
                     while (*e & BLKPOSSIB) {
                        e++ ;
                        accum++ ;
                     }
                     cmd = SETX ;
                     e -= accum ;
                     break ;
                  }
               } /* end 'for e'; d pts to first bad byte, e to next good one */
               while (i - pos > MAXOUT) {
                  addts((unsigned char)(ADVX+MAXOUT)) ;
                  pos += MAXOUT ;
               }
               if (k = (e - d)) {
                  if (lit) {
                     if (k > 2) {
                        if (i > pos) {
                           addts((unsigned char)(ADVX + i - pos)) ;
                           pos = i ;
                        }
                        while (k > MAXOUT) {
                           addts((unsigned char)(CHGX + MAXOUT)) ;
                           for (k=0; k<MAXOUT; k++)
                              addts((unsigned char)(*f++)) ;
                           d += MAXOUT ;
                           pos += MAXOUT ;
                           i += MAXOUT ;
                           k -= MAXOUT ;
                        }
                        addtse((unsigned char)(CHGX + k)) ;
                        pos += k ;
                        for (; d<e; d++, i++, f++)
                           addts((unsigned char)(*f)) ;
                     } else {
                        if (k == 1) {
                           if (i == pos+MAXOUT) {
                              addts((unsigned char)(ADVX + MAXOUT)) ;
                              i = pos ;
                           }
                           addtse((unsigned char)(ADVXCHG1 + i - pos)) ;
                           addts((unsigned char)(*f)) ;
                           i++ ;
                           pos = i ;
                           d++ ;
                           f++ ;
                        } else {
                           if (i == pos+MAXOUT) {
                              addts((unsigned char)(ADVX + MAXOUT)) ;
                              i = pos ;
                           }
                           addtse((unsigned char)(ADVXCHG2 + i - pos)) ;
                           addts((unsigned char)(*f)) ;
                           addts((unsigned char)(f[1])) ;
                           i += 2 ;
                           pos = i ;
                           d += 2 ;
                           f += 2 ;
                        }
                     }
                  } else {
                     while (e > d) {
                        if (*d & LSHPOSSIB) {
                           if (i == pos+MAXOUT) {
                              addts((unsigned char)(ADVX + MAXOUT)) ;
                              i = pos ;
                           }
                           addtse((unsigned char)(ADVXLSH + i - pos)) ;
                        } else if (*d & RSHPOSSIB) {
                           if (i == pos+MAXOUT) {
                              addts((unsigned char)(ADVX + MAXOUT)) ;
                              i = pos ;
                           }
                           addtse((unsigned char)(ADVXRSH + i - pos)) ;
                        } else if (*d & WHTPOSSIB) { /* i==pos */
                           addts((unsigned char)(CLRX + 1)) ; lcm = -1 ;
                        } else if (*d & BLKPOSSIB) { /* i==pos */
                           addts((unsigned char)(SETX + 1)) ; lcm = -1 ;
                        } else
                           error("! bug") ; /* why wasn't lit true? */
                        d++ ;
                        f++ ;
                        i++ ;
                        pos = i ;
                     } /* end 'while e>d' */
                  }
               } /* end 'if e>d' */
               if (accum > 0) {
                  if (i > pos) {
                     addts((unsigned char)(ADVX + i - pos)) ;
                     pos = i ;
                  }
                  i += accum ;
                  d += accum ;
                  f += accum ;
                  while (accum > MAXOUT) {
                     addts((unsigned char)(cmd + MAXOUT)) ;
                     accum -= MAXOUT ;
                     pos += MAXOUT ;
                  }
                  lcm = -1 ;
                  addts((unsigned char)(cmd + accum)) ;
                  pos += accum ;
               }
            } /* end 'else DIFFERENT' */
         } /* end 'for i' */
         if (lcm != -1) {
            tempstore[lcm] += MAXOUT ;  /* append END */
            lcm = -1 ;
         } else {
            addts((unsigned char)(END)) ;
         }
      } /* end 'else rows different' */
   } /* end 'for j' */
   if (repeatcount) {
      while (repeatcount > MAXOUT) {
         addts((unsigned char)(REPX+MAXOUT)) ;
         repeatcount -= MAXOUT ;
      }
      addts((unsigned char)(REPX+repeatcount)) ;
      repeatcount = 0 ;
   }
}

extern long bytesleft ;
extern quarterword *raster ;
static long mbytesleft ;
static quarterword *mraster ;

char *
makecopy(what, len, p)
register unsigned char *what ;
register long len ;
register unsigned char *p ;
{
   register unsigned char *q ;

   if (p == NULL) {
      if (len > 32760)
         error("! oops, raster too big") ;
      if (bytesleft < len) {
         if (RASTERCHUNK > len) {
            raster = (quarterword *)malloc((unsigned)RASTERCHUNK) ;
            bytesleft = RASTERCHUNK ;
         } else {
            raster = (quarterword *)malloc((unsigned)(len + 3)) ;
            bytesleft = len ;
         }
         if (raster == NULL) {
            error("! out of memory during allocation") ;
         }
      }
      p = (unsigned char *)raster ;
      bytesleft -= len ;
      raster += len ;
   }
   q = p ;
   while (len > 0) {
      *p++ = *what++ ;
      len -- ;
   }
   return ((char *)q) ;
}

/* Now the main routine, which is called when a character is used
   for the very first time. */

repack(cp)
register chardesctype *cp ;
{
   register long i, j ;
   register unsigned char *p ;
   register int width, height ;
   register int wwidth ;
   char startbytes ;

   p = cp->packptr ;
   if (p == NULL)
      error("! no raster?") ;
   tsp = tempstore ;
   tsend = tempstore + tslen ;
   addts(*p) ; /* copy the PK flag byte */
   if (*p & 4) {
      if ((*p & 7) == 7) {
         startbytes = 17 ;
         width = getlong(p + 1) ;
         height = getlong(p + 5) ;
         for (i=0; i<12; i++)
            addts(*++p) ;
      } else {
         startbytes = 9 ;
         width = p[1] * 256 + p[2] ;
         height = p[3] * 256 + p[4] ;
         addts(*++p) ;
         addts(*++p) ;
         addts(*++p) ;
         addts(*++p) ;
      }
   } else {
      startbytes = 5 ;
      width = p[1] ;
      height = p[2] ;
   }
   addts(*++p) ;
   addts(*++p) ;
   addts(*++p) ;
   addts(*++p) ;
   p++ ; /* OK, p now points to beginning of the nibbles */
   addts((unsigned char)0) ;
   addts((unsigned char)0) ;
   addts((unsigned char)0) ;
   addts((unsigned char)0) ; /* leave room to stick in a new length field */
   wwidth = (width + 15) / 16 ;
   i = 2 * height * (long)wwidth ;
   if (i <= 0)
      i = 2 ;
   if (mbytesleft < i) {
      if (mbytesleft >= RASTERCHUNK)
         (void) free((char *) mraster) ;
      if (RASTERCHUNK > i) {
         mraster = (quarterword *)malloc((unsigned)RASTERCHUNK) ;
         mbytesleft = RASTERCHUNK ;
      } else {
         mraster = (quarterword *)malloc((unsigned)(i + 3)) ;
         mbytesleft = i ;
      }
      if (mraster == NULL) {
         error("! out of memory during allocation") ;
      }
   }
   while (i > 0)
      mraster[--i] = 0 ;
   i = p - cp->packptr ;
   i += unpack(p, mraster, width, height, *(cp->packptr)) ;
   dochar(mraster, (width + 7) >> 3, height) ;
   j = tsp - tempstore ;
#ifdef DEBUG
   if (dd(D_COMPRESS))
#ifdef SHORTINT
        (void)fprintf(stderr,"PK %ld bytes, unpacked %ld, compressed %ld\n",
#else /* ~SHORTINT */
        (void)fprintf(stderr,"PK %d bytes, unpacked %d, compressed %d\n",
#endif /* ~SHORTINT */
            i-startbytes, ((width+7)/8)*height, j-startbytes-4) ;
#endif /* DEBUG */
   if ( i < j )
      cp->packptr = (unsigned char *)makecopy(tempstore, j, NULL) ;
   else
      makecopy(tempstore, j, cp->packptr) ;
   putlong(cp->packptr + startbytes, j - startbytes - 4) ;
   cp->flags |= REPACKED ;
}

putlong(a, i)
register char *a ;
long i ;
{
   register char *b = (char *)&i ;

   a[0] = b[0] ;
   a[1] = b[1] ;
   a[2] = b[2] ;
   a[3] = b[3] ;
}

long getlong(a)
register char *a ;
{
   long t ;
   register char *b ;

   b = (char *)&t ;
   b[0] = a[0] ;
   b[1] = a[1] ;
   b[2] = a[2] ;
   b[3] = a[3] ;
   return(t) ;
}
