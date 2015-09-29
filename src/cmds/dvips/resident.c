/*
 *   This code reads in and handles the defaults for the program from the
 *   file config.sw.  This entire file is a bit kludgy, sorry.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
/*
 *   This is the structure definition for resident fonts.  We use
 *   a small and simple hash table to handle these.  We don't need
 *   a big hash table.
 */
struct resfont *reshash[RESHASHPRIME] ;
/*
 *   These are the external routines we use.
 */
extern void error() ;
extern integer scalewidth() ;
extern void tfmload() ;
extern FILE *search() ;
extern shalfword pkbyte() ;
extern integer pkquad() ;
extern integer pktrio() ;
extern Boolean pkopen() ;
extern char *strcpy() ;
extern char *newstring() ;
extern void add_header() ;
/*
 *   These are the external variables we use.
 */
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern long bytesleft ;
extern quarterword *raster ;
extern FILE *pkfile ;
extern char *oname ;
extern integer swmem ;
extern char *tfmpath ;
extern char *pkpath ;
extern char *vfpath ;
extern char *figpath ;
extern char *nextstring ;
extern char *maxstring ;
extern Boolean disablecomments ;
extern Boolean compressed ;
extern int quiet ;
extern int filter ;
extern Boolean reverse ;
extern Boolean usesPSfonts ;
extern int actualdpi ;
extern int maxdrift ;
extern char *printer ;
extern char *mfmode ;
/*
 *   We use malloc here.
 */
char *malloc() ;
/*
 *   Our hash routine.
 */
int
hash(s)
   char *s ;
{
   int h = 12 ;

   while (*s != 0)
      h = (h + h + *s++) % RESHASHPRIME ;
   return(h) ;
}

/*
 *   The routine that looks up a font name.
 */
struct resfont *
lookup(name)
   char *name ;
{
   struct resfont *p ;

   for (p=reshash[hash(name)]; p!=NULL; p=p->next)
      if (strcmp(p->PSname, name)==0)
         return(p) ;
   return(NULL) ;
}
/*
 *   This routine adds an entry.
 */
void
add_entry(PSname, specinfo)
   char *PSname, *specinfo ;
{
   struct resfont *p ;
   int h ;

   p = (struct resfont *)malloc((unsigned int)sizeof(struct resfont)) ;
   if (p==NULL)
      error("! out of memory") ;
   p->PSname = PSname ;
   p->specialinstructions = specinfo ;
   h = hash(PSname) ;
   p->next = reshash[h] ;
   reshash[h] = p ;
}
/*
 *   Now our residentfont routine.
 */
Boolean
residentfont(curfnt)
        register fontdesctype *curfnt ;
{
   register shalfword i ;
   struct resfont *p ;

/*
 *   First we determine if we can find this font in the resident list.
 */
   if (*curfnt->area)
      return 0 ; /* resident fonts never have a nonstandard font area */
   if ((p=lookup(curfnt->name))==NULL)
      return 0 ;
/*
 *   We clear out some pointers:
 */
#ifdef DEBUG
   if (dd(D_FONTS))
        (void)fprintf(stderr,"Font %s is resident.\n", curfnt->name) ;
#endif  /* DEBUG */
   curfnt->resfont = p ;
   for (i=0; i<256; i++) {
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   tfmload(curfnt) ;
   usesPSfonts = 1 ;
   return(1) ;
}
static char was_inline[100] ;
void
bad_config() {
   error("Error in config file:") ;
   (void)fprintf(stderr, was_inline) ;
   exit(1) ;
}
/*
 *   Now we have the getdefaults routine.
 */
void
getdefaults(ext)
   char *ext ;
{
   FILE *deffile ;
   char PSname[100] ;
   register char *p ;
   char *specinfo, *fontname ;

   strcpy(PSname, "config.") ;
   strcat(PSname, ext) ;

   if ((deffile=search(CONFIGPATH,PSname))!=NULL) {
      while (fgets(was_inline, 100, deffile)!=NULL) {
       switch (was_inline[0]) {
case 'm' :
#ifdef SHORTINT
         if (sscanf(was_inline+1, "%ld", &swmem) != 1) bad_config() ;
#else   /* ~SHORTINT */
         if (sscanf(was_inline+1, "%d", &swmem) != 1) bad_config() ;
#endif  /* ~SHORTINT */
         break ;
case 'M' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         mfmode = newstring(PSname) ;
         break ;
case 'o' : case 'O' :
         p = was_inline + 1 ;
         while (*p && *p <= ' ')
            p++ ;
	 /* insert printer name */
	 if(strchr(p, '$')) {
	    char buf[200];
	    char *dollar = strchr(p, '$');
	    strncpy(buf, p, dollar-p); buf[dollar-p] = '\0';
	    if(printer != NULL)
	       strcat(buf, printer);
	    strcat(buf, dollar+1);
	    oname = newstring(buf);
	 } else {
	    oname = newstring(p);
	 }
         break ;
case 't' : case 'T' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else tfmpath = newstring(PSname) ;
         break ;
case 'p' : case 'P' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else pkpath = newstring(PSname) ;
         break ;
case 'v' : case 'V' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else vfpath = newstring(PSname) ;
         break ;
case 's' : case 'S' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else figpath = newstring(PSname) ;
         break ;
case ' ' : case '*' : case '#' : case ';' : case '=' : case 0 : case '\n' :
         break ;
case 'r' : case 'R' :
         reverse = 1 ;
         break ;
case 'D' :
         if (sscanf(was_inline+1, "%d", &actualdpi) != 1) bad_config() ;
         if (actualdpi < 10 || actualdpi > 10000) bad_config() ;
         break ;
case 'e' :
         if (sscanf(was_inline+1, "%d", &maxdrift) != 1) bad_config() ;
         if (maxdrift < 0) bad_config() ;
         break ;
case 'q' : case 'Q' :
         quiet = 1 ;
         break ;
case 'f' : case 'F' :
         filter = 1 ;
         break ;
case 'h' : case 'H' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config() ;
         else add_header(PSname) ;
         break ;
case 'N' :
         disablecomments = 1 ;
         break ;
case 'Z' :
         compressed = 1 ;
         break ;
default:
         bad_config() ;
      }
     }
     (void)fclose(deffile) ;
     if (printer)
        return ;
   }
   if ((deffile=search(CONFIGPATH,PSMAPFILE))!=NULL) {
     while (fgets(was_inline, 100, deffile)!=NULL) {
         specinfo = NULL ;
         fontname = NULL ;
         p = was_inline ;
         while (*p && *p <= ' ')
            p++ ;
         if (*p) fontname = p ;
         while (*p > ' ')
            p++ ;
         if (*p)
            *p++ = 0 ;
         while (*p && *p <= ' ')
            p++ ;
         if (*p != '"' && *p) {
            while (*p > ' ')
               p++ ;
            if (*p)
               *p++ = 0 ;
            while (*p && *p <= ' ')
               p++ ;
         }
         if (*p == '"') {
            p++ ;
            specinfo = p ;
            while (*p && *p != '"')
               p++ ;
            *p = 0 ;
         }
         add_entry(newstring(fontname), newstring(specinfo)) ;
       }
       (void)fclose(deffile) ;
   }
   checkenv() ;
}
/*
 *   Get environment variables! These override entries in ./config.h.
 */
checkenv() {
   char *p, *getenv() ;

   if (p=getenv("TEXFONTS"))
      tfmpath = newstring(p) ;
   if (p=getenv("TEXPKS"))
      pkpath = newstring(p) ;
   if (p=getenv("TEXINPUTS"))
      figpath = newstring(p) ;
}
