#include <stdio.h>
/* This is used by the `assert' macro.  */
void
__eprintf (string, line, filename)
     char *string;
     int line;
     char *filename;
{
  fprintf (stderr, string, line, filename);
}
