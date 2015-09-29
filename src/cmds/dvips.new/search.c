/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory. if the file SUBDIR exists and the file is a font file,
 *   it checks for the file in a subdirectory named the same as the font name.
 *   Returns the open file descriptor if ok, else NULL.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include <ctype.h>
#ifdef SYSV
#define MAXPATHLEN (256)
#else
#ifdef VMS
#define MAXPATHLEN (256)
#else   /* ~SYSV */
#include <sys/param.h>          /* for MAXPATHLEN */
#endif  /* ~SYSV */
#endif
#ifndef MSDOS
#ifndef VMS
#include <pwd.h>
#endif
#endif
/*
 *
 *   We hope MAXPATHLEN is enough -- only rudimentary checking is done!
 */

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern char *mfmode ;
extern int actualdpi ;

FILE *
search(path, file, mode)
        char *path, *file, *mode ;
{
   extern char *getenv(), *newstring() ;
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
   if (*file == DIRSEP) {               /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }

#ifdef MSDOS
   if ( isalpha(file[0]) && file[1]==':' ) {   /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#endif

   do {
      /* copy the current directory into fname */
      nam = fname;
      /* copy till PATHSEP */
      if (*path == '~') {
         char *p = nam ;
         path++ ;
         while (*path && *path != PATHSEP && *path != DIRSEP)
            *p++ = *path++ ;
         *p = 0 ;
         if (*nam == 0) {
            if (home == 0) {
               if (home = getenv("HOME"))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#ifdef MSDOS
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
            struct passwd *pw = getpwnam(fname) ;
            if (pw)
               strcpy(fname, pw->pw_dir) ;
            else
               error("no such user") ;
#endif
#endif
         }
         nam = fname + strlen(fname) ;
      }
      while (*path != PATHSEP && *path) *nam++ = *path++;
      *nam = 0 ;
#ifndef VMS
      if (nam == fname) *nam++ = '.';   /* null component is current dir */

      if (*file != '\0') {
         *nam++ = DIRSEP;                  /* add separator */
         (void)strcpy(nam,file);                   /* tack the file on */
      }
      else
         *nam = '\0' ;
#else
      (void)strcpy(nam,file);                   /* tack the file on */
#endif
      /* belated check -- bah! */
      if ((nam - fname) + strlen(file) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"Trying to open %s\n", fname) ;
#endif
      if ((fd=fopen(fname,mode)) != NULL)
         return(fd);

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

FILE *
pksearch(path, file, mode, n, dpi)
        char *path, *file, *mode ;
	char *n ;
	halfword dpi ;
{
   extern char *getenv(), *newstring() ;
   register char *nam ;                 /* index into fname */
   register FILE *fd ;                  /* file desc of file */
   char fname[MAXPATHLEN] ;             /* to store file name */
   static char *home = 0 ;              /* home is where the heart is */
   for (nam=path; *nam; nam++)
      if (*nam == '%')
         break ;
   if (*nam == 0)
      return search(path, file, mode) ;
   if (*file == DIRSEP) {               /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#ifdef MSDOS
   if ( isalpha(file[0]) && file[1]==':' ) {  /* if full path name */
      if ((fd=fopen(file,mode)) != NULL)
         return(fd) ;
      else
         return(NULL) ;
   }
#endif
   do {
      /* copy the current directory into fname */
      nam = fname;
      /* copy till PATHSEP */
      if (*path == '~') {
         char *p = nam ;
         path++ ;
         while (*path && *path != PATHSEP && *path != DIRSEP)
            *p++ = *path++ ;
         *p = 0 ;
         if (*nam == 0) {
            if (home == 0) {
               if (home = getenv("HOME"))
                  home = newstring(home) ;
               else
                  home = "." ;
            }
            strcpy(fname, home) ;
         } else {
#ifdef MSDOS
            error("! ~username in path???") ;
#else
#ifdef VMS
            error("! ~username in path???") ;
#else
            struct passwd *pw = getpwnam(fname) ;
            if (pw)
               strcpy(fname, pw->pw_dir) ;
            else
               error("no such user") ;
#endif
#endif
         }
         nam = fname + strlen(fname) ;
      }
      /* copy till PATHSEP */
      while (*path != PATHSEP && *path) {
         if (*path == '%') {
            path++ ;
            switch(*path) {
               case 'd': sprintf(nam, "%d", dpi) ; break ;
               case 'f': strcpy(nam, n) ; break ;
               case 'm': if (mfmode == 0)
                            if (actualdpi == 300) mfmode = "imagen" ;
                            else if (actualdpi == 400) mfmode = "nexthi" ;
                            else if (actualdpi == 635) mfmode = "linolo" ;
                            else if (actualdpi == 1270) mfmode = "linohi" ;
                            else if (actualdpi == 2540) mfmode = "linosuper" ;
                         if (mfmode == 0)
                            error("! MF mode not set, but used in pk path") ;
                         strcpy(nam, mfmode) ;
                         break ;
               case 'p': strcpy(nam, "pk") ; break ;
               case '%': strcpy(nam, "%") ; break ;
               default: error("! bad format character in pk path") ;
            }
            nam = fname + strlen(fname) ;
            if (*path)
               path++ ;
         } else
            *nam++ = *path++;
      }
#ifndef VMS
      if (nam == fname) *nam++ = '.';   /* null component is current dir */
#endif /* VMS */

      *nam = '\0' ;

      /* belated check -- bah! */
      if (strlen(fname) + 1 > MAXPATHLEN)
         error("! overran allocated storage in search()");

#ifdef DEBUG
      if (dd(D_PATHS))
         (void)fprintf(stderr,"Trying to open %s\n", fname) ;
#endif
      if ((fd=fopen(fname,mode)) != NULL)
         return(fd);

   /* skip over PATHSEP and try again */
   } while (*(path++));

   return(NULL);

}               /* end search */

/* do we report file openings? */

#ifdef DEBUG
#ifdef fopen
#undef fopen
#endif
FILE *my_real_fopen(n, t)
register char *n, *t ;
{
   FILE *tf ;
   if (dd(D_FILES)) {
      fprintf(stderr, "<%s(%s)> ", n, t) ;
      tf = fopen(n, t) ;
      if (tf == 0)
         fprintf(stderr, "failed\n") ;
      else
         fprintf(stderr, "succeeded\n") ;
   } else
      tf = fopen(n, t) ;
   return tf ;
}
#endif
