/*
 *   Here's some code to handle font libraries.  Not needed unless you are
 *   running on a system that can't handle files well.  Not endorsed by
 *   Tomas Rokicki or Radical Eye Software; use at your own risk.
 */
#ifdef FONTLIB
#include "structures.h"
#include "paths.h"
extern FILE *search() ;
extern char *newstring() ;
extern shalfword pkbyte() ;
extern void badpk() ;
extern char *malloc() ;
extern integer pkquad() ;
extern int debug_flag ;
extern char errbuf[] ;
extern char *flipath ;
extern char *fliname ;
extern FILE *pkfile ;
/*
 * font library structures
 */
struct fli_entry {
   unsigned long  offset;
   char          *name;
};

struct fli_size {
   unsigned long     size;
   halfword          entries;
   struct fli_entry *entry;
};

struct fli_lib {
   char            *name;
   FILE            *fp;
   halfword         sizes;
   struct fli_size *size;
   struct fli_lib  *next;
};

struct fli_lib *firstlib = NULL;

Boolean flib = 0;  /* non zero if reading a font library */

halfword
pkdouble()
{
   register halfword i ;
   i = pkbyte() ;
   i = i * 256 + pkbyte() ;
   return(i) ;
}
extern char name[] ;
/*
 *   fliload opens each font library, then reads in its
 *   directory for later use.  Font libraries are left
 *   open for later use by flisearch().
 *   loadfont() does not close pkfile if it is a font 
 *   library (flib non zero).
 */
void
fliload()
{
   int i ;
   halfword version1, version2;
   Boolean needext;
   char fontname[50]; 
   char name[50] ;
   char *fli;
   unsigned long dpi;
   halfword len, numsizes, numfonts;
   struct fli_lib *lib=NULL, *next_lib=NULL;
   struct fli_size *size;
   struct fli_entry *entry;

   fli = fliname;

   while (*fli) {
      /* get next font library name from fliname */
      needext=1;
      for (i=0; *fli && *fli!=PATHSEP; i++)
         if ( (name[i] = *fli++) == '.')
            needext=0;
      name[i] = '\0';
      if (*fli)
         fli++;  /* skip PATHSEP */
      if (*name) { 
         /* got fli name, now search for it */
         if (needext)
            strcat(name,".fli");

         if ( (pkfile=search(flipath,name,READBIN)) != (FILE *)NULL ) {
            /* for each font library */
            for (i=0; i<4; i++) {
              fontname[i] = pkbyte();  /* read header */
            }
            version1 = pkbyte();
            version2 = pkbyte();
            if (strncmp(fontname,"FLIB",4)!=0 || version1 != 2 || version2 != 0)
               badpk("incorrect font library format");

            (void) pkdouble();       /* ignore directory length */
            numsizes = pkdouble();   /* number of sizes */
            numfonts = pkdouble();   /* number of fonts */
            len = pkdouble();        /* length of comment */
            for (i=0; i<len; i++)
               (void)pkbyte();       /* skip comment */
#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Font library %s has %d font size%s, %d font%s\n",
         name, numsizes , numsizes !=1 ? "s" : "", 
         numfonts, numfonts!=1 ? "s" : "");
#endif /* DEBUG */

            next_lib =  (struct fli_lib *)malloc(sizeof(struct fli_lib)); 
            if (next_lib == (struct fli_lib *)NULL)
               error("! out of memory while reading font library") ;
            if (firstlib == (struct fli_lib *)NULL)
               firstlib = next_lib;
            else
               lib->next = next_lib;
            lib = next_lib;
            size = (struct fli_size *)
                        malloc(numsizes * sizeof(struct fli_size)); 
            if (size == (struct fli_size *)NULL)
               error("! out of memory while reading font library") ;
            entry = (struct fli_entry *)
                        malloc(numfonts * sizeof(struct fli_entry)); 
            if (entry == (struct fli_entry *)NULL)
               error("! out of memory while reading font library") ;
            lib->name = newstring(name);
            lib->fp = pkfile;         /* keep file open */
            lib->sizes = numsizes;
            lib->size = size;
            lib->next = (struct fli_lib *)NULL;

            for ( ;numsizes>0; numsizes--, size++) { 
               /* for each font size in this library */
               (void)pkdouble();      /* length of size entry - ignore */
               numfonts = pkdouble(); /* number of fonts */
               dpi = pkquad();        /* DPI (fixed point 16.16) */

#ifdef DEBUG
   if (dd(D_FONTS))
      (void)fprintf(stderr,"Font library %s size %.5gdpi has %d font%s\n", 
                  name, dpi/65536.0, numfonts, numfonts!=1 ? "s" : "");
#endif /* DEBUG */
               size->size    = dpi ;
               size->entries = numfonts ;
               size->entry   = entry ;
                  for ( ;numfonts > 0; numfonts--, entry++) {
                     /* read each entry */
                     (void)pkquad();            /* ignore length of font */
                     entry->offset = pkquad();  /* offset to font */
                     len = pkbyte();            /* length of name */
                     for (i=0; i<len; i++)
                        fontname[i] = pkbyte();
                     fontname[len] = '\0';
                     entry->name = newstring(fontname);
                  } /* end for numfonts>0 */
            }  /* end for numsizes>0 */
            /* keep font library open */
         }  /* end if opened library */
      } /* end if (*name) */
   }
}
   

/*
 *   flisearch searches all the font libraries for a PK font.
 *   returns FILE pointer positioned to PK font in font library
 */
FILE *
flisearch(n, dpi)
char *n;
halfword dpi;
{
   halfword dpi1, numsizes, numfonts;
   struct fli_lib *lib=NULL;
   struct fli_size *size;
   struct fli_entry *entry;
   int del ;
   
   if (firstlib == (struct fli_lib *)NULL)
      return((FILE *)NULL);  /* return if no font libraries */

#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr,"Trying %s at %ddpi\n", n, dpi);
#endif /* DEBUG */
   for (lib = firstlib; lib != (struct fli_lib *)NULL; lib = lib->next ) {
      /* for each font library */
      numsizes = lib->sizes ;
      size = lib->size ;
#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr,"  Searching %s\n", lib->name);
#endif /* DEBUG */
      for (; numsizes>0; numsizes--, size++) { 
         /* for each font size in this library */
         dpi1 = (halfword)((size->size+32768L)/65536) ;
         if ( dpi1 == dpi ) {
            /* if correct size then search for font */
#ifdef DEBUG
            if (dd(D_FONTS))
               (void)fprintf(stderr, "    Checking size %ddpi\n",dpi1);
#endif /* DEBUG */
            entry = size->entry ;
            for (numfonts=size->entries ;numfonts > 0; numfonts--, entry++) {
               if (strcmp(entry->name,n)==0) {
                  /* if correct font name then seek it */
                     pkfile = lib->fp;
                     flib = 1 ; /* this is a font library - don't close it */
                     (void)sprintf(name,"%s %s %ddpi",lib->name, n, dpi1) ;
                     if ( fseek(pkfile,entry->offset,0) )
                           badpk("couldn't seek font");
                        /* make sure it is a PK font */
                        if (pkbyte()==247) /* pre byte */
                           if (pkbyte()==89) {  /* id byte */
                              if ( fseek(pkfile,entry->offset,0) )
                                 badpk("couldn't seek font");
                              return(pkfile); /* found it */
                           }
                        sprintf(errbuf,"%s %s %ddpi isn't PK format, ignoring",
                              lib->name, n, dpi1);
                        error(errbuf);
               } /* end if name correct */
            } /* end for numfonts>0 */
         }
         else {
            /* if not correct size then skip */
#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr, "    Skipping size %ddpi\n", dpi1);
#endif /* DEBUG */
         }
      }  /* end for numsizes>0 */
   }
   return((FILE *)NULL);
}

/* parse the font library path, putting all directory names in path, 
 * and all font library names in name.  
 * Directory names have a trailing DIRSEP.
 */
char *
fliparse(path, name)
        char *path, *name ;
{
   char *p, *prevp ;           /* pointers to path */
   char *n, *prevn ;           /* pointers to name */
   char *s ;

   p = path ;
   n = name ;
   s = path ;

   while (*s) {
      prevp = p ;
      prevn = n ;
      while (*s && *s != PATHSEP) {
         /* copy till PATHSEP */
         *p++ = *s; 
         *n++ = *s;
         s++;
      }  
      *n = '\0' ;
      if (*s)
         s++;  /* skip PATHSEP */

      if ( *prevn=='\0' || prevn[strlen(prevn)-1] == DIRSEP ) {
         n = prevn ; /* ignore name if it is dir */
         if (*prevn)
            p--;     /* backup over DIRSEP */
         *p++ = PATHSEP;
         prevp = p ;
      }
      else {
         p = prevp ; /* ignore path if it is library name */
         *n++ = PATHSEP;
         prevn = n ;
      }

   }
   *p = '\0' ;
   *n = '\0' ;
   if (n!=name && *--n==PATHSEP)
      *n = '\0'; /* remove trailing PATHSEP from name */
   if (p!=path && *--p==PATHSEP)
      *p = '\0'; /* remove trailing PATHSEP from path */
   return(path);
}               /* end fliparse */
#else
/*
 *   Some systems don't like .o files that compile to nothing, so we
 *   provide a stub routine.
 */
void fliload() {}
#endif

