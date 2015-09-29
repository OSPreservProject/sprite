/* Remote target communications for serial-line targets in custom GDB protocol
   Copyright (C) 1988-1991 Free Software Foundation, Inc.

This file is part of GDB.

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

#include <stdio.h>
#include <sprite.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <errno.h>
#include "kernel/machTypes.h"

#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "value.h"
#include "expression.h"
#include "target.h"
#include "wait.h"
#include "terminal.h"
#include "sprite.h"
#include "kernel/sun4.md/vmSunConst.h"
#include "kernel/sun4.md/machConst.h"
#include "kernel/sun4.md/dbg.h"



#include <signal.h>

extern struct value *call_function_by_hand();
extern void start_remote ();

extern struct target_ops remote_ops;	/* Forward decl */

/*
 * Hostname of attached remote hosts.  Error otherwise.
 */

char *hostName;

static	int	initialized = 0; /* Set to true when remote connection is
				  * initialized. */
/*
 * Table mapping kernel exceptions into Unix signals.  
 */

struct sig_mapping_struct {
	char 	*sig_name;	/* Print string for signal. */
	int	 dbgSig; 	/* Boolean - A signal used by the debugger. */
	int 	unixSignal; 	/* Unix signal equalient. */
} sig_mapping[] =  {
/* 0 */ { "Reset Trap", 0, SIGQUIT },
/* 1 */ { "Instruction Fault", 0, SIGSEGV },
/* 2 */ { "Illegal Instruction Fault", 0, SIGILL },
/* 3 */ { "Privilege Instruction Fault", 0, SIGILL },
/* 4 */ { "FPU Disabled Fault", 0, SIGFPE },
/* 5 */ { "Window Overflow Fault", 0, SIGBUS },
/* 6 */ { "Window Underflow Fault", 0, SIGBUS },
/* 7 */ { "Memory Address Fault", 0, SIGSEGV },
/* 8 */ { "FPU Exception Fault", 0, SIGFPE },
/* 9 */ { "Data Fault",  0, SIGSEGV },
/* 10 */ { "Tag Overflow Trap", 0, SIGSEGV },
/* 11 */ { "Unknown Trap 11",  0, SIGSEGV },
/* 12 */ { "Unknown Trap 12",  0, SIGSEGV },
/* 13 */ { "Unknown Trap 13",  0, SIGSEGV },
/* 14 */ { "Unknown Trap 14",  0, SIGSEGV },
/* 15 */ { "Unknown Trap 15",  0, SIGSEGV },
/* 16 */ { "Interrupt Trap",  0, SIGINT },
/* 17 */ { "Level 1 Interrupt", 0, SIGINT },
/* 18 */ { "Level 2 Interrupt", 0, SIGINT },
/* 19 */ { "Level 3 Interrupt", 0, SIGINT },
/* 20 */ { "Level 4 Interrupt", 0, SIGINT },
/* 21 */ { "Level 5 Interrupt", 0, SIGINT },
/* 22 */ { "Level 6 Interrupt", 0, SIGINT },
/* 23 */ { "Level 7 Interrupt", 0, SIGINT },
/* 24 */ { "Level 8 Interrupt", 0, SIGINT },
/* 25 */ { "Level 9 Interrupt", 0, SIGINT },
/* 26 */ { "Level 10 Interrupt", 0, SIGINT },
/* 27 */ { "Level 11 Interrupt", 0, SIGINT },
/* 28 */ { "Level 12 Interrupt", 0, SIGINT },
/* 29 */ { "Level 13 Interrupt", 0, SIGINT },
/* 30 */ { "Level 14 Interrupt", 0, SIGINT },
/* 31 */ { "Level 15 Interrupt", 0, SIGINT },
/* 32 */ { "Breakpoint Trap",  1, SIGTRAP },
/* 33 */ { "Unknown Trap",  0, SIGSEGV },
/* 34 */ { "UNKNOWN EXCEPTION", 0, SIGSEGV },
};

#define	NUM_SIG_MAPS	(sizeof(sig_mapping)/sizeof(sig_mapping[0]))



#define	PBUFSIZ	1024

/* Maximum number of bytes to read/write at once.  The value here
   is chosen to fill up a packet (the headers account for the 32).  */
#define MAXBUFBYTES ((PBUFSIZ-32)/2)



/* Initialize remote connection */

void
remote_start()
{
}

/* Clean up connection to a remote debugger.  */

/* ARGSUSED */
void
remote_close (quitting)
     int quitting;
{
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

void
remote_open (name, from_tty)
     char *name;
     int from_tty;
{
  TERMINAL sg;

  if (name == 0)
    error (
"To open a remote debug connection, you need to specify what host\n\
machine is attached to the remote system (e.g. allspice).");

  hostName = savestring(name,strlen(name));
  target_preopen (from_tty);

  remote_close (0);

  if (from_tty)
    printf ("Remote debugging using %s\n", name);
  push_target (&remote_ops);	/* Switch to using remote target now */

  start_remote ();		/* Initialize gdb process mechanisms */
}

/* remote_detach()
   takes a program previously attached to and detaches it.
   We better not have left any breakpoints
   in the program or it'll die when it hits one.
   Close the open connection to the remote debugger.
   Use this when you want to detach and do something else
   with your gdb.  */

static void
remote_detach (args, from_tty)
     char *args;
     int from_tty;
{
  int cur_pc;
  cur_pc = read_pc();
  if (args) 
      Kdbx_Trace(DBG_DETACH, &cur_pc, 0, sizeof(int));
  remote_clean_up();
  pop_target ();
  if (from_tty)
    printf ("Ending remote debugging.\n");
}

/* Tell the remote machine to resume.  */

void
remote_resume (step, siggnal)
     int step, siggnal;
{
  char buf[PBUFSIZ];

  if (siggnal)
    error ("Can't send signals to a remote system.");
  ERROR_NO_ATTACHED_HOST;
  step_addr = -2;
  if (step) {
         static char break_insn[] = BREAKPOINT;
         step_addr = read_register(NPC_REGNUM);
         if ((remote_read_bytes(step_addr,step_addr_contents,8) != 0) ||
		 (remote_write_bytes(step_addr,break_insn, 4) != 0) ||
		 (remote_write_bytes(step_addr+4,break_insn, 4) != 0)) {
	     error("Can't set single step breakpoint at 0x%x\n", step_addr);
	  }

  }
  if (Kdbx_Trace( DBG_CONTINUE, 0, 0, sizeof(int)) < 0) {
        error("error trying to continue process\n");
  }
}

/* Wait until the remote machine stops, then return,
   storing status in STATUS just as `wait' would.
   Returns "pid" (though it's not clear what, if anything, that
   means in the case of this target).  */

int
remote_wait (status)
     WAITTYPE *status;
{
  unsigned char buf[PBUFSIZ];

  WSETEXIT ((*status), 0);
  getpkt (buf);
  if (buf[0] == 'E')
    error ("Remote failure reply: %s", buf);
  if (buf[0] != 'S')
    error ("Invalid remote reply: %s", buf);
  WSETSTOP ((*status), (((fromhex (buf[1])) << 4) + (fromhex (buf[2]))));
  return 0;
}

/* Read the remote registers into the block REGS.  */

/* Currently we just read all the registers, so we don't use regno.  */
/* ARGSUSED */
void
remote_fetch_registers (regno)
     int regno;
{
  char buf[PBUFSIZ];
  int i;
  char *p;
  char regs[REGISTER_BYTES];

  sprintf (buf, "g");
  remote_send (buf);

  /* Reply describes registers byte by byte, each byte encoded as two
     hex characters.  Suck them all up, then supply them to the
     register cacheing/storage mechanism.  */

  p = buf;
  for (i = 0; i < REGISTER_BYTES; i++)
    {
      if (p[0] == 0 || p[1] == 0)
	error ("Remote reply is too short: %s", buf);
      regs[i] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }
  for (i = 0; i < NUM_REGS; i++)
    supply_register (i, &regs[REGISTER_BYTE(i)]);
}

/* Prepare to store registers.  Since we send them all, we have to
   read out the ones we don't want to change first.  */

void 
remote_prepare_to_store ()
{
  remote_fetch_registers (-1);
}

/* Store the remote registers from the contents of the block REGISTERS. 
   FIXME, eventually just store one register if that's all that is needed.  */

/* ARGSUSED */
int
remote_store_registers (regno)
     int regno;
{
  char buf[PBUFSIZ];
  int i;
  char *p;

  buf[0] = 'G';
  
  /* Command describes registers byte by byte,
     each byte encoded as two hex characters.  */

  p = buf + 1;
  for (i = 0; i < REGISTER_BYTES; i++)
    {
      *p++ = tohex ((registers[i] >> 4) & 0xf);
      *p++ = tohex (registers[i] & 0xf);
    }
  *p = '\0';

  remote_send (buf);
  return 0;
}

#if 0
/* Read a word from remote address ADDR and return it.
   This goes through the data cache.  */

int
remote_fetch_word (addr)
     CORE_ADDR addr;
{
  if (icache)
    {
      extern CORE_ADDR text_start, text_end;

      if (addr >= text_start && addr < text_end)
	{
	  int buffer;
	  xfer_core_file (addr, &buffer, sizeof (int));
	  return buffer;
	}
    }
  return dcache_fetch (addr);
}

/* Write a word WORD into remote address ADDR.
   This goes through the data cache.  */

void
remote_store_word (addr, word)
     CORE_ADDR addr;
     int word;
{
  dcache_poke (addr, word);
}
#endif /* 0 */

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.  */

void
remote_write_bytes (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  char buf[PBUFSIZ];
  int i;
  char *p;

  if (len > PBUFSIZ / 2 - 20)
    abort ();

  sprintf (buf, "M%x,%x:", memaddr, len);

  /* We send target system values byte by byte, in increasing byte addresses,
     each byte encoded as two hex characters.  */

  p = buf + strlen (buf);
  for (i = 0; i < len; i++)
    {
      *p++ = tohex ((myaddr[i] >> 4) & 0xf);
      *p++ = tohex (myaddr[i] & 0xf);
    }
  *p = '\0';

  remote_send (buf);
}

/* Read memory data directly from the remote machine.
   This does not use the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.  */

void
remote_read_bytes (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  char buf[PBUFSIZ];
  int i;
  char *p;

  if (len > PBUFSIZ / 2 - 1)
    abort ();

  sprintf (buf, "m%x,%x", memaddr, len);
  remote_send (buf);

  /* Reply describes registers byte by byte,
     each byte encoded as two hex characters.  */

  p = buf;
  for (i = 0; i < len; i++)
    {
      if (p[0] == 0 || p[1] == 0)
	error ("Remote reply is too short: %s", buf);
      myaddr[i] = fromhex (p[0]) * 16 + fromhex (p[1]);
      p += 2;
    }
}

/* Read or write LEN bytes from inferior memory at MEMADDR, transferring
   to or from debugger address MYADDR.  Write to inferior if SHOULD_WRITE is
   nonzero.  Returns length of data written or read; 0 for error.  */

/* ARGSUSED */
int
remote_xfer_memory(memaddr, myaddr, len, should_write, target)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int should_write;
     struct target_ops *target;			/* ignored */
{
  int origlen = len;
  int xfersize;
  while (len > 0)
    {
      if (len > MAXBUFBYTES)
	xfersize = MAXBUFBYTES;
      else
	xfersize = len;

      if (should_write)
        remote_write_bytes(memaddr, myaddr, xfersize);
      else
	remote_read_bytes (memaddr, myaddr, xfersize);
      memaddr += xfersize;
      myaddr  += xfersize;
      len     -= xfersize;
    }
  return origlen; /* no error possible */
}

void
remote_files_info ()
{
  printf ("remote files info missing here.  FIXME.\n");
}

/*

A debug packet whose contents are <data>
is encapsulated for transmission in the form:

	$ <data> # CSUM1 CSUM2

	<data> must be ASCII alphanumeric and cannot include characters
	'$' or '#'

	CSUM1 and CSUM2 are ascii hex representation of an 8-bit 
	checksum of <data>, the most significant nibble is sent first.
	the hex digits 0-9,a-f are used.

Receiver responds with:

	+	- if CSUM is correct and ready for next packet
	-	- if CSUM is incorrect

*/

static int
readchar ()
{
  char buf;

  buf = '\0';
#ifdef HAVE_TERMIO
  /* termio does the timeout for us.  */
  read (remote_desc, &buf, 1);
#else
  alarm (timeout);
  read (remote_desc, &buf, 1);
  alarm (0);
#endif

  return buf & 0x7f;
}

/* Send the command in BUF to the remote machine,
   and read the reply into BUF.
   Report an error if we get an error reply.  */

static void
remote_send (buf)
     char *buf;
{

  putpkt (buf);
  getpkt (buf);

  if (buf[0] == 'E')
    error ("Remote failure reply: %s", buf);
}

/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF.  */

static void
putpkt (buf)
     char *buf;
{
  int i;
  unsigned char csum = 0;
  char buf2[500];
  int cnt = strlen (buf);
  char ch;
  char *p;

  /* Copy the packet into buffer BUF2, encapsulating it
     and giving it a checksum.  */

  p = buf2;
  *p++ = '$';

  for (i = 0; i < cnt; i++)
    {
      csum += buf[i];
      *p++ = buf[i];
    }
  *p++ = '#';
  *p++ = tohex ((csum >> 4) & 0xf);
  *p++ = tohex (csum & 0xf);

  /* Send it over and over until we get a positive ack.  */

  do {
    if (kiodebug)
      {
	*p = '\0';
	printf ("Sending packet: %s (%s)\n", buf2, buf);
      }
    write (remote_desc, buf2, p - buf2);

    /* read until either a timeout occurs (\0) or '+' is read */
    do {
      ch = readchar ();
    } while ((ch != '+') && (ch != '\0'));
  } while (ch != '+');
}

/* Read a packet from the remote machine, with error checking,
   and store it in BUF.  */

static void
getpkt (buf)
     char *buf;
{
  char *bp;
  unsigned char csum;
  int c;
  unsigned char c1, c2;

#if 0
  /* Sorry, this will cause all hell to break loose, i.e. we'll end
     up in the command loop with an inferior, but (at least if this
     happens in remote_wait or some such place) without a current_frame,
     having set up prev_* in wait_for_inferior, etc.

     If it is necessary to have such an "emergency exit", seems like
     the only plausible thing to do is to say the inferior died, and
     make the user reattach if they want to.  Perhaps with a prompt
     asking for confirmation.  */

  /* allow immediate quit while reading from device, it could be hung */
  immediate_quit++;
#endif /* 0 */

  while (1)
    {
      /* Force csum to be zero here because of possible error retry.  */
      csum = 0;
      
      while ((c = readchar()) != '$');

      bp = buf;
      while (1)
	{
	  c = readchar ();
	  if (c == '#')
	    break;
	  *bp++ = c;
	  csum += c;
	}
      *bp = 0;

      c1 = fromhex (readchar ());
      c2 = fromhex (readchar ());
      if ((csum & 0xff) == (c1 << 4) + c2)
	break;
      printf ("Bad checksum, sentsum=0x%x, csum=0x%x, buf=%s\n",
	      (c1 << 4) + c2, csum & 0xff, buf);
      write (remote_desc, "-", 1);
    }

#if 0
  immediate_quit--;
#endif

  write (remote_desc, "+", 1);

  if (kiodebug)
    fprintf (stderr,"Packet received :%s\n", buf);
}

/* The data cache leads to incorrect results because it doesn't know about
   volatile variables, thus making it impossible to debug functions which
   use hardware registers.  Therefore it is #if 0'd out.  Effect on
   performance is some, for backtraces of functions with a few
   arguments each.  For functions with many arguments, the stack
   frames don't fit in the cache blocks, which makes the cache less
   helpful.  Disabling the cache is a big performance win for fetching
   large structures, because the cache code fetched data in 16-byte
   chunks.  */
#if 0
/* The data cache records all the data read from the remote machine
   since the last time it stopped.

   Each cache block holds 16 bytes of data
   starting at a multiple-of-16 address.  */

#define DCACHE_SIZE 64		/* Number of cache blocks */

struct dcache_block {
	struct dcache_block *next, *last;
	unsigned int addr;	/* Address for which data is recorded.  */
	int data[4];
};

struct dcache_block dcache_free, dcache_valid;

/* Free all the data cache blocks, thus discarding all cached data.  */ 

static void
dcache_flush ()
{
  register struct dcache_block *db;

  while ((db = dcache_valid.next) != &dcache_valid)
    {
      remque (db);
      insque (db, &dcache_free);
    }
}

/*
 * If addr is present in the dcache, return the address of the block 
 * containing it.
 */

struct dcache_block *
dcache_hit (addr)
{
  register struct dcache_block *db;

  if (addr & 3)
    abort ();

  /* Search all cache blocks for one that is at this address.  */
  db = dcache_valid.next;
  while (db != &dcache_valid)
    {
      if ((addr & 0xfffffff0) == db->addr)
	return db;
      db = db->next;
    }
  return NULL;
}

/*  Return the int data at address ADDR in dcache block DC.  */

int
dcache_value (db, addr)
     struct dcache_block *db;
     unsigned int addr;
{
  if (addr & 3)
    abort ();
  return (db->data[(addr>>2)&3]);
}

/* Get a free cache block, put it on the valid list,
   and return its address.  The caller should store into the block
   the address and data that it describes.  */

struct dcache_block *
dcache_alloc ()
{
  register struct dcache_block *db;

  if ((db = dcache_free.next) == &dcache_free)
    /* If we can't get one from the free list, take last valid */
    db = dcache_valid.last;

  remque (db);
  insque (db, &dcache_valid);
  return (db);
}

/* Return the contents of the word at address ADDR in the remote machine,
   using the data cache.  */

int
dcache_fetch (addr)
     CORE_ADDR addr;
{
  register struct dcache_block *db;

  db = dcache_hit (addr);
  if (db == 0)
    {
      db = dcache_alloc ();
      remote_read_bytes (addr & ~0xf, db->data, 16);
      db->addr = addr & ~0xf;
    }
  return (dcache_value (db, addr));
}

/* Write the word at ADDR both in the data cache and in the remote machine.  */

dcache_poke (addr, data)
     CORE_ADDR addr;
     int data;
{
  register struct dcache_block *db;

  /* First make sure the word is IN the cache.  DB is its cache block.  */
  db = dcache_hit (addr);
  if (db == 0)
    {
      db = dcache_alloc ();
      remote_read_bytes (addr & ~0xf, db->data, 16);
      db->addr = addr & ~0xf;
    }

  /* Modify the word in the cache.  */
  db->data[(addr>>2)&3] = data;

  /* Send the changed word.  */
  remote_write_bytes (addr, &data, 4);
}

/* Initialize the data cache.  */

dcache_init ()
{
  register i;
  register struct dcache_block *db;

  db = (struct dcache_block *) xmalloc (sizeof (struct dcache_block) * 
					DCACHE_SIZE);
  dcache_free.next = dcache_free.last = &dcache_free;
  dcache_valid.next = dcache_valid.last = &dcache_valid;
  for (i=0;i<DCACHE_SIZE;i++,db++)
    insque (db, &dcache_free);
}
#endif /* 0 */

/* Define the target subroutine names */

struct target_ops remote_ops = {
	"remote", "Remote serial target in gdb-specific protocol",
	"Use a remote computer via a serial line, using a gdb-specific protocol.\n\
Specify the serial device it is connected to (e.g. /dev/ttya).",
	remote_open, remote_close,
	0, remote_detach, remote_resume, remote_wait,  /* attach */
	remote_fetch_registers, remote_store_registers,
	remote_prepare_to_store, 0, 0, /* conv_from, conv_to */
	remote_xfer_memory, remote_files_info,
	0, 0, /* insert_breakpoint, remove_breakpoint, */
	0, 0, 0, 0, 0,	/* Terminal crud */
	0, /* kill */
	0,  /* load */
	call_function_by_hand,
	0, /* lookup_symbol */
	0, 0, /* create_inferior FIXME, mourn_inferior FIXME */
	process_stratum, 0, /* next */
	1, 1, 1, 1, 1,	/* all mem, mem, stack, regs, exec */
	0, 0,			/* Section pointers */
	OPS_MAGIC,		/* Always the last thing */
};

void
_initialize_remote ()
{
  add_target (&remote_ops);
}
