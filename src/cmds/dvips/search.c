/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory. if the file SUBDIR exists and the file is a font file,
 *   it checks for the file in a subdirectory named the same as the font name.
 *   Returns the open file descriptor if ok, else NULL.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include <stdio.h>              /* for FILE and fopen */
#ifdef SYSV
#include <string.h>
#define MAXPATHLEN (128)
#else   /* ~SYSV */
#include <sys/param.h>          /* for MAXPATHLEN */
#include <strings.h>            /* for strlen */
#endif  /* ~SYSV */
/*
 *
 *   We hope MAXPATHLEN is enough -- only rudimentary checking is done!
 */

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */

/* argument to fopen */
#define READ            "r"
/* directories are separated in the path by PATHSEP */
#define PATHSEP         ':'
/* DIRSEP is the char that separates directories from files */
#define DIRSEP          '/'
extern void error() ;

FILE *
search(path, file)
        char *path, *file ;
{
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */

   if (*file == DIRSEP) {               /* if full path name */
      if ((fd=fopen(file,READ)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }

   do {
      /* copy the current directory into fname */
      nam = fname;
      /* copy till PATHSEP */
      while (*path != PATHSEP && *path) *nam++ = *path++;
      if (nam == fname) *nam++ = '.';   /* null component is current dir */
      *nam++ = DIRSEP;                  /* add separator */

      (void)strcpy(nam,file);                   /* tack the file on */

      /* belated check -- bah! */
      if ((nam - fname) + strlen(file) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"Trying to open %s\n", fname) ;
#endif
      if ((fd=fopen(fname,READ)) != NULL)
         return(fd);

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

