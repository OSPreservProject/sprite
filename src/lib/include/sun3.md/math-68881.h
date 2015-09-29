
/*******************************************************************
*                                                                  *
*  <math-68881.h>               last modified: March 22, 1989.     *
*                                                                  *
*  Copyright (C) 1989 by Matthew Self.                             *
*  You may freely distribute verbatim copies of this software      *
*  provided that this copyright notice is retained in all copies.  *
*  You may distribute modifications to this software under the     *
*  conditions above if you also clearly note such modifications    *
*  with their author and date.                                     *
*                                                                  *
*  Note:  errno is not set to EDOM when domain errors occur for    *
*  most of these functions.  Rather, it is assumed that the        *
*  68881's OPERR exception will be enabled and handled             *
*  appropriately by the operating system.  Similarly, overflow     *
*  and underflow do not set errno to ERANGE.                       *
*                                                                  *
*  Send bugs to Matthew Self (self@bayes.arc.nasa.gov).            *
*                                                                  *
*******************************************************************/

#ifndef __asm
#define __asm asm               /* until new __asm is done */
#endif

#ifndef __inline
#define __inline inline         /* until new __inline is done */
#endif

#include <errno.h>

#ifndef HUGE_VAL
#define HUGE_VAL                                                        \
({                                                                      \
  double huge_val;                                                      \
                                                                        \
  __asm ("fmove%.d #0x7ff0000000000000,%0"      /* Infinity */          \
         : "=f" (huge_val)                                              \
         : /* no inputs */);                                            \
  huge_val;                                                             \
})
#endif

__inline static const double sin (double x)
{
  double value;

  __asm ("fsin%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double cos (double x)
{ 
  double value;

  __asm ("fcos%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double tan (double x)
{
  double value;

  __asm ("ftan%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double asin (double x)
{
  double value;

  __asm ("fasin%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double acos (double x)
{
  double value;

  __asm ("facos%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double atan (double x)
{
  double value;

  __asm ("fatan%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double atan2 (double y, double x)
{
  double pi, pi_over_2;

  __asm ("fmovecr%.x %#0,%0"            /* extended precision pi */
         : "=f" (pi)
         : /* no inputs */ );
  __asm ("fscale%.b %#-1,%0"            /* no loss of accuracy */
         : "=f" (pi_over_2)
         : "0" (pi));
  if (x > 0)
    {
      if (y > 0)
        {
          if (x > y)
            return atan (y / x);
          else
            return pi_over_2 - atan (x / y);
        }
      else
        {
          if (x > -y)
            return atan (y / x);
          else
            return - pi_over_2 - atan (x / y);
        }
    }
  else
    {
      if (y > 0)
        {
          if (-x > y)
            return pi + atan (y / x);
          else
            return pi_over_2 - atan (x / y);
        }
      else
        {
          if (-x > -y)
            return - pi + atan (y / x);
          else if (y < 0)
            return - pi_over_2 - atan (x / y);
          else
            {
              double value;

              errno = EDOM;
              __asm ("fmove%.d %#0rnan,%0"      /* quiet NaN */
                     : "=f" (value)
                     : /* no inputs */);
              return value;
            }
        }
    }
}

__inline static const double sinh (double x)
{
  double value;

  __asm ("fsinh%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double cosh (double x)
{
  double value;

  __asm ("fcosh%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double tanh (double x)
{
  double value;

  __asm ("ftanh%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double exp (double x)
{
  double value;

  __asm ("fetox%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double log (double x)
{
  double value;

  __asm ("flogn%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double log10 (double x)
{
  double value;

  __asm ("flog10%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double pow (const double x, const double y)
{
  if (x > 0)
    return exp (y * log (x));
  else if (x == 0)
    {
      if (y > 0)
        return 0.0;
      else
        {
          double value;

          errno = EDOM;
          __asm ("fmove%.d %#0rnan,%0"          /* quiet NaN */
                 : "=f" (value)
                 : /* no inputs */);
          return value;
        }
    }
  else
    {
      double temp;

      __asm ("fintrz%.x %1,%0"
             : "=f" (temp)                      /* integer-valued float */
             : "f" (y));
      if (y == temp)
        {
          int i = (int) y;

          if (i & 1 == 0)                       /* even */
            return exp (y * log (x));
          else
            return - exp (y * log (x));
        }
      else
        {
          double value;

          errno = EDOM;
          __asm ("fmove%.d %#0rnan,%0"          /* quiet NaN */
                 : "=f" (value)
                 : /* no inputs */);
          return value;
        }
    }
}

__inline static const double sqrt (double x)
{
  double value;

  __asm ("fsqrt%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}

__inline static const double ceil (double x)
{
  int rounding_mode, round_up;
  double value;

  __asm volatile ("fmove%.l fpcr,%0"
                  : "=dm" (rounding_mode)
                  : /* no inputs */ );
  round_up = rounding_mode | 0x30;
  __asm volatile ("fmove%.l %0,fpcr"
                  : /* no outputs */
                  : "dmi" (round_up));
  __asm volatile ("fint%.x %1,%0"
                  : "=f" (value)
                  : "f" (x));
  __asm volatile ("fmove%.l %0,fpcr"
                  : /* no outputs */
                  : "dmi" (rounding_mode));
  return value;
}

__inline static const double floor (double x)
{
  int rounding_mode, round_down;
  double value;

  __asm volatile ("fmove%.l fpcr,%0"
                  : "=dm" (rounding_mode)
                  : /* no inputs */ );
  round_down = (rounding_mode & ~0x10)
                | 0x20;
  __asm volatile ("fmove%.l %0,fpcr"
                  : /* no outputs */
                  : "dmi" (round_down));
  __asm volatile ("fint%.x %1,%0"
                  : "=f" (value)
                  : "f" (x));
  __asm volatile ("fmove%.l %0,fpcr"
                  : /* no outputs */
                  : "dmi" (rounding_mode));
  return value;
}

#ifndef fabs
__inline static const double fabs (double x)
{
  double value;

  __asm ("fabs%.x %1,%0"
         : "=f" (value)
         : "f" (x));
  return value;
}
#endif

__inline static const double fmod (double x, double y)
{
  double value;

  __asm ("fmod%.x %2,%0"
         : "=f" (value)
         : "0" (x),
           "f" (y));
  return value;
}

__inline static const double ldexp (double x, int n)
{
  double value;

  __asm ("fscale%.l %2,%0"
         : "=f" (value)
         : "0" (x),
           "dmi" (n));
  return value;
}

__inline static double frexp (double x, int *exp)
{
  double float_exponent;
  int int_exponent;
  double mantissa;

  __asm ("fgetexp%.x %1,%0"
         : "=f" (float_exponent)        /* integer-valued float */
         : "f" (x));
  int_exponent = (int) float_exponent;
  __asm ("fgetmant%.x %1,%0"
         : "=f" (mantissa)              /* 1.0 <= mantissa < 2.0 */
         : "f" (x));
  if (mantissa != 0)
    {
      __asm ("fscale%.b %#-1,%0"
             : "=f" (mantissa)          /* mantissa /= 2.0 */
             : "0" (mantissa));
      int_exponent += 1;
    }
  *exp = int_exponent;
  return mantissa;
}

__inline static double modf (double x, double *ip)
				/* ip means "integer part" */
{
  __asm ("fintrz%.x %1,%0"
         : "=f" (*ip)		/* integer-valued float */
         : "f" (x));
  return x - (*ip);
}


