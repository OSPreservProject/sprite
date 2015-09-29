/* ar.c - Archive modify and extract.
   Copyright (C) 1988 Free Software Foundation, Inc.

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

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/ar/RCS/ar.c,v 1.6 90/11/12 11:08:45 kupfer Exp $";
#endif

#include <stdio.h>
#include <ar.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <bstring.h>
#include <assert.h>

#if !defined(A_OUT) && !defined(MACH_O)
#define A_OUT
#endif

#ifdef A_OUT
#ifdef COFF_ENCAPSULATE
#include "a.out.encap.h"
#else
#include <a.out.h>
#endif
#endif

#ifdef MACH_O
#ifndef A_OUT
#include <nlist.h>
#endif
#include <sys/loader.h>
#endif

#ifdef USG
#include <time.h>
#include <fcntl.h>
#else
#include <sys/file.h>
#include <sys/time.h>
#endif

#ifdef	__GNUC__
#define	alloca	__builtin_alloca
#else
# ifdef sparc
#  include <alloca.h>
# else
char *alloca ();
# endif
#endif

#ifdef	USG
#define	bcopy(source, dest, size)	memcpy((dest), (source), (size))
#define	bcmp(a, b, size)		memcmp((a), (b), (size))
#define	bzero(s, size)			memset((s), 0, (size))
#endif

/* If LOCKS is defined, locking is enabled.  If LOCK_FLOCK is defined,
   then BSD-style flock() is used.  Otherwise, fcntl() is used.  If
   LOCKS is not defined, LOCK_FLOCK is irrelevant.

   Locking is normally disabled because fcntl hangs on the Sun
   and it isn't supported properly across NFS anyway.  */

#ifdef LOCKS

/* This flag tells whether the archive was opened read-only or
   read-write.  It is used for a sanity check before trying to write a
   new archive.  It is also temporarily as a workaround for a Sprite
   bug that requires flock() to pass in the type of lock that is being
   unlocked.  */

int open_flags;				/* O_RDONLY or O_RDWRITE */

#ifndef LOCK_FLOCK
/* You might need to compile with -I/usr/include/sys if your fcntl.h
   isn't in /usr/include (which is where it should be according to POSIX).  */
#include <fcntl.h>
#endif /* LOCK_FLOCK */

void lock_a_file (), unlock_a_file ();
#endif /* LOCKS */

/* This structure represents member names.  It is here to simplify 
   dealing with different truncation schemes and with names that the
   user specifies as paths.  Operations deal with the `stored' name,
   except we maintain a mapping to the name the user gave for
   reading/writing the member.   If the user didn't specify a name
   that matches the `stored' name, the `given' name is NULL.  For
   names provided by the user, the `stored' name is computed
   immediately, so all member_name objects should have a non-null
   `stored' name.  */

struct member_name {
  char *given;				/* name given by the user */
  char *stored;				/* name as stored in the ar header */
};

#define Empty_Name(name)	((name)->stored == NULL)

/* This structure is used internally to represent the info
   on a member of an archive.  This is to make it easier to change format.  */

struct member_desc
  {
    /* `given' will be zero if the user didn't specifically name this
       member.  The name will be empty if this member is marked for
       deletion.  */
    struct member_name name;

    /* The following fields are stored in the member header as decimal or octal
       numerals, but in this structure they are stored as machine numbers.  */
    int mode;		/* Protection mode from member header.  */
    long int date;	/* Last modify date as stored in member header.  */
    unsigned int size;	/* Bytes of member's data, from member header.  */
    int uid, gid;	/* UID and GID fields copied from member header.  */
    unsigned int offset;/* Offset in archive of the header of this member.  */
    unsigned int data_offset;/* Offset of first data byte of the member.  */

    /* The next field does not describe where the member was in the
       old archive, but rather where it will be in the modified archive.
       It is set up by write_archive.  */
    unsigned int new_offset;	/* Offset of this member in new archive */

    /* Symdef data for member.  Used only for files being inserted.  */
    struct symdef *symdefs;
    unsigned int nsymdefs;	/* Number of entries of symdef data.  */
    unsigned int string_size;	/* Size of strings needed by symdef data.  */
  };

/* Each symbol is recorded by something like this.  */

struct symdef
  {
    union
      {
	unsigned long int stringoffset;
	char *name;
      } s;
    unsigned long int offset;
  };

/* Nonzero means that it's the name of an existing member;
   position new or moved files with respect to this one.  */

struct member_name *posname;


/* How to use `posname':
   POS_BEFORE means position before that member.
   POS_AFTER means position after that member.
   POS_DEFAULT if position by default; then `posname' should also be zero. */

enum { POS_DEFAULT, POS_BEFORE, POS_AFTER } postype;

/* Nonzero means describe each action performed.  */

int verbose;

/* Nonzero means don't warn about creating the archive file if necessary.  */

int silent_create;

/* Nonzero means don't replace existing members whose
   dates are more recent than the corresponding files.  */

int newer_only;

/* Nonzero means preserve dates of members when extracting them.  */

int preserve_dates;

/* Operation to be performed.  */

#define DELETE 1
#define REPLACE 2
#define PRINT_TABLE 3
#define PRINT_FILES 4
#define EXTRACT 5
#define MOVE 6
#define QUICK_APPEND 7

int operation;

/* Name of archive file.  */

char *archive;

/* Descriptor for the archive file.  This descriptor is used for
   locking the archive.  -1 if the archive is not yet opened.  */

int arcfd;

/* File pointer for the archive, used only for reading the archive.  0 
   if the archive hasn't been opened yet.  */

FILE *arcstream;

/* (Pointer to) an array of file names specified by the user.  The 
   last element is a dummy, with an "empty" name.  The remaining
   elements have both `given' and `header' filled in.  Zero if the
   user didn't specify any file names.  */

struct member_name *file_args;

/* *** Lots of globals related to the __.SYMDEF member.***  */

/* Name "__.SYMDEF", converted to "member_name" form. */

struct member_name symdef_name;

/* Nonzero means write a __.SYMDEF member into the modified archive.  */

int symdef_flag;

/* Nonzero means __.SYMDEF member exists in old archive.  */

int symdef_exists;

/* Nonzero means don't update __.SYMDEF unless the flag was given.  */

int ignore_symdef;

/* Total number of symdef entries we will have. */

unsigned long int nsymdefs;

/* Symdef data from old archive (set up only if we need it) */

struct symdef *old_symdefs;

/* Number of symdefs in remaining in old_symdefs.  */

unsigned int num_old_symdefs;

/* Number of symdefs old_symdefs had when it was read in.  */

unsigned long int original_num_symdefs;

/* String table from old __.SYMDEF member.  */

char *old_strings;

/* Size of old_strings */

unsigned long int old_strings_size;

/* String table to be written into __.SYMDEF member.  */

char *new_strings;

/* Size of new_strings */

unsigned long int new_strings_size;

/* ***End of __.SYMDEF globals.*** */

/* Controls the way in which long names are truncated.  If non-zero,
   SomeVeryLongName.o is converted to SomeVeryLongN.o.  Otherwise, it
   is converted to SomeVeryLongNam (which is compatible with the BSD
   ar).  */

#ifndef GNU_TRUNCATION
#define GNU_TRUNCATION	1
#endif

int gnu_truncation = GNU_TRUNCATION;

/* An archive map is a chain of these structures.
  Each structure describes one member of the archive.
  The chain is in the same order as the members are.  */

struct mapelt
  {
    struct member_desc info;
    struct mapelt *next;
  };

struct mapelt *maplast;

/* If nonzero, this is the map-element for the __.SYMDEF member
   and we should update the time of that member just before finishing.  */

struct mapelt *symdef_mapelt;

/* Header that we wrote for the __.SYMDEF member.  */

struct ar_hdr symdef_header;

char *xmalloc (), *xrealloc ();
void free ();

void add_to_map (), delete_from_map ();
int insert_in_map ();
void print_descr ();
char *concat ();
void scan ();
void extract_members ();
void extract_member ();
void print_contents ();
void write_symdef_member ();
void read_old_symdefs ();
void two_operations ();
void usage (), fatal (), error (), error_with_file ();
void perror_with_name (), pfatal_with_name ();
void open_archive ();
void write_archive ();
void touch_symdef_member ();
void update_symdefs ();
void delete_members (), move_members (), replace_members ();
void quick_append ();
void init_elt (), mark_as_deleted ();
int marked_for_deletion ();
int name_match ();
void init_name (), free_name_strings ();
int move_in_map ();
int filter_symbols ();
char *user_to_header ();
struct member_name *make_file_args ();
void verify_is_archive ();
#if DEBUG
void verify_symdefs ();
#endif

/* Output BYTES of data at BUF to the descriptor DESC.
   FILE is the name of the file (for error messages).  */

void
mywrite (desc, buf, bytes, file)
     int desc;
     char *buf;
     int bytes;
     char *file;
{
  register int val;

  while (bytes > 0)
    {
      val = write (desc, buf, bytes);
      if (val <= 0)
	perror_with_name (file);
      buf += val;
      bytes -= val;
    }
}

int
main (argc, argv)
     int argc;
     char **argv;
{
  int i;

  operation = 0;
  verbose = 0;
  newer_only = 0;
  silent_create = 0;
  posname = 0;
  postype = POS_DEFAULT;
  preserve_dates = 0;
  init_name (&symdef_name, "__.SYMDEF");
  symdef_flag = 0;
  symdef_exists = 0;
  ignore_symdef = 0;
  symdef_mapelt = 0;
  file_args = 0;
  arcfd = -1;
  arcstream = 0;

  if (argc < 2)
    usage ("too few command arguments", 0);

  {
    char *key = argv[1];
    char *p = key;
    char c;

    while (c = *p++)
      {
	switch (c)
	  {
	  case 'a':
	    postype = POS_AFTER;
	    break;

	  case 'b':
	    postype = POS_BEFORE;
	    break;

	  case 'c':
	    silent_create = 1;
	    break;

	  case 'd':
	    if (operation)
	      two_operations ();

	    operation = DELETE;
	    break;

	  case 'i':
	    postype = POS_BEFORE;
	    break;

	  case 'l':
	    break;

	  case 'm':
	    if (operation)
	      two_operations ();
	    operation = MOVE;
	    break;

	  case 'o':
	    preserve_dates = 1;
	    break;

	  case 'p':
	    if (operation)
	      two_operations ();
	    operation = PRINT_FILES;
	    break;

	  case 'q':
	    if (operation)
	      two_operations ();
	    operation = QUICK_APPEND;
	    break;

	  case 'r':
	    if (operation && operation != REPLACE)
	      two_operations ();
	    operation = REPLACE;
	    break;

	  case 's':
	    symdef_flag = 1;
	    break;

	  case 't':
	    if (operation)
	      two_operations ();
	    operation = PRINT_TABLE;
	    break;

	  case 'u':
	    if (operation && operation != REPLACE)
	      two_operations ();
	    operation = REPLACE;
	    newer_only = 1;
	    break;

	  case 'v':
	    verbose = 1;
	    break;

	  case 'x':
	    if (operation)
	      two_operations ();
	    operation = EXTRACT;
	    break;
	  }
      }
  
  }

  if (operation == 0 && symdef_flag)
    operation = REPLACE;

  if (operation == 0)
    usage ("no operation specified", 0);

  i = 2;

  if (postype != POS_DEFAULT)
    posname = make_file_args(&argv[i++], 1);

  archive = argv[i++];

  if (i < argc)
    {
      file_args = make_file_args(&argv[i], argc - i);
      while (i < argc)
	if (!strcmp (argv[i++], "__.SYMDEF"))
	  {
	    ignore_symdef = 1;
	    break;
	  }
    }

  switch (operation)
    {
    case EXTRACT:
	extract_members (extract_member);
	break;

    case PRINT_TABLE:
	extract_members (print_descr);
	break;

    case PRINT_FILES:
	extract_members (print_contents);
	break;

    case DELETE:
	if (file_args != 0)
	  delete_members ();
	break;

    case MOVE:
	if (file_args != 0)
	  move_members ();
	break;

    case REPLACE:
	if (file_args != 0 || symdef_flag)
	  replace_members ();
	break;

    case QUICK_APPEND:
	if (file_args != 0)
	  quick_append ();
	break;

    default:
	usage ("invalid operation %d", operation);
    }

  exit (0);
  return 0;
}

void
two_operations ()
{
  usage ("two different operation switches specified", 0);
}

/* Apply the given function to all members in the archive.  */

void
scan (function, crflag)
     void (*function) ();
     int crflag;
{
  if (arcstream == 0)
    open_archive (O_RDONLY);

  if (arcstream == 0 && crflag)
    /* Creation-warning, if desired, will happen later.  */
    return;

  if (arcstream == 0)
    pfatal_with_name (archive);
  verify_is_archive (arcfd);

  /* Now find the members one by one.  */
  {
    int member_offset = SARMAG;
    while (1)
      {
	int nread;
	struct ar_hdr member_header;
	struct member_desc member_desc;
	char name [1 + sizeof member_header.ar_name];

	if (fseek (arcstream, member_offset, 0) < 0)
	  perror_with_name (archive);

	nread = fread (&member_header, 1, sizeof (struct ar_hdr), arcstream);
	if (nread == 0)
	  /* No data left means end of file; that is OK.  */
	  break;

	if (nread != sizeof (member_header)
	    || bcmp (member_header.ar_fmag, ARFMAG, 2))
	  fatal ("file %s not a valid archive", archive);
	bcopy (member_header.ar_name, name, sizeof member_header.ar_name);

	/* remove trailing blanks */
	{
	  char *p = name + sizeof member_header.ar_name;
	  *p = '\0';
	  while (p > name && *--p == ' ')
	    *p = '\0';
	}

	/* Make a safe copy of the name, so that `function' can just 
	   make a copy of `member_desc'.  */
	member_desc.name.stored = concat (name, "", "");
	member_desc.name.given = NULL;

	sscanf (member_header.ar_mode, "%o", &member_desc.mode);
	member_desc.date = atoi (member_header.ar_date);
	member_desc.size = atoi (member_header.ar_size);
	member_desc.uid = atoi (member_header.ar_uid);
	member_desc.gid = atoi (member_header.ar_gid);
	member_desc.offset = member_offset;
	member_desc.data_offset = member_offset + sizeof (member_header);

	member_desc.new_offset = 0;
	member_desc.symdefs = 0;
	member_desc.nsymdefs = 0;
	member_desc.string_size = 0;

	if (!ignore_symdef && !strcmp (name, "__.SYMDEF"))
	  symdef_exists = 1;

	function (member_desc, arcstream);

	member_offset += sizeof (member_header) + member_desc.size;
	if (member_offset & 1)
	  ++member_offset;
      }
  }
}

void print_modes ();

void
print_descr (member)
     struct member_desc member;
{
  char *timestring;
  if (!verbose)
    {
      puts (member.name.stored);
      return;
    }
  print_modes (member.mode);
  timestring = ctime (&member.date);
  printf (" %2d/%2d %6d %12.12s %4.4s %s\n",
	  member.uid, member.gid,
	  member.size, timestring + 4, timestring + 20,
	  member.name.stored);
}

void
print_modes (modes)
     int modes;
{
  putchar (modes & 0400 ? 'r' : '-');
  putchar (modes & 0200 ? 'w' : '-');
  putchar (modes & 0100 ? 'x' : '-');
  putchar (modes & 040 ? 'r' : '-');
  putchar (modes & 020 ? 'w' : '-');
  putchar (modes & 010 ? 'x' : '-');
  putchar (modes & 04 ? 'r' : '-');
  putchar (modes & 02 ? 'w' : '-');
  putchar (modes & 01 ? 'x' : '-');
}

#define BUFSIZE 1024

void
extract_member (member, istream)
     struct member_desc member;
     FILE *istream;
{
  int ncopied = 0;
  FILE *ostream;
  char *filename;			/* name to store file into */

  fseek (istream, member.data_offset, 0);
  filename = (member.name.given ? member.name.given : member.name.stored);
  ostream = fopen (filename, "w");
  if (!ostream)
    {
      perror_with_name (filename);
      return;
    }

  if (verbose)
    printf ("x - %s\n", filename);

  while (ncopied < member.size)
    {
      char buf [BUFSIZE];
      int tocopy = member.size - ncopied;
      int nread;
      if (tocopy > BUFSIZE) tocopy = BUFSIZE;
      nread = fread (buf, 1, tocopy, istream);
      if (nread != tocopy)
	fatal ("file %s not a valid archive", archive);
      fwrite (buf, 1, nread, ostream);
      ncopied += tocopy;
    }

#ifdef USG
  chmod (filename, member.mode);
#else
  fchmod (fileno (ostream), member.mode);
#endif
  if (ferror (ostream) || fclose (ostream) != 0)
    error ("%s: I/O error", filename);

  if (preserve_dates)
    {
#ifdef USG
      long tv[2];
      tv[0] = member.date;
      tv[1] = member.date;
      utime (filename, tv);
#else
      struct timeval tv[2];
      tv[0].tv_sec = member.date;
      tv[0].tv_usec = 0;
      tv[1].tv_sec = member.date;
      tv[1].tv_usec = 0;
      utimes (filename, tv);
#endif
    }
}

void
print_contents (member, istream)
     struct member_desc member;
     FILE *istream;
{
  int ncopied = 0;

  fseek (istream, member.data_offset, 0);

  if (verbose)
    printf ("\n<member %s>\n\n", member.name.stored);

  while (ncopied < member.size)
    {
      char buf [BUFSIZE];
      int tocopy = member.size - ncopied;
      int nread;
      if (tocopy > BUFSIZE) tocopy = BUFSIZE;
      nread = fread (buf, 1, tocopy, istream);
      if (nread != tocopy)
	fatal ("file %s not a valid archive", archive);
      fwrite (buf, 1, nread, stdout);
      ncopied += tocopy;
    }
}

/* Make a map of the existing members of the archive: their names,
 positions and sizes.  */

/* If `nonexistent_ok' is nonzero,
 just return 0 for an archive that does not exist.
 This will cause the ordinary supersede procedure to
 create a new archive.  */

struct mapelt *
make_map (nonexistent_ok)
     int nonexistent_ok;
{
  struct mapelt mapstart;
  mapstart.next = 0;
  maplast = &mapstart;
  scan (add_to_map, nonexistent_ok);
  return mapstart.next;
}

void
add_to_map (member)
     struct member_desc member;
{
  struct mapelt *mapelt = (struct mapelt *) xmalloc (sizeof (struct mapelt));

  mapelt->info = member;
  maplast->next = mapelt;
  mapelt->next = 0;
  maplast = mapelt;
}

/* Return the last element of the specified map.  */

struct mapelt *
last_mapelt (map)
     struct mapelt *map;
{
  struct mapelt *tail = map;
  while (tail->next) tail = tail->next;
  return tail;
}

/* Return the element of the specified map which precedes elt.  */

struct mapelt *
prev_mapelt (map, elt)
     struct mapelt *map, *elt;
{
  struct mapelt *tail = map;
  while (tail->next && tail->next != elt)
    tail = tail->next;
  if (tail->next) return tail;
  return 0;
}

/* Return the element of the specified map which has the specified 
   name.  Possible side effect: if NAME or the matching element has a
   known `given' (user) name, that name is propagated so that both
   NAME and the matching element have it.  */

struct mapelt *
find_mapelt_noerror (map, name)
     struct mapelt *map;
     struct member_name *name;
{
  register struct mapelt *tail;

  for (tail = map; tail != 0; tail = tail->next)
    {
      if (marked_for_deletion (tail))
	continue;
      if (name_match (&tail->info.name, name))
	return tail;
    }

  return 0;
}

struct mapelt *
find_mapelt (map, name)
     struct mapelt *map;
     struct member_name *name;
{
  register struct mapelt *found = find_mapelt_noerror (map, name);
  if (found == 0)
    error ("no member named `%s'", name->stored);
  return found;
}

/* Open the archive, either read-only or read-write, using the global
   name "archive".  The archive is locked at this time to protect
   against a concurrent writer.  This lock will be released when the
   archive is closed.  This routine should only be called once--no
   upgrading of access from read-only to read-write is allowed.

   Side effects:
   - If opening read-write and the archive doesn't exist, create it.
   - arcfd and arctream are set to mean the opened archive.  If the
     archive doesn't exist and can't be created, they are left as
     meaning "unopened".  */

void
open_archive (how)
     int how;				/* O_RDONLY or O_RDWR */
{
  void open_for_reading (), open_for_update ();
  if (arcfd != -1 || arcstream != 0)
    fatal ("opening archive twice");

  switch (how)
    {
    case O_RDONLY:
      open_for_reading ();
      break;
    case O_RDWR:
      open_for_update ();
      break;
    default:
      fatal ("bogus flag passed to open_archive");
    }
}

/* Open the archive for read-only access.  If the archive doesn't
   exist, just quit.  */

void
open_for_reading ()
{
  arcfd = open (archive, O_RDONLY, 0);
  if (arcfd < 0 && errno != ENOENT)
    pfatal_with_name (archive);
  if (arcfd < 0)
    return;

  lock_a_file (arcfd, O_RDONLY);

  arcstream = fdopen (arcfd, "r");
  if (arcstream == 0)
    fatal ("can't make stream for archive");
}

/* Open the archive for read-write access.  If it doesn't exist,
   create it.  The order of creates and opens and locks is to keep
   competing ar's (spawned by pmake) from tripping on each other.  */

void
open_for_update ()
{
  struct stat statbuf;

  /* Assume that the archive doesn't exist, and try to create it.  If 
     it does exist, just open it normally.  */
  arcfd = open (archive, O_RDWR | O_CREAT | O_EXCL, 0666);
  if (arcfd < 0 && errno != EEXIST)
    pfatal_with_name (archive);

  if (arcfd >= 0)
    {
      if (!silent_create)
	printf ("Creating archive file `%s'\n", archive);
    }
  else
    arcfd = open (archive, O_RDWR, 0);

  /* If the file suddenly doesn't exist, punt.  Some user must have
     manually deleted the file.  */
  if (arcfd < 0)
    pfatal_with_name (archive);

  lock_a_file (arcfd, O_RDWR);

  /* Whew.  Now that we've got the file and it's locked, check whether
     it's really an archive or just an empty shell, created either by
     us or by a competing ar.  */

  fstat (arcfd, &statbuf);
  if (statbuf.st_size == 0)
    mywrite (arcfd, ARMAG, SARMAG, archive);
  else
    verify_is_archive (arcfd);

  arcstream = fdopen (arcfd, "r+");
  if (arcstream == 0)
    fatal ("can't create stream for archive");
}

#ifndef LOCKS

void
lock_a_file (fd, how)
     int fd, how;
{
}

void
unlock_a_file (fd)
     int fd;
{
}

#else /* LOCKS */

/* Lock the old file so that it won't be written while there are
   readers or another writer.
   Non-sprite systems use the fcntl locking facility found on Sun
   systems, which is also in POSIX.  (Perhaps it comes from sysV.)  */

#ifndef LOCK_FLOCK

void
lock_a_file (fd, how)
     int fd;
     int how;				/* read/write flag */
{
  struct flock lock;

  lock.l_type = (how == O_RDONLY ? F_RDLCK : F_WRLCK);
  lock.l_whence = 0;
  lock.l_start = 0;
  lock.l_len = 0;

  while (1)
    {
      int value = fcntl (fd, F_SETLKW, &lock);
      if (value >= 0)
	break;
      else if (errno == EINTR)
	continue;
      else
	pfatal_with_name ("locking archive");
    }
}

void
unlock_a_file (fd)
     int fd;
  {
    struct flock lock;

    /* Unlock the old archive.  */

    lock.l_type = F_UNLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;

    fcntl (fd, F_SETLK, &lock);
  }

#else /* LOCK_FLOCK */

void
lock_a_file (fd, how)
     int fd;
     int how;				/* read/write flag */
{
  int lock_type = (how == O_RDONLY ? LOCK_SH : LOCK_EX);

  open_flags = how;
  if (flock (fd, lock_type) < 0)
    pfatal_with_name (archive);
}

/* Putting lock_flags in the flock() call is a workaround for a bug in 
   Sprite's flock() emulation.  -mdk 19-Oct-90 */

void
unlock_a_file (fd)
     int fd;
{
  int lock_type = (open_flags == O_RDONLY ? LOCK_SH : LOCK_EX);

  if (flock (fd, LOCK_UN | lock_type) < 0)
    pfatal_with_name (archive);
}

#endif /* LOCK_FLOCK */
#endif /* LOCKS */

/* Unlock archive and close the file descriptor.  */

void
close_archive ()
{
#ifndef USG
  fsync (arcfd);
#endif
  unlock_a_file (arcfd);
  if (close (arcfd) < 0)
    {
      perror_with_name (archive);
      exit (1);
    }
}

/* Ensure that the given file is an archive.  Side effect: repositions
   the archive.  After calling this routine, you should do a seek.  */

void
verify_is_archive (fd)
     int fd;
{
  char buf[SARMAG];
  int nread;

  lseek (fd, 0, L_SET);
  nread = read (fd, buf, sizeof (buf));
  if (nread != SARMAG || bcmp (buf, ARMAG, SARMAG) != 0)
    fatal ("file %s not a valid archive", archive);
}

/* Write a new archive file from a given map.  */
/* When a map is used as the pattern for a new archive,
   each element represents one member to put in it, and
   the order of elements controls the order of writing.
  
   Ordinarily, the element describes a member of the old
   archive, to be copied into the new one.
  
   If, however, the `offset' field of the element's info is 0,
   then the element describes a file to be copied into the
   new archive.

   The archive is updated by writing a new file and then copying the
   new file onto the old one.  We don't use rename, because if some
   other "ar" has the archive open and is waiting to obtain the lock,
   it would end up with the old file, not the new one.  There can
   never be two ar's writing the new file simultaneously, because of
   the lock on the archive.
*/

char *make_tempname ();
void copy_out_member ();
void copy_file ();

void
write_archive (map, appendflag)
     struct mapelt *map;
     int appendflag;
{
  char *tempname = make_tempname (archive);
  int indesc = arcfd;
  int outdesc;
  char *outname;
  struct mapelt *tail;

  /* Sanity check */

  if (open_flags == O_RDONLY)
    fatal ("want to update archive after declaring read-only");

  /* Now open the output.  */

  if (!appendflag)
    {
      /* Write the revised archive to TEMPNAME, then copy it back. */

      outdesc = open (tempname, O_RDWR | O_CREAT, 0666);
      if (outdesc < 0)
	pfatal_with_name (tempname);
      outname = tempname;
      mywrite (outdesc, ARMAG, SARMAG, outname);
    }
  else
    {
      /* Fast-append to existing archive.  */

      outdesc = open (archive, O_WRONLY | O_APPEND, 0);
      if (outdesc < 0)
	pfatal_with_name (archive);
      outname = archive;
    }

  /* If archive has or should have a __.SYMDEF member,
     compute the contents for it.  */

  if (symdef_flag || symdef_exists)
    {
      if (symdef_exists)
	read_old_symdefs (map, indesc);
      else
	{
	  struct mapelt *this = (struct mapelt *)
	    xmalloc (sizeof (struct mapelt));
	  init_name (&this->info.name, "__.SYMDEF");
	  this->info.offset = SARMAG;
	  this->info.data_offset = SARMAG + sizeof (struct ar_hdr);
	  this->info.new_offset = 0;
	  this->info.date = 0;
	  this->info.size = 0;
	  this->info.uid = 0;
	  this->info.gid = 0;
	  this->info.mode = 0666;
	  this->info.symdefs = 0;
	  this->info.nsymdefs = 0;
	  this->info.string_size = 0;
	  this->next = map;
	  map = this;
	  original_num_symdefs = 0;
	  old_strings_size = 0;
	}

      update_symdefs (map, indesc);
    }

  /* Copy the members into the output, either from the old archive
     or from specified files.  */

  for (tail = map; tail != 0; tail = tail->next)
    {
      if ((symdef_flag || symdef_exists) && !marked_for_deletion(tail)
	  && name_match (&tail->info.name, &symdef_name)
#if 0
	  && tail->info.date==0
#endif
	  )
	write_symdef_member (tail, map, outdesc, outname);
      else
	copy_out_member (tail, indesc, outdesc, outname);
    }

  if (symdef_mapelt != 0)
    {
      /* Check for members whose data offsets weren't
	 known when the symdef member was first written.  */
      int doneany = 0;
      for (tail = map; tail != 0; tail = tail->next)
	if (tail->info.offset == 0)
	  {
	    /* Fix up the symdefs.  */
	    register unsigned int i;
	    for (i = 0; i < tail->info.nsymdefs; ++i)
	      tail->info.symdefs[i].offset = tail->info.new_offset;
	    doneany = 1;
	  }
      if (doneany)
	{
	  /* Some files had bad symdefs; rewrite the symdef member.  */
	  lseek (outdesc, symdef_mapelt->info.offset, 0);
	  write_symdef_member (symdef_mapelt, map, outdesc, outname);
	}
    }

  /* Mark the __.SYMDEF member as up to date.  */

  if (symdef_mapelt != 0)
    touch_symdef_member (outdesc, outname);

  if (!appendflag)
    copy_file (outdesc, tempname, arcfd, archive);
  close (outdesc);
  if (!appendflag)
    unlink (tempname);

  close_archive ();
}

/* Copy all of fromfd to tofd.  The file names are passed in for error
   reporting.  */

void
copy_file (from_fd, from_name, to_fd, to_name)
     int from_fd, to_fd;
     char *from_name, *to_name;
{
  char buf[8192];
  int nchars;

  lseek (from_fd, 0, L_SET);
  lseek (to_fd, 0, L_SET);
  if (ftruncate (to_fd, 0) < 0)
    pfatal_with_name (to_name);

  while ((nchars = read (from_fd, buf, sizeof (buf))) > 0)
    {
      if (write (to_fd, buf, nchars) != nchars) 
	pfatal_with_name (to_name);
    }
  if (nchars < 0)
    pfatal_with_name (from_name);
}

/* Fill in an ar header for an element of the archive.  */

void
header_from_map (header, mapelt)
     struct ar_hdr *header;
     struct mapelt *mapelt;
{
  /* Zero the header, then store in the data as text.  */
  bzero ((char *) header, sizeof (*header));

  assert (mapelt->info.name.stored != NULL);
  assert (strlen (mapelt->info.name.stored) < sizeof (header->ar_name));
  strcpy (header->ar_name, mapelt->info.name.stored);

  sprintf (header->ar_date, "%ld", mapelt->info.date);
  sprintf (header->ar_size, "%d", mapelt->info.size);
  sprintf (header->ar_uid, "%d", mapelt->info.uid);
  sprintf (header->ar_gid, "%d", mapelt->info.gid);
  sprintf (header->ar_mode, "%o", mapelt->info.mode);
  strncpy (header->ar_fmag, ARFMAG, sizeof (header->ar_fmag));

  /* Change all remaining nulls in the header into spaces.  */
  {
    char *end = (char *) &header[1];
    register char *p;
    for (p = (char *) header; p < end; ++p)
      if (*p == '\0')
	*p = ' ';
  }
}

/* writes to file open on OUTDESC with name OUTNAME.  */
void
copy_out_member (mapelt, archive_indesc, outdesc, outname)
     struct mapelt *mapelt;
     int archive_indesc;
     int outdesc;
     char *outname;
{
  struct ar_hdr header;
  int indesc;

  if (marked_for_deletion (mapelt))
    return;

  header_from_map (&header, mapelt);

  /* Either copy the member from the (old) archive, or copy it from 
     the user-named filed.  */
  if (mapelt->info.offset != 0)
    {
      indesc = archive_indesc;
      lseek (indesc, mapelt->info.data_offset, 0);
    }
  else
    {
      assert (mapelt->info.name.given != NULL);
      indesc = open (mapelt->info.name.given, O_RDONLY, 0);
      if (indesc < 0)
	{
	  perror_with_name (mapelt->info.name.given);
	  return;
	}
    }

  mywrite (outdesc, &header, sizeof (header), outname);

  if (mapelt->info.data_offset == 0)
    mapelt->info.data_offset = lseek (outdesc, 0L, 1);

  {
    char buf[BUFSIZE];
    int tocopy = mapelt->info.size;
    while (tocopy > 0)
      {
	int thistime = tocopy;
	if (thistime > BUFSIZE) thistime = BUFSIZE;
        read (indesc, buf, thistime);
	mywrite (outdesc, buf, thistime, outname);
	tocopy -= thistime;
      }
  }

  if (indesc != archive_indesc)
    close (indesc);

  if (mapelt->info.size & 1)
    mywrite (outdesc, "\n", 1, outname);
}

/* Update the time of the __.SYMDEF member; done when we updated
   that member, just before we close the new archive file.
   It is open on OUTDESC and its name is OUTNAME.  */

void
touch_symdef_member (outdesc, outname)
     int outdesc;
     char *outname;
{
  struct stat statbuf;
  int i;

  /* See what mtime the archive file has as a result of our writing it.  */
  fstat (outdesc, &statbuf);

  /* Advance member's time to that time.  */
  bzero (symdef_header.ar_date, sizeof symdef_header.ar_date);
  sprintf (symdef_header.ar_date, "%ld", statbuf.st_mtime);
  for (i = 0; i < sizeof symdef_header.ar_date; i++)
    if (symdef_header.ar_date[i] == 0)
      symdef_header.ar_date[i] = ' ';

  /* Write back this member's header with the new time.  */
  if (lseek (outdesc, symdef_mapelt->info.new_offset, 0) >= 0)
    mywrite (outdesc, &symdef_header, sizeof symdef_header, outname);
}

char *
make_tempname (name)
     char *name;
{
  return concat (name, "", "_supersede");
}

void
delete_members ()
{
  struct mapelt *map;
  struct mapelt mapstart;
  struct member_name *name;

  open_archive (O_RDWR);

  map = make_map (0);
  init_elt (&mapstart);
  mapstart.next = map;
  map = &mapstart;

  if (file_args)
    for (name = file_args; !Empty_Name(name); name++)
      {
	/* If user says to delete the __.SYMDEF member,
	   don't make a new one to replace it.  */
	if (name_match (name, &symdef_name))
	  symdef_exists = 0;
	delete_from_map (name, map);
      }

  write_archive (map->next, 0);
}

void
delete_from_map (name, map)
     struct member_name *name;
     struct mapelt *map;
{
  struct mapelt *this = find_mapelt (map, name);

  if (!this) return;
  mark_as_deleted (this);
  if (verbose)
    printf ("d - %s\n", name->stored);
}

void
move_members ()
{
  struct mapelt *map;
  struct member_name *name;
  struct mapelt *after_mapelt;
  struct mapelt mapstart;
  struct mapelt *change_map;

  open_archive (O_RDWR);

  map = make_map (0);
  init_elt (&mapstart);
  mapstart.next = map;
  change_map = &mapstart;

  switch (postype)
    {
    case POS_DEFAULT:
      after_mapelt = last_mapelt (change_map);
      break;

    case POS_AFTER:
      after_mapelt = find_mapelt (map, posname);
      break;

    case POS_BEFORE:
      after_mapelt = prev_mapelt (change_map, find_mapelt (map,
							   posname));
      break;

    default:
      after_mapelt = 0;			/* lint */
      fatal ("bogus position type");	/* "can't happen" */
    }

  /* Failure to find specified "before" or "after" member
     is a fatal error; message has already been printed.  */

  if (!after_mapelt) exit (1);

  if (file_args)
    for (name = file_args; !Empty_Name(name); name++)
      {
	if (move_in_map (name, change_map, after_mapelt))
	  after_mapelt = after_mapelt->next;
      }

  write_archive (change_map->next, 0);
}

int
move_in_map (name, map, after)
     struct member_name *name;
     struct mapelt *map, *after;
{
  struct mapelt *this = find_mapelt (map, name);
  struct mapelt *prev;

  if (!this)
    return 0;
  prev = prev_mapelt (map, this);
  if (this == after || prev == after)
    return 1;				/* no-op */
  prev->next = this->next;
  this->next = after->next;
  after->next = this;
  return 1;
}

/* Insert files into the archive.  */

void
replace_members ()
{
  struct mapelt *map;
  struct mapelt mapstart;
  struct mapelt *after_mapelt;
  struct mapelt *change_map;
  struct member_name *name;
  int changed;

  open_archive (O_RDWR);

  map = make_map (1);
  init_elt (&mapstart);
  mapstart.next = map;
  change_map = &mapstart;

  switch (postype)
    {
    case POS_DEFAULT:
      after_mapelt = last_mapelt (change_map);
      break;

    case POS_AFTER:
      after_mapelt = find_mapelt (map, posname);
      break;

    case POS_BEFORE:
      after_mapelt = prev_mapelt (change_map, find_mapelt (map, posname));
      break;

    default:
      after_mapelt = 0;			/* lint */
      fatal ("bogus position type");	/* "can't happen" */
    }

  /* Failure to find specified "before" or "after" member
     is a fatal error; the message has already been printed.  */
  if (after_mapelt == 0)
    exit (1);

  changed = 0;
  if (file_args)
    for (name = file_args; !Empty_Name(name); ++name)
      if (insert_in_map (name, change_map, after_mapelt))
	{
	  after_mapelt = after_mapelt->next;
	  changed = 1;
	}

  if (!changed && (!symdef_flag || symdef_exists))
    /* Nothing changed.  */
    close_archive ();
  else
    write_archive (change_map->next, 0);
}

/* Handle the "quick insert" operation.  */

void
quick_append ()
{
  struct mapelt *map;
  struct mapelt *after;
  struct mapelt mapstart;
  struct member_name *name;

  init_elt (&mapstart);
  mapstart.next = 0;
  map = &mapstart;
  after = map;

  open_archive (O_RDWR);

  /* Insert the specified files into the "map",
     but is a map of the inserted files only,
     and starts out empty.  */
  if (file_args)
    for (name = file_args; !Empty_Name(name); name++)
      {
	if (insert_in_map (name, map, after))
	  after = after->next;
      }

  /* Append these files to the end of the existing archive file.  */

  write_archive (map->next, 1);
}

/* Insert an entry for REQUESTED_NAME into the map MAP after the map
   entry AFTER. 
   Deletes any old entry for REQUESTED_NAME.
   MAP is assumed to start with a dummy entry, which facilitates
   insertion at the beginning of the list.
   Return 1 if successful, 0 if did nothing because file
   REQUESTED_NAME doesn't exist or (optionally) is older.  */

int
insert_in_map (requested_name, map, after)
     struct member_name *requested_name;
     struct mapelt *map, *after;
{
  struct mapelt *old = find_mapelt_noerror (map, requested_name);
  struct mapelt *this;
  struct stat status;

  assert (requested_name->given != NULL);
  if (stat (requested_name->given, &status))
    {
      perror_with_name (requested_name->given);
      return 0;
    }
  if (old && newer_only && status.st_mtime <= old->info.date)
    return 0;

  this = (struct mapelt *) xmalloc (sizeof (struct mapelt));
  if (old)
    mark_as_deleted (old);

  this->info.name = *requested_name;
  this->info.offset = 0;
  this->info.data_offset = 0;
  this->info.date = status.st_mtime;
  this->info.size = status.st_size;
  this->info.uid = status.st_uid;
  this->info.gid = status.st_gid;
  this->info.mode = status.st_mode;
  this->next = after->next;
  after->next = this;

  if (verbose)
    printf ("%c - %s\n", old == 0 ? 'a' : 'r', this->info.name.stored);

  return 1;
}

/* Apply a function to each of the specified members.
*/

void
extract_members (function)
     void (*function) ();
{
  struct mapelt *map;
  struct member_name *name;

  if (!file_args)
    {
      /* Handle case where we want to operate on every member.
	 No need to make a map and search it for this.  */
      scan (function, 0);
      return;
    }

  if (arcstream == 0)
    open_archive (O_RDONLY);
  if (arcstream == 0)
    fatal ("failure opening archive %s for the second time", archive);
  verify_is_archive (arcfd);
  map = make_map (0);

  for (name = file_args; !Empty_Name(name); name++)
    {
      struct mapelt *this = find_mapelt (map, name);
      if (!this) continue;
      function (this->info, arcstream);
    }
}

/* Write the __.SYMDEF member from data in core.  OUTDESC and OUTNAME
   are descriptor and name of file to write to.  */

void
write_symdef_member (mapelt, map, outdesc, outname)
     struct mapelt *mapelt;
     struct mapelt *map;
     int outdesc;
     char *outname;
{
  struct ar_hdr header;
  struct mapelt *mapptr;
  unsigned long int symdefs_size;
  int symdef_sanity_count;

  if (marked_for_deletion (mapelt))
    return;

  header_from_map (&header, mapelt);

  bcopy (&header, &symdef_header, sizeof header);

  mywrite (outdesc, &header, sizeof (header), outname);

  /* Write the number of bytes taken by the symdef table.  */
  symdefs_size = nsymdefs * sizeof (struct symdef);
  mywrite (outdesc, &symdefs_size, sizeof symdefs_size, outname);

  /* Write symdefs surviving from old archive.  */
  mywrite (outdesc, old_symdefs, num_old_symdefs * sizeof (struct symdef),
	   outname);
  symdef_sanity_count = num_old_symdefs;

#if DEBUG
  verify_symdefs (map, old_symdefs, num_old_symdefs, new_strings);
#endif

  /* Write symdefs for new members.  */
  for (mapptr = map; mapptr != 0; mapptr = mapptr->next)
    if (mapptr->info.nsymdefs != 0)
      {
	write (outdesc, mapptr->info.symdefs,
	       mapptr->info.nsymdefs * sizeof (struct symdef));
#if DEBUG
	verify_symdefs (map, mapptr->info.symdefs, mapptr->info.nsymdefs,
			new_strings);
#endif
	symdef_sanity_count += mapptr->info.nsymdefs;
      }

  if (symdef_sanity_count != nsymdefs)
    fatal ("bug: wrote wrong number of symdefs");

  /* Write the string table size.  */
  mywrite (outdesc, &new_strings_size, sizeof new_strings_size, outname);

  /* Write the string table.  */
  mywrite (outdesc, new_strings, new_strings_size, outname);

  if (mapelt->info.size & 1)
    mywrite (outdesc, "", 1, outname);
}

#if DEBUG

/* Verify that the given symdefs point to the right place.  Treat the
   symbol as the name of a .o file and look it up.  If we find it,
   make sure we are pointing to it and not to some other .o file.  */

void
verify_symdefs (map, symdefs, numsymdefs, string_table)
     struct mapelt *map;
     struct symdef *symdefs;
     unsigned int numsymdefs;
     char *string_table;
{
  struct symdef *sym;
  struct member_name file_name;
  struct mapelt *member;
  char *tmp_name;

  for (sym = symdefs; sym < symdefs+numsymdefs; ++sym)
    {
      tmp_name = concat (string_table + sym->s.stringoffset, ".o", "");
      init_name (&file_name, (tmp_name[0] == '_' ? tmp_name+1 : tmp_name));
      member = find_mapelt_noerror (map, &file_name);
      if (member)
	{
	  if (member->info.new_offset)
	    {
	      if (member->info.new_offset != sym->offset)
		{
		  printf ("Symbol %s points to 0x%x, which doesn't match \
0x%x (old 0x%x)\n",
			  string_table + sym->s.stringoffset,
			  sym->offset,
			  member->info.new_offset,
			  member->info.offset);
		  abort ();
		}
	    }
	  /* else no new offset, so check the old one */
	  else if (member->info.offset != sym->offset)
	    {
	      printf ("Symbol %s points to 0x%x, which doesn't match 0x%x\n",
		      string_table + sym->s.stringoffset,
		      sym->offset,
		      member->info.offset);
	      abort();
	    }
	}
      free_name_strings (&file_name);
      free (tmp_name);
    }
}

#endif /* DEBUG */

void
read_old_symdefs (map, archive_indesc)
     struct mapelt *map;
     int archive_indesc;
{
  struct mapelt *mapelt;
  char *data;
  int symdefs_size;

  mapelt = find_mapelt_noerror (map, &symdef_name);
  if (!mapelt)
    abort ();			/* Only call here if an old one exists */

  data  = (char *) xmalloc (mapelt->info.size);
  lseek (archive_indesc, mapelt->info.data_offset, 0);
  if (read (archive_indesc, data, mapelt->info.size) !=
      mapelt->info.size)
    pfatal_with_name (archive);

  symdefs_size = *(unsigned long int *) data;
  original_num_symdefs = symdefs_size / sizeof (struct symdef);
  old_symdefs = (struct symdef *) (data + sizeof (symdefs_size));
  old_strings_size
    = *(unsigned long int *) (old_symdefs + original_num_symdefs);
  old_strings = ((char *) (old_symdefs + original_num_symdefs)
		 + sizeof (old_strings_size));
#if DEBUG
  verify_symdefs (map, old_symdefs, original_num_symdefs, old_strings);
#endif
}

/* Read various information from the header of an object file.
   Return 0 for failure or 1 for success.  */

int
read_header_info (mapelt, desc, offset, syms_offset, syms_size, strs_offset,
		  strs_size)
     struct mapelt *mapelt;
     int desc;
     long int offset;
     long int *syms_offset;
     unsigned int *syms_size;
     long int *strs_offset;
     unsigned int *strs_size;
{
#ifdef A_OUT
  {
    struct exec hdr;

    lseek (desc, offset, 0);
#ifdef HEADER_SEEK_FD
    HEADER_SEEK_FD (desc);
#endif

    if (read (desc, (char *) &hdr, sizeof hdr) == sizeof hdr && !N_BADMAG(hdr))
      {
	*syms_offset = N_SYMOFF (hdr);
	*syms_size = hdr.a_syms;
	*strs_offset = N_STROFF (hdr);
	lseek (desc, N_STROFF (hdr) + offset, 0);
	if (read (desc, (char *) strs_size, sizeof *strs_size) != sizeof *strs_size)
	  {
	    error_with_file ("failure reading string table size in ", mapelt);
	    return 0;
	  }
	return 1;
      }
  }
#endif

#ifdef MACH_O
  {
    struct mach_header mach_header;
    struct load_command *load_command;
    struct symtab_command *symtab_command;
    char *hdrbuf;
    int cmd, symtab_seen;

    lseek (desc, offset, 0);
    if (read (desc, (char *) &mach_header, sizeof mach_header) == sizeof mach_header
	&& mach_header.magic == MH_MAGIC)
      {
	hdrbuf = xmalloc (mach_header.sizeofcmds);
	if (read (desc, hdrbuf, mach_header.sizeofcmds) != mach_header.sizeofcmds)
	  {
	    error_with_file ("failure reading load commands of ", mapelt);
	    return 0;
	  }
	load_command = (struct load_command *) hdrbuf;
	symtab_seen = 0;
	for (cmd = 0; cmd < mach_header.ncmds; ++cmd)
	  {
	    if (load_command->cmd == LC_SYMTAB)
	      {
		symtab_seen = 1;
		symtab_command = (struct symtab_command *) load_command;
		*syms_offset = symtab_command->symoff;
		*syms_size = symtab_command->nsyms * sizeof (struct nlist);
		*strs_offset = symtab_command->stroff;
		*strs_size = symtab_command->strsize;
	      }
	    load_command = (struct load_command *) ((char *) load_command + load_command->cmdsize);
	  }
	free (hdrbuf);
	if (!symtab_seen)
	  *syms_offset = *syms_size = *strs_offset = *strs_size = 0;
	return 1;
      }
  }
#endif

  error_with_file ("bad format (not an object file) in ", mapelt);
  return 0;
}

/* Create the info.symdefs for a new member
   by reading the file it is coming from.  */

void
make_new_symdefs (mapelt, archive_indesc)
     struct mapelt *mapelt;
     int archive_indesc;
{
  int indesc;
  long int syms_offset, strs_offset;
  unsigned int syms_size, strs_size;
  struct nlist *symbols;
  int symcount;
  char *strings;
  register unsigned int i;
  unsigned long int offset;

  if (marked_for_deletion (mapelt))
    fatal ("trying to make symdefs for deleted member");

  /* Either use an existing member from the archive, or use the 
     user-specified file.  */

  if (mapelt->info.offset != 0)
    {
      indesc = archive_indesc;
      lseek (indesc, mapelt->info.data_offset, 0);
      offset = mapelt->info.data_offset;
    }
  else
    {
      assert (mapelt->info.name.given != NULL);
      indesc = open (mapelt->info.name.given, O_RDONLY, 0);
      if (indesc < 0)
	{
	  perror_with_name (mapelt->info.name.given);
	  return;
	}
      offset = 0;
    }

  if (!read_header_info (mapelt, indesc, (long) offset, &syms_offset,
			 &syms_size, &strs_offset, &strs_size))
    {
      if (mapelt->info.offset == 0)
	close (indesc);
      return;
    }

  /* Number of symbol entries in the file.  */
  symcount = syms_size / sizeof (struct nlist);
  /* Allocate temporary space for the symbol entries.  */
  symbols = (struct nlist *) alloca (syms_size);
  /* Read in the symbols.  */
  lseek (indesc, syms_offset + offset, 0);
  if (read (indesc, (char *) symbols, syms_size) != syms_size)
    {
      error_with_file ("premature end of file in symbols of ", mapelt);
      if (mapelt->info.offset == 0)
	(void) close (indesc);
      return;
    }

  /* The string table size includes the size word.  */
  if (strs_size < sizeof (strs_size))
    {
      error_with_file ("bad string table size in ", mapelt);
      if (mapelt->info.offset == 0)
	(void) close (indesc);
      return;
    }
  strs_size -= sizeof (strs_size);

  /* Allocate permanent space for the string table.  */
  strings = (char *) xmalloc (strs_size);

  /* Read in the strings.  */
  lseek (indesc, offset + strs_offset + sizeof strs_size, 0);
  if (read (indesc, strings, strs_size) != strs_size)
    {
      error_with_file ("premature end of file in strings of ", mapelt);
      if (mapelt->info.offset == 0)
	(void) close (indesc);
      return;
    }

  if (indesc != archive_indesc)
    (void) close (indesc);

  /* Discard the symbols we don't want to mention; compact the rest down.  */
  symcount = filter_symbols (symbols, (unsigned) symcount);

  mapelt->info.symdefs = (struct symdef *)
    xmalloc (symcount * sizeof (struct symdef));
  mapelt->info.nsymdefs = symcount;
  mapelt->info.string_size = 0;

  for (i = 0; i < symcount; ++i)
    {
      unsigned long int stroff = symbols[i].n_un.n_strx - sizeof (strs_size);
      char *symname = strings + stroff;
      if (stroff > strs_size)
	{
	  char buf[100];
	  sprintf (buf, "ridiculous string offset %lu in symbol %u of ",
		   stroff + sizeof (strs_size), i);
	  error_with_file (buf, mapelt);
	  return;
	}
      mapelt->info.symdefs[i].s.name = symname;
      mapelt->info.string_size += strlen (symname) + 1;
    }
}

/* Choose which symbol entries to mention in __.SYMDEF;
   compact them downward to get rid of the rest.
   Return the number of symbols left.  */

int
filter_symbols (syms, symcount)
     struct nlist *syms;
     unsigned int symcount;
{
  struct nlist *from, *to;
  struct nlist *end = syms + symcount;

  for (to = from = syms; from < end; ++from)
    if ((from->n_type & N_EXT)
	&& (from->n_type != N_EXT || from->n_value != 0))
      *to++ = *from;

  return to - syms;
}


/* Update the __.SYMDEF data before writing a new archive.  */

void
update_symdefs (map, archive_indesc)
     struct mapelt *map;
     int archive_indesc;
{
  struct mapelt *tail;
  int pos;
  register unsigned int i;
  unsigned int len;
  struct symdef *s;
  unsigned long int deleted_strings_size = 0;
  unsigned long *newoffsets = 0;

  nsymdefs = original_num_symdefs;
  num_old_symdefs = original_num_symdefs;
  new_strings_size = old_strings_size;

  if (nsymdefs != 0)
    {
      /* We already had a __.SYMDEF member, so just update it.  */

      /* Mark as cancelled any old symdefs for members being deleted.  */

      for (tail = map; tail != 0; tail = tail->next)
	{
	  if (marked_for_deletion (tail))
	    {
	      assert (tail->info.offset != 0);
	      /* Old member being deleted.  Delete its symdef entries too.  */
	      for (i = 0; i < original_num_symdefs; i++)
		if (old_symdefs[i].offset == tail->info.offset)
		  {
		    old_symdefs[i].offset = 0;
		    --nsymdefs;
		    deleted_strings_size
		      += strlen (old_strings
				 + old_symdefs[i].s.stringoffset) + 1;
		  }
	    }
	}

      /* Compactify old symdefs.  */
      {
	register unsigned int j = 0;
	for (i = 0; i < num_old_symdefs; ++i)
	  {
	    if (j != i)
	      old_symdefs[j] = old_symdefs[i];
	    if (old_symdefs[i].offset != 0)
	      ++j;
	  }
	num_old_symdefs -= i - j;
      }

      /* Create symdef data for any new members.  */
      for (tail = map; tail != 0; tail = tail->next)
	{
	  if (tail->info.offset != 0
	      || marked_for_deletion (tail)
	      || name_match (&tail->info.name, &symdef_name))
	    continue;
	  make_new_symdefs (tail, archive_indesc);
	  nsymdefs += tail->info.nsymdefs;
	  new_strings_size += tail->info.string_size;
	}
    }
  else
    {
      /* Create symdef data for all existing members.  */

      for (tail = map; tail != 0; tail = tail->next)
	{
	  if (marked_for_deletion (tail)
	      || name_match (&tail->info.name, &symdef_name))
	    continue;
	  make_new_symdefs (tail, archive_indesc);
	  nsymdefs += tail->info.nsymdefs;
	  new_strings_size += tail->info.string_size;
	}
    }

  new_strings_size -= deleted_strings_size;
  old_strings_size -= deleted_strings_size;

#if DEBUG
  verify_symdefs (map, old_symdefs, num_old_symdefs, old_strings);
#endif

  /* Now we know the size of __.SYMDEF,
     so assign the positions of all the members.  */

  tail = find_mapelt_noerror (map, &symdef_name);
  tail->info.size = (sizeof (nsymdefs) + (nsymdefs * sizeof (struct symdef))
		     + sizeof (new_strings_size) + new_strings_size);
  symdef_mapelt = tail;

  pos = SARMAG;
  for (tail = map; tail != 0; tail = tail->next)
    {
      if (marked_for_deletion (tail))
	continue;
      tail->info.new_offset = pos;
      pos += sizeof (struct ar_hdr) + tail->info.size;
      if (tail->info.size & 1)
	++pos;
    }

  /* Now update the offsets in the symdef data to be the new offsets
     rather than the old ones.  We can't update the old symdefs in
     place, because the new offset for one member might match the old
     offset for another member.  So for the old symdefs we mark in a
     separate array what the new offsets are and then update the
     symdefs after going through all the members.  */

  newoffsets =
    (unsigned long *)xmalloc (num_old_symdefs * sizeof(unsigned long));
  bzero (newoffsets, num_old_symdefs * sizeof (unsigned long));

  for (tail = map; tail != 0; tail = tail->next)
    {
      if (marked_for_deletion (tail))
	continue;
      if (tail->info.symdefs == 0)
	{
	  /* Member without new symdef data.
	     Check the old symdef data; it may be included there. */
	  assert (tail->info.offset != 0);
	  for (i = 0; i < num_old_symdefs; i++)
	    {
	      if (old_symdefs[i].offset == tail->info.offset)
		newoffsets[i] = tail->info.new_offset;
	    }
	}
      else
	for (i = 0; i < tail->info.nsymdefs; i++)
	  tail->info.symdefs[i].offset = tail->info.new_offset;
    }

  /* Actually update any old symdefs that have new offsets. */
  for (i = 0; i < num_old_symdefs; i++)
    if (newoffsets[i] != 0)
      old_symdefs[i].offset = newoffsets[i];

  free (newoffsets);
  newoffsets = 0;

#if DEBUG
  verify_symdefs (map, old_symdefs, num_old_symdefs, old_strings);
#endif

  /* Generate new, combined string table and put each string's offset into the
     symdef that refers to it.  Note that old symdefs ref their strings by
     offsets into old_strings but new symdefs contain addresses of strings.  */

  new_strings = (char *) xmalloc (new_strings_size);
  pos = 0;

  /* Write the strings of the old symdefs and update the structures
     to contain indices into the string table instead of strings.  */
  for (i = 0; i < num_old_symdefs; i++)
    {
      strcpy (new_strings + pos, old_strings + old_symdefs[i].s.stringoffset);
      old_symdefs[i].s.stringoffset = pos;
      pos += strlen (new_strings + pos) + 1;
    }
  if (pos < old_strings_size)
    {
      unsigned int d = old_strings_size - pos;
      /* Correct the string table size.  */
      new_strings_size -= d;
      /* Correct the size of the `__.SYMDEF' member,
	 since it contains the string table.  */
      symdef_mapelt->info.size -= d;
    }
  else if (pos > old_strings_size)
    fatal ("Old archive's string size was %u too small.",
	   pos - old_strings_size);

  for (tail = map; tail != 0; tail = tail->next)
    if (tail->info.symdefs)
      {
	len = tail->info.nsymdefs;
	s = tail->info.symdefs;

	for (i = 0; i < len; i++)
	  {
	    strcpy (new_strings + pos, s[i].s.name);
	    s[i].s.stringoffset = pos;
	    pos += strlen (new_strings + pos) + 1;
	  }
      }
  if (pos != new_strings_size)
    fatal ("internal error: inconsistency in new_strings_size", 0);
}

/* Print error message and usage message, and exit.  */

void
usage (s1, s2)
     char *s1, *s2;
{
  error (s1, s2);
  fprintf (stderr, "\
Usage: ar d|m|p|q|r|t|x [abiclouv] [position-name] archive file...\n");
  exit (1);
}

/* Print error message and exit.  */

void
fatal (s1, s2)
     char *s1, *s2;
{
  error (s1, s2);
  exit (1);
}

/* Print error message.  `s1' is printf control string, the rest are args.  */

void
error (s1, s2, s3, s4, s5)
     char *s1, *s2, *s3, *s4, *s5;
{
  fprintf (stderr, "ar: ");
  fprintf (stderr, s1, s2, s3, s4, s5);
  fprintf (stderr, "\n");
}

void
error_with_file (string, mapelt)
     char *string;
     struct mapelt *mapelt;
{
  fprintf (stderr, "ar: ");
  fprintf (stderr, string);
  if (mapelt->info.offset != 0)
    {
      assert (mapelt->info.name.stored != NULL);
      fprintf (stderr, "%s(%s)", archive, mapelt->info.name.stored);
    }
  else
    {
      assert (mapelt->info.name.given != NULL);
      fprintf (stderr, "%s", mapelt->info.name.given);
    }
  fprintf (stderr, "\n");
}

void
perror_with_name (name)
     char *name;
{
  extern int errno, sys_nerr;
  extern char *sys_errlist[];
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "unknown error for %s";
  error (s, name);
}

void
pfatal_with_name (name)
     char *name;
{
  extern int errno, sys_nerr;
  extern char *sys_errlist[];
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "cannot open %s";
  fatal (s, name);
}

/* Return a newly-allocated string whose contents
   concatenate those of S1, S2, and S3.  */

char *
concat (s1, s2, s3)
     char *s1, *s2, *s3;
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  *(result + len1 + len2 + len3) = 0;

  return result;
}

/* Like malloc but get fatal error if memory is exhausted.  */

char *
xmalloc (size)
     unsigned int size;
{
  extern char *malloc ();
  char *result = malloc (size);
  if (result == 0)
    fatal ("virtual memory exhausted", 0);
  return result;
}

char *
xrealloc (ptr, size)
     char *ptr;
     unsigned int size;
{
  extern char *realloc ();
  char *result = realloc (ptr, size);
  if (result == 0)
    fatal ("virtual memory exhausted");
  return result;
}


/* Operations on member_names: */

/* Convert USER_NAME, possibly a path, into the name that goes into
   the archive header.  This involves stripping off leading path
   information and truncating the final name according to the desired
   rules.  The caller is responsible for freeing the returned string. */

char *
user_to_header(user_name)
     char *user_name;
{
  int namelen;
  struct ar_hdr dummy_hdr;
  char *result;
  char *tmp;
  char *file_name;		/* user name after removing initial path */

  /* Make a clean copy of the user name.  Bump the pointer past any
     leading path information.  */

  tmp = concat(user_name, "", "");

  user_name = rindex(tmp, '/');
  user_name = (user_name ? user_name + 1 : tmp);

  /* Save a copy of the file name, in case we need it for an error
     message later. */
  file_name = concat (user_name, "", "");

  /* Truncate the name to fit the ar header size. */
  namelen = strlen (user_name);
  if (namelen >= sizeof (dummy_hdr.ar_name))
    {
      if (gnu_truncation
	  && user_name[namelen - 2] == '.'
	  && user_name[namelen - 1] == 'o')
	{
	  user_name[sizeof (dummy_hdr.ar_name) - 3] = '.';
	  user_name[sizeof (dummy_hdr.ar_name) - 2] = 'o';
	}
      user_name[sizeof (dummy_hdr.ar_name) - 1] = '\0';
      error ("Using member name `%s' for filename `%s'", user_name, file_name);
    }

  /* Now make a fresh copy to return to the user. */
  result = concat (user_name, "", "");

  free (tmp);
  free (file_name);
  return result;
}

/* Return non-zero if the member names match.  As a side effect, 
   propogates user names.  This side-effect is important for, e.g.,
   extracting named members.  */ 

int
name_match(name1, name2)
     struct member_name *name1, *name2;
{
  if (strcmp(name1->stored, name2->stored) !=0)
    return 0;

  /* They match. */
  if (name1->given && !name2->given)
    name2->given = concat (name1->given, "", "");
  if (name2->given && !name1->given)
    name1->given = concat (name2->given, "", "");

  return 1;
}

/* Return an array of member names, from an argv-type array of user 
   names. */

struct member_name *
make_file_args (argvp, num_files)
     char **argvp;
     int num_files;			/* number of elements in argvp array */
{
  struct member_name *result, *name;

  result = (struct member_name *)xmalloc ((num_files + 1) *
					  sizeof (struct member_name));
  for (name = result; name < result + num_files; ++name, ++argvp)
    init_name (name, *argvp);

  name->given = NULL;
  name->stored = NULL;

  return result;
}

/* Initialize a member_name.  The caller is responsible for eventually
   freeing the allocated strings.  */

void
init_name (name_ptr, user_name)
     struct member_name *name_ptr;
     char *user_name;
{
  name_ptr->given = concat (user_name, "", "");
  name_ptr->stored = user_to_header (user_name);
}

void
free_name_strings (name_ptr)
     struct member_name *name_ptr;
{
  if (name_ptr->given)
    free (name_ptr->given);
  if (name_ptr->stored)
    free (name_ptr->stored);
  name_ptr->given = name_ptr->stored = NULL;
}

/* Methods for managing mapelt's. */

/* Initialize a map element: make sure the name is empty. */

void
init_elt (elt)
     struct mapelt *elt;
{
  elt->info.name.stored = elt->info.name.given = NULL;
}

/* Mark a map element for deletion by making the name empty.
   XXX - leaks some memory here. */

void
mark_as_deleted (elt)
     struct mapelt *elt;
{
  elt->info.name.stored = elt->info.name.given = NULL;
}

int
marked_for_deletion (elt)
     struct mapelt *elt;
{
  return Empty_Name(&elt->info.name);
}
