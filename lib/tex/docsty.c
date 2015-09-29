/*
  DOCSTY.C

  Convert TeX DOC files to TeX STY files for faster loading.
  Tony Li
  5/22/86

  Copyright (c) 1986
  University of Southern California
*/

#include <stdio.h>
#include <ctype.h>
#define STRLEN 256
#define WHITE 1
#define BLACK 2
#define COMMENT '%'
#define SP ' '
#define NL '\n'
#define CR '\r'
#define THRESH 79

#define SAVECHAR	{buf[linelen++] = ch;}
#define TESTBUF		{if (linelen > THRESH) dumpline(out);}

char buf [STRLEN];
int linelen = 0;

main (argc, argv)
int argc;
char *argv[];
{
  char dest[STRLEN];
  FILE *in, *out;
  int state, ret;
  char ch;

  strcpy (dest, argv[1]);
  strcat (argv[1], ".doc");
  strcat (dest, ".sty");
  in = fopen (argv[1], "r");
#ifdef vms
  out = fopen (dest, "w", "rfm=var", "rat=cr");
#endif
#ifdef unix
  out = fopen (dest, "w");
#endif
  state = WHITE;
  while ((ch = getc(in)) != EOF) {
    if (ch == COMMENT)		/* If its a comment, read the rest. */
      while ((ch = getc (in)) != EOF && ch != NL);
    if (isspace (ch)) {		/* If its blank then */
      if (state == BLACK) {	/* if we haven't seen a space then */
	ch = SP;
	TESTBUF;		/* put it out */
	SAVECHAR;		/* and append a space */
	state = WHITE;		/* and we've seen a space */
      }				/* otherwise, ignore */
    }
    else {			/* its a char */
	SAVECHAR;		/* save it */
	state = BLACK;		/* we saw it */
      }
  };
  emptybuf (out);
  fclose (in);
  fclose (out);
}

dumpline (out)			/* Figure out where to dump the buffer. */
     FILE *out;			/* Invariant: linelen points past the */
{				/* end, so you can always put a NULL there. */
  char *pt, *st;

  buf[linelen] = NULL;		/* Terminate the string */
  pt = &buf[linelen - 1];	/* Point to the char before */
  st = &buf[0];			/* Point to the start */
  while (!isspace(*pt) && pt > st)
    pt--;			/* Run back until a space */
  if (pt == st) {		/* Theres no middle space */
    fputs (st, out);		/* Put it out */
    fputc (NL, out);		/* Make it a line */
    linelen = 0;		/* No chars left */
  }
  else {
    *pt = NULL;			/* End the string */
    fputs (st, out);		/* Put it out */
    fputc (NL, out);		/* Make it a line */
    pt++;			/* Point to the remaining string */
    while (*pt) *st++ = *pt++;	/* Copy the string */
    *st = NULL;			/* Terminate it */
    linelen = strlen (buf);	/* Figure the new length */
  };
}

emptybuf (out)			/* Clear the buffer */
     FILE *out;
{
  buf[linelen] = NULL;		/* Terminate it */
  fputs (buf, out);		/* Put it out */
  fputc (NL, out);		/* Make it a line */
}
