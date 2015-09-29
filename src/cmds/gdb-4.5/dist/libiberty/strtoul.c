/*
 * strtol : convert a string to long.
 *
 * Andy Wilson, 2-Oct-89.
 */

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include "ansidecl.h"

#ifndef ULONG_MAX
#define	ULONG_MAX	((unsigned long)(~0L))		/* 0xFFFFFFFF */
#endif

extern int errno;

unsigned long
strtoul(s, ptr, base)
     CONST char *s; char **ptr; int base;
{
  unsigned long total = 0, tmp = 0;
  unsigned digit;
  int radix;
  CONST char *start=s;
  int did_conversion=0;

  if (s==NULL)
    {
      errno = ERANGE;
      if (!ptr)
	*ptr = (char *)start;
      return 0L;
    }

  while (isspace(*s))
    s++;
  if (*s == '+')
    s++;
  radix = base;
  if (base==0 || base==16)
    {
      /*
       * try to infer radix from the string
       * (assume decimal).
       */
      if (*s=='0')
	{
	  radix = 8;	/* guess it's octal */
	  s++;		/* (but check for hex) */
	  if (*s=='X' || *s=='x')
	    {
	      s++;
	      radix = 16;
	    }
	}
    }
  if (radix==0)
    radix = 10;

  while ( digit = *s )
    {
      if (digit >= '0' && digit < ('0'+radix))
	digit -= '0';
      else
	if (radix > 10)
	  {
	    if (digit >= 'a' && digit < ('a'+radix))
	      digit = digit - 'a' + 10;
	    else if (digit >= 'A' && digit < ('A'+radix))
	      digit = digit - 'A' + 10;
	    else
	      break;
	  }
	else
	  break;
      did_conversion = 1;
      tmp = (total * radix) + digit;
      if (tmp < total)	/* check overflow */
	{
	  errno = ERANGE;
	  if (ptr != NULL)
	    *ptr = (char *)s;
	  return (ULONG_MAX);
	}
      total = tmp;
      s++;
    }
  if (ptr != NULL)
    *ptr = (char *) ((did_conversion) ? (char *)s : (char *)start);
  return (total);
}
