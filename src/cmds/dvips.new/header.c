/*
 *   This routine handles the PostScript prologs that might
 *   be included through:
 *
 *      - Default
 *      - Use of PostScript fonts
 *      - Specific inclusion through specials, etc.
 *      - Use of graphic specials that require them.
 *
 *   Things are real simple.  We build a linked list of headers to
 *   include.  Then, when the time comes, we simply copy those
 *   headers down.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
struct header_list *header_head ;
/*
 *   The external routines we use.
 */
extern char *malloc() ;
extern char *newstring() ;
extern void error() ;
extern void copyfile() ;
#ifdef DEBUG
extern integer debug_flag ;
#endif
/*
 *   This more general routine adds a name to a list of unique
 *   names.
 */
int
add_name(s, what)
   char *s ;
   struct header_list **what ;
{
   struct header_list *p, *q ;

   for (p = *what ; p != NULL; p = p->next)
      if (strcmp(p->name, s)==0)
         return 0 ;
   q = (struct header_list *)malloc((unsigned)sizeof(struct header_list)) ;
   if (q==NULL)
      error("! out of memory") ;
   q->next = NULL ;
   q->name = newstring(s) ;
   if (*what == NULL)
      *what = q ;
   else {
      for (p = *what; p->next != NULL; p = p->next) ;
      p->next = q ;
   }
   return 1 ;
}
/*
 *   This routine is responsible for adding a header file.
 */
int
add_header(s)
char *s ;
{
#ifdef DEBUG
   if (dd(D_HEADER))
      (void)fprintf(stderr, "Adding header file \"%s\"\n", s) ;
#endif
   return add_name(s, &header_head) ;
}
/*
 *   This routine runs down a list, returning each in order.
 */
char *
get_name(what)
   struct header_list **what ;
{
   if (what && *what) {
      char *p = (*what)->name ;
      *what =  (*what)->next ;
      return p ;
   } else
      return 0 ;
}
/*
 *   This routine actually sends the headers.
 */
void
send_headers() {
   struct header_list *p = header_head ;
   char *q ;

   while (q=get_name(&p)) {
#ifdef DEBUG
      if (dd(D_HEADER))
         (void)fprintf(stderr, "Sending header file \"%s\"\n", q) ;
#endif
      copyfile(q) ;
   }
}
