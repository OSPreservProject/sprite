/*
 *   This software is Copyright 1988 by Radical Eye Software.
 *   All Rights Reserved.
 */
#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include "structures.h"
extern int quiet ;
extern int filter ;
extern char *mfmode ;
/*
 *   Calculate magstep values.
 */
static int
magstep(n, bdpi)
register int n, bdpi ;
{
   register float t ;
   int neg = 0 ;

   if (n < 0) {
      neg = 1 ;
      n = -n ;
   }
   if (n & 1) {
      n &= ~1 ;
      t = 1.095445115 ;
   } else
      t = 1.0 ;
   while (n > 8) {
      n -= 8 ;
      t = t * 2.0736 ;
   }
   while (n > 0) {
      n -= 2 ;
      t = t * 1.2 ;
   }
   if (neg)
      return((int)(0.5 + bdpi / t)) ;
   else
      return((int)(0.5 + bdpi * t)) ;
}
char *command = "MakeTeXPK %n %d %b %m" ;
/*
 *   This routine tries to create a font by executing a command, and
 *   then opening the font again if possible.
 */
static char buf[125] ;
void
makefont(name, dpi, bdpi)
   char *name ;
   int dpi, bdpi ;
{
   register char *p, *q ;
   register int m, n ;

   for (p=command, q=buf; *p; p++)
      if (*p != '%')
         *q++ = *p ;
      else {
         switch (*++p) {
case 'n' : case 'N' :
            (void)strcpy(q, name) ;
            break ;
case 'd' : case 'D' :
            (void)sprintf(q, "%d", dpi) ;
            break ;
case 'b' : case 'B' :
            (void)sprintf(q, "%d", bdpi) ;
            break ;
case 'm' : case 'M' :
/*
 *   Here we want to return a string.  If we can find some integer
 *   m such that floor(0.5 + bdpi * 1.2 ^ (m/2)) = dpi, we write out
 *      magstep(m/2)
 *   where m/2 is a decimal number; else we write out
 *      dpi/bdpi
 *   We do this for the very slight improvement in accuracy that
 *   magstep() gives us over the rounded dpi/bdpi.
 */
            m = 0 ;
            if (dpi < bdpi) {
               while (1) {
                  m-- ;
                  n = magstep(m, bdpi) ;
                  if (n == dpi)
                     break ;
                  if (n < dpi || m < -40) {
                     m = 9999 ;
                     break ;
                  }
               }
            } else if (dpi > bdpi) {
               while (1) {
                  m++ ;
                  n = magstep(m, bdpi) ;
                  if (n == dpi)
                     break ;
                  if (n > dpi || m > 40) {
                     m = 9999 ;
                     break ;
                  }
               }
            }
            if (m == 9999) {
               (void)sprintf(q, "%d+%d/%d", dpi/bdpi, dpi%bdpi, bdpi) ;
            } else if (m >= 0) {
               (void)sprintf(q, "magstep\\(%d.%d\\)", m/2, (m&1)*5) ;
            } else {
               (void)sprintf(q, "magstep\\(-%d.%d\\)", (-m)/2, (m&1)*5) ;
            }
            break ;
case 0 :    *q = 0 ;
            break ;
default:    *q++ = *p ;
            *q = 0 ;
            break ;
         }
         q += strlen(q) ;
      }
   *q = 0 ;
   if (mfmode) {
      strcpy(q, " ") ;
      strcat(q, mfmode) ;
   }
   if (filter)
      (void)strcat(buf, " >/dev/null") ;
   if (! quiet)
      (void)fprintf(stderr, "- %s\n", buf) ;
   (void)system(buf) ;
}
