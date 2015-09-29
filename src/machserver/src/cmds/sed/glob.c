/* File-name wildcard pattern matching for GNU.
   Copyright (C) 1985, 1988, 1989 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* To whomever it may concern: I have never seen the code which most
 Unix programs use to perform this function.  I wrote this from scratch
 based on specifications for the pattern matching.  */

#include <sys/types.h>

#ifdef	USGr3
#include <dirent.h>
#define direct dirent
#define	D_NAMLEN(d) strlen((d)->d_name)
#else	/* not USGr3	*/
#define D_NAMLEN(d) ((d)->d_namlen)
#	ifdef	USG
#include "ndir.h"   /* Get ndir.h from the Emacs distribution.  */
#	else	/* not USG	*/
#include <sys/dir.h>
#	endif	/* USG		*/
#endif	/* USGr3	*/

#ifdef USG
#include <memory.h>
#include <string.h>
#define bcopy(s, d, n) ((void) memcpy ((d), (s), (n)))
#define rindex strrchr
struct passwd *getpwent(), *getpwuid(), *getpwnam();

extern char *memcpy ();
#else /* not USG */
#include <strings.h>

extern void bcopy ();
#endif /* not USG */

#ifdef	__GNUC__
#define	alloca(n)	__builtin_alloca (n)
#else	/* Not GCC.  */
#ifdef	sparc
#include <alloca.h>
#else	/* Not sparc.  */
extern char *alloca ();
#endif	/* sparc.  */
#endif	/* GCC.  */

#include <pwd.h>

extern char *malloc (), *realloc ();
extern void free ();

#ifndef NULL
#define NULL 0
#endif

/* Zero if * matches .*.  */
int noglob_dot_filenames = 1;

/* Nonzero if ~ and ~USER are expanded by glob_filename.  */
int glob_tilde = 0;


static int glob_match_after_star ();

/* Return nonzero if PATTERN has any special globbing chars in it.  */
int
glob_pattern_p (pattern)
     char *pattern;
{
  register char *p = pattern;
  register char c;

  while ((c = *p++))
    {
      switch (c)
	{
	case '?':
	case '[':
	case '*':
	  return 1;

	case '\\':
	  if (*p++ == 0) return 0;
	default:
	  ;
	}
    }

  return 0;
}


/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.

   A match means the entire string TEXT is used up in matching.

   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [^SET] matches any character not in the specified set.

   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.

   To suppress the special syntactic significance of any of `[]*?^-\',
   and match the character exactly, precede it with a `\'.

   If DOT_SPECIAL is nonzero,
   `*' and `?' do not match `.' at the beginning of TEXT.  */

int
glob_match (pattern, text, dot_special)
     char *pattern, *text;
     int dot_special;
{
  register char *p = pattern, *t = text;
  register char c;

  while ((c = *p++))
    {
      switch (c)
	{
	case '?':
	  if (*t == 0 || (dot_special && t == text && *t == '.')) return 0;
	  else ++t;
	  break;

	case '\\':
	  if (*p++ != *t++) return 0;
	  break;

	case '*':
	  if (dot_special && t == text && *t == '.')
	    return 0;
	  return glob_match_after_star (p, t);

	case '[':
	  {
	    register char c1 = *t++;
	    register int invert = (*p == '^');

	    if (invert) p++;

	    c = *p++;
	    while (1)
	      {
		register char cstart = c, cend = c;

		if (c == '\\')
		  {
		    cstart = *p++; cend = cstart;
		  }

		if (!c) return (0);

		c = *p++;

		if (c == '-')
		  {
		    cend = *p++;
		    if (cend == '\\')
		      cend = *p++;
		    if (!cend) return (0);
		    c = *p++;
		  }
		if (c1 >= cstart && c1 <= cend) goto match;
		if (c == ']')
		  break;
	      }
	    if (!invert) return 0;
	    break;

	  match:
	    /* Skip the rest of the [...] construct that already matched.  */
	    while (c != ']')
	      { 
	        if (!c || !(c = *p++)) return (0);
		if (c == '\\') p++;
	      }
	    if (invert) return 0;
	    break;
	  }

	default:
	  if (c != *t++) return 0;
	}
    }

  if (*t) return 0;
  return 1;
}

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */

static int
glob_match_after_star (pattern, text)
     char *pattern, *text;
{
  register char *p = pattern, *t = text;
  register char c, c1;

  while ((c = *p++) == '?' || c == '*')
    {
      if (c == '?' && *t++ == 0)
	return 0;
    }

  if (c == 0)
    return 1;

  if (c == '\\') c1 = *p;
  else c1 = c;

  for (;;)
    {
      if ((c == '[' || *t == c1) 
          && glob_match (p - 1, t, 0))
	return 1;
      if (*t++ == 0) return 0;
    }
}

/* Return a vector of names of files in directory DIR
   whose names match glob pattern PAT.
   The names are not in any particular order.
   Wildcards at the beginning of PAT do not match an initial period.

   The vector is terminated by an element that is a null pointer.

   To free the space allocated, first free the vector's elements,
   then free the vector.

   Return 0 if cannot get enough memory to hold the pointer
   and the names.

   Return -1 if cannot access directory DIR.
   Look in errno for more information.  */

char **
glob_vector (pat, dir)
     char *pat;
     char *dir;
{
  struct globval
    {
      struct globval *next;
      char *name;
    };

  DIR *d;
  register struct direct *dp;
  struct globval *lastlink;
  register struct globval *nextlink;
  register char *nextname;
  int count;
  int lose;
  register char **name_vector;
  register int i;

  if (!(d = opendir (dir)))
    return (char **) -1;

  lastlink = 0;
  count = 0;
  lose = 0;

  /* Scan the directory, finding all names that match.
     For each name that matches, allocate a struct globval
     on the stack and store the name in it.
     Chain those structs together; lastlink is the front of the chain.  */
  /* Loop reading blocks */
  while (1)
    {
      dp = readdir (d);
      if (!dp) break;
      if (dp->d_ino && glob_match (pat, dp->d_name, noglob_dot_filenames))
	{
	  nextlink = (struct globval *) alloca (sizeof (struct globval));
	  nextlink->next = lastlink;
	  nextname = (char *) malloc (D_NAMLEN(dp) + 1);
	  if (!nextname)
	    {
	      lose = 1;
	      break;
	    }
	  lastlink = nextlink;
	  nextlink->name = nextname;
	  bcopy (dp->d_name, nextname, D_NAMLEN(dp) + 1);
	  count++;
	}
    }
  closedir (d);

  name_vector = (char **) malloc ((count + 1) * sizeof (char *));

  /* Have we run out of memory?  */
  if (!name_vector || lose)
    {
      /* Here free the strings we have got */
      while (lastlink)
	{
	  free (lastlink->name);
	  lastlink = lastlink->next;
	}
      return 0;
    }

  /* Copy the name pointers from the linked list into the vector */
  for (i = 0; i < count; i++)
    {
      name_vector[i] = lastlink->name;
      lastlink = lastlink->next;
    }

  name_vector[count] = 0;
  return name_vector;
}

/* Return a new array which is the concatenation of each string in
   ARRAY to DIR. */

static char **
glob_dir_to_array (dir, array)
     char *dir, **array;
{
  register int i, l;
  int add_slash = 0;
  char **result;

  l = strlen (dir);
  if (!l) return (array);

  if (dir[l - 1] != '/') add_slash++;

  for (i = 0; array[i]; i++);

  result = (char **)malloc ((1 + i) * sizeof (char *));
  if (!result) return (result);

  for (i = 0; array[i]; i++) {
    result[i] = (char *)malloc (1 + l + add_slash + strlen (array[i]));
    if (!result[i]) return (char **)NULL;
    strcpy (result[i], dir);
    if (add_slash) strcat (result[i], "/");
    strcat (result[i], array[i]);
  }
  result[i] = (char *)NULL;

  /* Free the input array. */
  for (i = 0; array[i]; i++) free (array[i]);
  free (array);
  return (result);
}

/* Do globbing on PATHNAME.  Return an array of pathnames that match,
   marking the end of the array with a null-pointer as an element.
   If no pathnames match, then the array is empty (first element is null).
   If there isn't enough memory, then return NULL.
   If a file system error occurs, return -1; `errno' has the error code.

   Wildcards at the beginning of PAT, or following a slash,
   do not match an initial period.  */

char **
glob_filename (pathname)
     char *pathname;
{
  char **result;
  unsigned int result_size;
  char *directory_name, *filename;
  unsigned int directory_len;

  result = (char **) malloc (sizeof (char *));
  result_size = 1;
  if (result == NULL)
    return NULL;

  result[0] = NULL;

  /* Find the filename.  */
  filename = rindex (pathname, '/');
  if (filename == 0)
    {
      filename = pathname;
      directory_name = "";
      directory_len = 0;
    }
  else
    {
      directory_len = filename - pathname;
      directory_name = (char *) alloca (directory_len + 1);
      bcopy (pathname, directory_name, directory_len);
      directory_name[directory_len] = '\0';
      ++filename;
    }

  if (glob_tilde && *pathname == '~')
    {
      if (directory_len == 0)
	{
	  filename = directory_name;
	  directory_name = pathname;
	  directory_len = strlen (directory_name);
	}

      if (directory_len == 1)
	{
	  extern char *getenv ();
	  static char *home_directory = 0;
	  static unsigned int home_len;

	  if (home_directory == 0)
	    {
	      home_directory = getenv ("HOME");
	      if (home_directory == NULL)
		{
		  home_directory == "";
		  home_len = 0;
		}
	      else
		home_len = strlen (home_directory);
	    }

	  directory_name = home_directory;
	  directory_len = home_len;
	}
      else
	{
	  struct passwd *pwent = getpwnam (directory_name + 1);
	  if (pwent == 0)
	    {
	      directory_name = "";
	      directory_len = 0;
	    }
	  else
	    {
	      directory_name = pwent->pw_dir;
	      directory_len = strlen (directory_name);
	    }
	}
    }
  else if (glob_pattern_p (directory_name))
    {
      /* If directory_name contains globbing characters, then we
	 have to expand the previous levels.  Just recurse. */
      char **directories;
      register unsigned int i;

      if (directory_name[directory_len - 1] == '/')
	directory_name[directory_len - 1] = '\0';

      directories = glob_filename (directory_name);
      if (directories == NULL)
	goto memory_error;
      else if ((int) directories == -1)
	return (char **) -1;
      else if (*directories == NULL)
	{
	  free ((char *) directories);
	  return (char **) -1;
	}

      /* We have successfully globbed the preceding directory name.
	 For each name in DIRECTORIES, call glob_vector on it and
	 FILENAME.  Concatenate the results together.  */
      for (i = 0; directories[i] != NULL; ++i)
	{
	  char **temp_results = glob_vector (filename, directories[i]);
	  if (temp_results == NULL)
	    goto memory_error;
	  else if ((int) temp_results == -1)
	    /* This filename is probably not a directory.  Ignore it.  */
	    ;
	  else
	    {
	      char **array = glob_dir_to_array (directories[i], temp_results);

	      register unsigned int l = 0;
	      while (array[l] != NULL)
		++l;

	      result = (char **) realloc (result,
					  (result_size + 1) * sizeof (char *));
	      if (result == NULL)
		goto memory_error;

	      for (l = 0; array[l] != NULL; ++l)
		result[result_size++ - 1] = array[l];
	      result[result_size - 1] = NULL;
	      free ((char *) array);
	    }
	}
      /* Free the directories.  */
      for (i = 0; directories[i]; i++)
	free (directories[i]);
      free ((char *) directories);

      return result;
    }

  if (*filename == '\0')
    {
      /* If there is only a directory name, return it.  */
      result = (char **) realloc ((char *) result, 2 * sizeof (char *));
      if (result == NULL)
	return NULL;
      result[0] = (char *) malloc (directory_len + 1);
      if (result[0] == NULL)
	goto memory_error;
      bcopy (directory_name, result[0], directory_len + 1);
      result[1] = NULL;
      return result;
    }
  else
    {
      /* Otherwise, just return what glob_vector
	 returns appended to the directory name. */
      char **temp_results = glob_vector (filename,
					 (directory_len == 0
					  ? "." : directory_name));

      if (temp_results == NULL || (int) temp_results == -1)
	return temp_results;

      return glob_dir_to_array (directory_name, temp_results);
    }

  memory_error:;
  if (result != NULL)
    {
      register unsigned int i;
      for (i = 0; result[i] != NULL; ++i)
	free (result[i]);
      free ((char *) result);
    }
  return NULL;
}



#ifdef TEST

main (argc, argv)
     int argc;
     char **argv;
{
  char **value;
  int i, index = 1;

  while (index < argc) {
    value = glob_filename (argv[index]);
    if ((int) value == 0)
      printf ("Memory exhausted.\n");
    else if ((int) value == -1)
      perror (argv[index]);
    else
      for (i = 0; value[i]; i++)
	printf ("%s\n", value[i]);
    index++;
  }
  return 0;
}

#endif /* TEST */
