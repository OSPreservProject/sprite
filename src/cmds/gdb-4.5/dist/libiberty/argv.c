/* Create and destroy argument vectors (argv's)
   Copyright (C) 1992 Free Software Foundation, Inc.
   Written by Fred Fish @ Cygnus Support

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


/*  Create and destroy argument vectors.  An argument vector is simply an
    array of string pointers, terminated by a NULL pointer. */

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not __GNUC__ */
#ifdef sparc
#include <alloca.h>
extern char *__builtin_alloca();  /* Stupid include file doesn't declare it */
#else
#ifdef _AIX
 #pragma alloca
#else
char *alloca ();
#endif
#endif /* sparc */
#endif /* not __GNUC__ */

#define isspace(ch) ((ch) == ' ' || (ch) == '\t')

#include "alloca-conf.h"

/*  Routines imported from standard C runtime libraries. */

#ifdef __STDC__

#include <stddef.h>
extern void *memcpy (void *s1, const void *s2, size_t n);	/* 4.11.2.1 */
extern size_t strlen (const char *s);				/* 4.11.6.3 */
extern void *malloc (size_t size);				/* 4.10.3.3 */
extern void free (void *ptr);					/* 4.10.3.2 */
extern char *strdup (const char *s);				/* Non-ANSI */

#else	/* !__STDC__ */

extern char *memcpy ();		/* Copy memory region */
extern int strlen ();		/* Count length of string */
extern char *malloc ();		/* Standard memory allocater */
extern void free ();		/* Free malloc'd memory */
extern char *strdup ();		/* Duplicate a string */

#endif	/* __STDC__ */

#ifndef NULL
#define NULL 0
#endif

#ifndef EOS
#define EOS '\000'
#endif

/*  Local data. */

static int argc = 0;
static int maxargc = 0;
static char **argv = NULL;

/*

NAME

	expandargv -- initialize or expand an argv

SYNOPSIS

	static int expandargv ()

DESCRIPTION

	Expand argv by a factor of two (or initialize argv for the first
	time if there is no current argv).

RETURNS

	Returns the new max value for argc, or zero if any error.
*/

static int expandargv ()
{
  register int argvsize;
  register char **newargv;

  /* Increase maxargc */
  if (maxargc == 0)
    {
      maxargc = 16;
    }
  else
    {
      maxargc *= 2;
    }
  argvsize = (maxargc + 1) * sizeof (char *);
  if ((newargv = (char **) malloc (argvsize)) == NULL)
    {
      maxargc = 0;
    }
  else
    {
      if (argv != NULL)
	{
	  /* Copy and free old args */
	  memcpy (newargv, argv, argc * sizeof (char *));
	  free (argv);
	}
      argv = newargv;
    }
  return (maxargc);
}


/*

NAME

	freeargv -- free an argument vector

SYNOPSIS

	void freeargv (vector)
	char **vector;

DESCRIPTION

	Free an argument vector that was built using buildargv.  Simply scans
	through the vector, freeing the memory for each argument, and then
	finally the vector itself.

RETURNS

	No value.

*/

void freeargv (vector)
char **vector;
{
  register char **scan;

  if (vector != NULL)
    {
      for (scan = vector; *scan != NULL; scan++)
	{
	  free (*scan);
	}
      free (vector);
    }
}

/*

NAME

	buildargv -- build an argument vector from a string

SYNOPSIS

	char **buildargv (sp)
	char *sp;

DESCRIPTION

	Given a pointer to a string, parse the string extracting fields
	separated by whitespace and optionally enclosed within either single
	or double quotes (which are stripped off), and build a vector of
	pointers to copies of the string for each field.  The input string
	remains unchanged.

	All of the memory for the pointer array and copies of the string
	is obtained from malloc.  All of the memory can be returned to the
	system with the single function call freeargv, which takes the
	returned result of buildargv, as it's argument.

	The memory for the argv array is dynamically expanded as necessary.

RETURNS

	Returns a pointer to the argument vector if successful. Returns NULL
	if the input string pointer is NULL or if there is insufficient
	memory to complete building the argument vector.

NOTES

	In order to provide a working buffer for extracting arguments into,
	with appropriate stripping of quotes and translation of backslash
	sequences, we allocate a working buffer at least as long as the input
	string.  This ensures that we always have enough space in which to
	work, since the extracted arg is never larger than the input string.

*/

char **buildargv (input)
char *input;
{
  register char *arg;
  register int squote = 0;
  register int dquote = 0;
  register int bsquote = 0;
  char *copybuf;

  argv = NULL;
  if (input != NULL)
    {
      argc = 0;
      maxargc = 0;
      copybuf = alloca (strlen (input) + 1);
      while (*input != EOS)
	{
	  /* Pick off argv[argc] */
	  while (isspace (*input))
	    {
	      input++;
	    }
	  /* Check to see if argv needs expansion. */
	  if (argc >= maxargc)
	    {
	      if (expandargv () == 0)
		{
		  freeargv (argv);
		  argv = NULL;
		  break;
		}
	    }
	  /* Begin scanning arg */
	  arg = copybuf;
	  while (*input != EOS)
	    {
	      if (isspace (*input) && !squote && !dquote && !bsquote)
		{
		  break;
		}
	      else
		{
		  if (bsquote)
		    {
		      bsquote = 0;
		      *arg++ = *input;
		    }
		  else if (*input == '\\')
		    {
		      bsquote = 1;
		    }
		  else if (squote)
		    {
		      if (*input == '\'')
			{
			  squote = 0;
			}
		      else
			{
			  *arg++ = *input;
			}
		    }
		  else if (dquote)
		    {
		      if (*input == '"')
			{
			  dquote = 0;
			}
		      else
			{
			  *arg++ = *input;
			}
		    }
		  else
		    {
		      if (*input == '\'')
			{
			  squote = 1;
			}
		      else if (*input == '"')
			{
			  dquote = 1;
			}
		      else
			{
			  *arg++ = *input;
			}
		    }
		  input++;
		}
	    }
	  *arg = EOS;
	  /* Add arg to argv */
	  if ((argv[argc++] = strdup (copybuf)) == NULL)
	    {
	      freeargv (argv);
	      argv = NULL;
	      break;
	    }
	}
      if (argv != NULL)
	{
	  argv[argc] = NULL;
	}
    }
  return (argv);
}

#ifdef MAIN

/* Simple little test driver. */

static char *tests[] =
{
  "a simple command line",
  "arg 'foo' is single quoted",
  "arg \"bar\" is double quoted",
  "arg \"foo bar\" has embedded whitespace",
  "arg 'Jack said \\'hi\\'' has single quotes",
  "arg 'Jack said \\\"hi\\\"' has double quotes",
  NULL
};

main ()
{
  char **argv;
  char **test;
  char **targs;

  for (test = tests; *test != NULL; test++)
    {
      printf ("buildargv(\"%s\")\n", *test);
      if ((argv = buildargv (*test)) == NULL)
	{
	  printf ("failed!\n\n");
	}
      else
	{
	  for (targs = argv; *targs != NULL; targs++)
	    {
	      printf ("\t\"%s\"\n", *targs);
	    }
	  printf ("\n");
	}
      freeargv (argv);
    }

}

#endif	/* MAIN */
