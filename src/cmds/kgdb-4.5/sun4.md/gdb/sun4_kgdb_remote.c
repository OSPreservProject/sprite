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
#include <errno.h>
#include "kernel/machTypes.h"

#include "defs.h"
/*#include "param.h"*/
#include "frame.h"
#include "inferior.h"
#include "value.h"
#include "expression.h"
#include "wait.h"
#include <sys/wait.h>
#include "terminal.h"
#include "sprite.h"
#include "kernel/sun4.md/vmSunConst.h"
#include "kernel/sun4.md/machConst.h"
#include "kernel/sun4.md/dbg.h"
#include <signal.h>
#include "target.h"

extern bfd *core_bfd;

/*
 * Hostname of attached remote hosts.  Error otherwise.
 */

char *hostName;

/*
 * Useful macros
 * ERROR_NO_ATTACHED_HOST - Remote an error and abort if no host is
 *			    currently attached.
 * MARK_DISCONNECTED - Mark a host a disconnected and free up state.
 */

#define ERROR_NO_ATTACHED_HOST \
	if (!hostName) error("No machine attached.");

#define MARK_DISCONNECTED  {	 \
	initialized = 0;     	 \
	inferior_pid = 0;   	 \
	hostName = (char *) 0;	 \
	free(dataCache); 	 \
	free(cacheInfo); 	 \
	}

extern struct value *call_function_by_hand();
extern void start_remote ();

int remote_debugging = 0;
static int kiodebug = 0;
static int timeout = 5;

int remote_desc = -1;
int step_addr;
int step_addr_contents[2];

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

/* Convert hex digit A to a number.  */

static int
fromhex (a)
     int a;
{
  if (a >= '0' && a <= '9')
    return a - '0';
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    error ("Reply contains invalid hex digit");
  return -1;
}

/* Convert number NIB to a hex digit.  */

static int
tohex (nib)
     int nib;
{
  if (nib < 10)
    return '0'+nib;
  else
    return 'a'+nib-10;
}

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
/*  target_preopen (from_tty);

  remote_close (0);*/

  if (from_tty)
    printf ("Remote debugging using %s\n", name);
  push_target (&remote_ops);	/* Switch to using remote target now */
  remote_debugging = 1;
  start_remote ();		/* Initialize gdb process mechanisms */
}

char *
remote_version()
{
  static char version[1024];

  ERROR_NO_ATTACHED_HOST;
  Kdbx_Trace(DBG_GET_VERSION_STRING, 0, version, 1024);
  return version;
}

int lastPid = -1;
StopInfo remoteStopInfo;    /* Current Dbg_StopInfo from core file. */

int
remote_attach(pid)
    int	pid;
{
    int	status;
    struct expression *expr;
    register struct cleanup *old_chain;
    register value val;
    int	machRegStateAddr;
    Proc_ControlBlock	*procPtr;
    Mach_RegState machRegState;
    char	exp[128];

    ERROR_NO_ATTACHED_HOST;
    if (pid != lastPid) {
      Kdbx_Trace(DBG_SET_PID, &pid, 0,sizeof(int));
      lastPid = pid;
      /* Now, set up the frame cache, and print the top of stack */
      set_current_frame (create_new_frame (read_register (FP_REGNUM),
					   read_pc ()));
      select_frame (get_current_frame (), 0);
      print_stack_frame (selected_frame, selected_frame_level, 1);
    }
    start_remote();
    return 1;
}

/* remote_detach()
   takes a program previously attached to and detaches it.
   We better not have left any breakpoints
   in the program or it'll die when it hits one.
   Close the open connection to the remote debugger.
   Use this when you want to detach and do something else
   with your gdb.  */

void
remote_detach (args, from_tty)
     char *args;
     int from_tty;
{
  int cur_pc;

  ERROR_NO_ATTACHED_HOST;
  cur_pc = read_pc();
  if (args) 
      Kdbx_Trace(DBG_DETACH, &cur_pc, 0, sizeof(int));
  remote_clean_up();
  pop_target ();
  remote_debugging = 0;
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
#ifdef MYDEBUG
	 int a, b, c;
	 a = remote_read_bytes(step_addr, step_addr_contents, 8);
	 b = remote_write_bytes(step_addr, break_insn, 4);
	 c = remote_write_bytes(step_addr+4, break_insn, 4);
	 printf("%x: %d, %d, %d, %s\n", step_addr, a, b, c, break_insn);
#endif
         step_addr = read_register(NPC_REGNUM);
         if ((remote_read_bytes(step_addr, step_addr_contents, 8) != 0) ||
	     (remote_write_bytes(step_addr,break_insn, 4) != 0) ||
	     (remote_write_bytes(step_addr+4,break_insn, 4) != 0)) {
	   error("Can't set single step breakpoint at 0x%x\n", step_addr);
	 }
	 
  }
  if (Kdbx_Trace( DBG_CONTINUE, 0, 0, sizeof(int)) < 0) {
        error("error trying to continue process\n");
  }
}

/*
 * The following variables are sued when using the core memory interface
 * rather than active ethernet debugging.
 */
static int remoteCoreChan = -1;	/* Open file descriptor of core file. -1 
				 * means no file open. */
static char *remoteCoreFile;    /* Malloced name of core file. */
static int remoteOffset;	/* Offset used to convert addresses to 
				 * offsets into core file. */
static Dbg_DumpBounds remoteBounds;  /* Dump bounds from core file. */

/*
 * IN_CORE_FILE - Returns TRUE if address is in the corefile.
 */
#define	IN_CORE_FILE(addr) ((addr) >= (CORE_ADDR) remoteBounds.kernelCodeStart\
	  && ((addr) < (CORE_ADDR) remoteBounds.fileCacheStart + \
				          remoteBounds.fileCacheSize))


/* Wait until the remote machine stops, then return,
   storing status in STATUS just as `wait' would.
   Returns "pid" (though it's not clear what, if anything, that
   means in the case of this target).  */

int
remote_wait (status)
     union wait *status;
{
#ifdef sprite
  StopInfo    stopInfo;
  int		trap;
  int	text_size;
  extern CORE_ADDR text_start, text_end;
  if (remoteCoreChan >= 0) {
    /*
     * Debugging using core file, just set text_{start, end} and
     * returned stopped signal.
     */
    status->w_status = 0;
    status->w_stopval = WSTOPPED;
    trap = DBG_CVT_MACH_TRAP(remoteStopInfo.trapType);
    status->w_stopsig = sig_mapping[trap].unixSignal;
    if ((lastPid == -1) && !sig_mapping[trap].dbgSig) { 
      printf("Kernel returns with signal (%d) %s\n",
	     remoteStopInfo.trapType,
	     sig_mapping[trap].sig_name);
    }
    text_start = remoteBounds.kernelCodeStart;
    text_end = remoteBounds.kernelCodeStart + remoteBounds.kernelCodeSize;
    return status->w_stopsig;
  }
  ERROR_NO_ATTACHED_HOST;
  
  if (Kdbx_Trace(DBG_GET_STOP_INFO, (char *) 0, (char *)&stopInfo,
		 sizeof(stopInfo)) != 0) {
    error("Can't get stop info\n");
  }
  if (stopInfo.regs.pc == step_addr ||
      stopInfo.regs.pc == step_addr+4) {
    if (remote_write_bytes(step_addr,step_addr_contents, 8) != 0) 
      error("Can't restore single step address\n");
  }
  status->w_status = 0;
  status->w_stopval = WSTOPPED;
  
  trap = DBG_CVT_MACH_TRAP(stopInfo.trapType);
  status->w_stopsig = sig_mapping[trap].unixSignal;
  if ((lastPid != -1) && !sig_mapping[trap].dbgSig) { 
    printf("Kernel returns with signal (%d) %s\n",stopInfo.trapType,
	   sig_mapping[trap].sig_name);
  }
  text_size = text_end - text_start;
  text_size &= ~(8*1024-1);
  text_start = stopInfo.codeStart - 8*1024;
  text_end = text_start+text_size;
  return status->w_stopsig;
#else
  unsigned char buf[PBUFSIZ];
  
  WSETEXIT ((*status), 0);
  getpkt (buf);
  if (buf[0] == 'E')
    error ("Remote failure reply: %s", buf);
  if (buf[0] != 'S')
    error ("Invalid remote reply: %s", buf);
  WSETSTOP ((*status), (((fromhex (buf[1])) << 4) + (fromhex (buf[2]))));
  return 0;
#endif
}

/* Read the remote registers into the block REGS.  */

/* Currently we just read all the registers, so we don't use regno.  */
/* ARGSUSED */
#define FIRST_LOCAL_REGNUM	16
extern char *regs;
void
remote_fetch_registers (regno)
     int regno;
{
  StopInfo    stopInfo;
  char regs[REGISTER_BYTES];
  int i;
  if (remoteCoreChan >= 0) {

      bcopy(remoteStopInfo.regs.globals,regs,4*8);
      bcopy(remoteStopInfo.regs.ins,regs+4*8,4*8);
      ((int *)regs)[Y_REGNUM] = remoteStopInfo.regs.y; 
      ((int *)regs)[PS_REGNUM] = remoteStopInfo.regs.curPsr; 
      ((int *)regs)[PC_REGNUM] = remoteStopInfo.regs.pc; 
      ((int *)regs)[NPC_REGNUM] = remoteStopInfo.regs.nextPc; 
      remote_read_bytes(remoteStopInfo.regs.ins[6], 
		    ((int *) regs) + FIRST_LOCAL_REGNUM,4*16);
      return;
  }
  ERROR_NO_ATTACHED_HOST;
  Kdbx_Trace(DBG_GET_STOP_INFO, (char *) 0, (char *)&stopInfo,
               sizeof(stopInfo));
  bcopy(stopInfo.regs.globals,regs,4*8);
  bcopy(stopInfo.regs.ins,regs+4*8,4*8);
  ((int *)regs)[Y_REGNUM] = stopInfo.regs.y; 
  ((int *)regs)[PS_REGNUM] = stopInfo.regs.curPsr; 
  ((int *)regs)[PC_REGNUM] = stopInfo.regs.pc; 
  ((int *)regs)[NPC_REGNUM] = stopInfo.regs.nextPc; 
  remote_read_bytes(stopInfo.regs.ins[6], 
		((int *) regs) + FIRST_LOCAL_REGNUM,4*16);
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

int
remote_write_bytes (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  extern CORE_ADDR text_start, text_end;
  ERROR_NO_ATTACHED_HOST;
  if (memaddr >= text_start && memaddr < text_end)
	{
	  return Kdbx_Trace(DBG_INST_WRITE, myaddr, memaddr, len);
	}
  return Kdbx_Trace(DBG_INST_WRITE,  myaddr, memaddr, len);
}

/* Read memory data directly from the remote machine.
   This does not use the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.  */

int
remote_read_bytes (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  int err;

  if (remoteCoreChan >= 0) {
      if (IN_CORE_FILE(memaddr)) {
	  if (lseek(remoteCoreChan, memaddr - remoteOffset, L_SET) < 0) {
	      err = errno;
	      perror("lseek corefile");
	      return err;
	   }
	  if (myread(remoteCoreChan,myaddr, len) != len) {
	      err = errno;
	      perror("read corefile");
	      return (err > 0) ? err : 1;
	  }
    } else {
	  /* fprintf(stderr, "Address 0x%x out of range\n", memaddr); */
	  return EIO;
    }
    return 0;
  }
  ERROR_NO_ATTACHED_HOST;
  return Kdbx_Trace(DBG_DATA_READ, memaddr, myaddr, len);
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

  printf("We're doing putpkt...\n");
  fflush(stdout);
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

  printf("We're doing getpkt...\n");
  fflush(stdout);
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

void
remote_reboot (args)
     char *args;
{
  char *rebootString = (char *)xmalloc(strlen(args)+ 9);
  ERROR_NO_ATTACHED_HOST;
  if (!args)
	args = "";

/*  Kdbx_Trace(DBG_REBOOT, args, NULL, strlen(args));*/
  remote_detach(args, 0);
  sprintf(rebootString, "kmsg -r %s", args);
  system(rebootString);
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
    remote_open, 
    remote_close,
    remote_attach,
    remote_detach, 
    remote_resume, 
    remote_wait,  /* attach */
    remote_fetch_registers, 
    remote_store_registers,
    remote_prepare_to_store, 
    0, 0, /* conv_from, conv_to */
    remote_xfer_memory, 
    remote_files_info,
    0, 0, /* insert_breakpoint, remove_breakpoint, */
    0, 0, 0, 0, 0,	/* Terminal crud */
    remote_detach, /* kill */
    0,  /* load */
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


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sgtty.h>

/*
 * Direct mapped cache of data and code.   The cache is flushed after every
 * step and continue by incrementing the version number.  Flushing code
 * isn't necessary but since kdbx already has an internal code cache it
 * doesn't hurt and makes life easier.
 */
static	int	cacheBlockSize = -1;
static	int	cacheBlockShift = -1;
static	int	cacheSize = 128 * 1024;
#define	CACHE_BLOCK_MASK 	(cacheSize / cacheBlockSize - 1)
#define	CACHE_BLOCK_OFFSET_MASK	(cacheBlockSize - 1)
#define	NUM_CACHE_BLOCKS	(cacheSize >> cacheBlockShift)
#define	CACHE_OFFSET_MASK	(cacheSize - 1)
#define	GET_CACHE_BLOCK(address) (((unsigned int) address) >> cacheBlockShift)
/*
 * The data cache is just an array of characters.
 */
static	char	*dataCache;
/*
 * There is information about kept about each cache block.
 */
typedef	struct {
    int		version;	/* Version number of this cache block. */
    char	*realAddr;	/* Actual address of data stored in the block.*/
} CacheInfo;
static CacheInfo	*cacheInfo;
static int	currentVersion = 1;

/*
 * Stuff for the serial port.
 */
static	int	kernChannel = 0;


/*
 * Message buffers.
 */
static Dbg_Msg	msg;
static int	msgSize;
#define	REPLY_BUFFER_SIZE	16384
static	char	replyBuffer[REPLY_BUFFER_SIZE];
static	char	requestBuffer[DBG_MAX_REQUEST_SIZE];
static	int	msgNum = 0;

static void	RecvReply();

static	struct sockaddr_in	remote;
static	int			kdbxTimeout = 1;
static	int			netSocket;


/*
 *----------------------------------------------------------------------
 *
 * CreateSocket --
 *
 *	Creates a UDP socket connected to the Sprite host's kernel 
 *	debugger port.
 *
 * Results:
 *	The stream ID of the socket.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
CreateSocket(spriteHostName)
    char	*spriteHostName;
{
    int			socketID;
    struct hostent 	*hostPtr;

    hostPtr = gethostbyname(spriteHostName);
    if (hostPtr == (struct hostent *) NULL) {
	error("CreateSocket: unknown host %s\n", spriteHostName);
    }
    if (hostPtr->h_addrtype != AF_INET) {
	error("CreateSocket: bad address type for host %s\n", 
		spriteHostName);
    }

    socketID = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketID < 0) {
	perror_with_name("CreateSocket: socket");
    }

    bzero((Address)&remote, sizeof(remote));
    bcopy(hostPtr->h_addr, (Address)&remote.sin_addr, hostPtr->h_length);
    remote.sin_port = htons(DBG_UDP_PORT);
    remote.sin_family = AF_INET;

    if (connect(socketID, (struct sockaddr *) &remote, sizeof(remote)) < 0) {
	perror_with_name("CreateSocket: connect");
    }

    return(socketID);
}


/*
 * ----------------------------------------------------------------------------
 *
 *  StartDebugger --
 *
 *     Start off a new conversation with the debugger.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Setup r network socket.
 * ----------------------------------------------------------------------------
 */
static void
StartDebugger()
{
        char	*host = hostName;
	hostName = (char *) 0;
	netSocket = CreateSocket(host);
	hostName = host;
}


/*
 * ----------------------------------------------------------------------------
 *
 *  SendRequest --
 *
 *     Send a request message to the kernel.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 * ----------------------------------------------------------------------------
 */
static void
SendRequest(numBytes, newRequest)
    int		numBytes;
    Boolean	newRequest;
{
     {
	Dbg_Opcode	opcode;

	msgSize = numBytes;
	if (newRequest) {
	    msgNum++;
	}
	*(int *)requestBuffer = msgNum;
#ifdef MYDEBUG
	printf("opcode: %d, write: %x, read: %x, pc: %d\n", msg.opcode, msg.data.writeMem.address, msg.data.readMem.address, msg.data.pc);
#endif
	bcopy(&msg, requestBuffer + 4, numBytes);
	if (write(netSocket, requestBuffer, numBytes + 4) < numBytes + 4) {
	     MARK_DISCONNECTED;
	    perror_with_name("SendRequest: Couldn't write to the kernel socket\n");
	    return;
	}
	if (newRequest) {
	    opcode = (Dbg_Opcode) msg.opcode;
	    if (opcode == DBG_DETACH || opcode == DBG_CONTINUE ||
		opcode == DBG_SINGLESTEP || opcode == DBG_DIVERT_SYSLOG || 
		opcode == DBG_BEGIN_CALL || 
		opcode == DBG_WRITE_REG || opcode == DBG_SET_PID) {
		int	dummy;
		/*
		 * Wait for explicit acknowledgments of these packets.
		 */
		RecvReply(opcode, 4, &dummy, NULL, 1);
	    }
	}
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *  RecvReply --
 *
 *     Receive a reply from the kernel.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 * ----------------------------------------------------------------------------
 */
static void
RecvReply(opcode, numBytes, destAddr, readStatusPtr, timeout)
    Dbg_Opcode	opcode;
    int		numBytes;
    char	*destAddr;
    int	*readStatusPtr;
    int	timeout;
{
    int		status;
    int 	resendRequest = 0;

    if (numBytes + 8 > REPLY_BUFFER_SIZE) {
	fprintf(stderr,"numBytes <%d> > REPLY_BUFFER_SIZE <%d>\n",
		    numBytes + 8, REPLY_BUFFER_SIZE);
	abort();
    }
     {
	int		readMask;
	struct	timeval	interval;
	int		bytesRead;

	interval.tv_sec = kdbxTimeout;
	interval.tv_usec = 0;
	do {
	    if (timeout) {
		int	numTimeouts;

		numTimeouts = 0;
		/*
		 * Loop timing out and sending packets until a new packet
		 * has arrived.
		 */
		do {
		    if (!resendRequest) { 
			readMask = 1 << netSocket;
			status = select(32, &readMask, NULL, NULL, &interval);
		    } else {
			status = 0;
			resendRequest = 0;
		    }
		    if (status == 1) {
			break;
		    } else if (status == -1) {
		        MARK_DISCONNECTED;
			perror_with_name("RecvReply: Couldn't select on socket.\n");
		    } else if (status == 0) {
			SendRequest(msgSize, 0);
			numTimeouts++;
			if (numTimeouts % 10 == 0) {
			    fprintf(stderr, 
				    "Timing out and resending to host %s\n",
				    hostName);
			    fflush(stderr);
			    QUIT;
			}
		    }
		} while (1);
	    }
	    if (opcode == DBG_DATA_READ || opcode == DBG_INST_READ ||
		opcode == DBG_GET_VERSION_STRING) {
		/*
		 * Data and instruction reads return variable size packets.
		 * The first two ints are message number and status.  If
		 * the status is OK then the data follows.
		 */
		immediate_quit++;
		bytesRead = read(netSocket, replyBuffer, numBytes + 8);
		immediate_quit--;
		if (bytesRead < 0) {
		    MARK_DISCONNECTED;
		    perror_with_name("RecvReply: Error reading socket.");
		}
		/*
		 * Check message number before the size because this could
		 * be an old packet.
		 */
		if (*(int *)replyBuffer != msgNum) {
		    printf("RecvReply: Old message number = %d, expecting %d\n",
			    *(int *)replyBuffer, msgNum);
		    fflush(stdout);
		    resendRequest = 1;
		    continue;
		}
		if (bytesRead == 8) {
		    /*
		     * Only 8 bytes so the read failed and there is no data.
		     */
		    *readStatusPtr = 0;
		    return;
		}
	        if (opcode == DBG_GET_VERSION_STRING) {
		     strncpy(destAddr, (char *)(replyBuffer + 4),numBytes);
		     return;
		}
		if (bytesRead != numBytes + 8) {
		    printf("RecvReply: Short read (1): op=%d exp=%d read=%d",
			    opcode, numBytes + 4, bytesRead);
		    continue;
		}
		*readStatusPtr = 1;
		bcopy(replyBuffer + 8, destAddr, numBytes);
		return;
	    } else if (opcode == DBG_END_CALL) {
		int	length;
		/*
		 * End call returns a variable size packet that contains
		 * the result of the call. The format of the message is 
		 * message number, length, data.
		 */
		immediate_quit++;
		bytesRead = read(netSocket, replyBuffer, REPLY_BUFFER_SIZE);
		immediate_quit--;
		if (bytesRead < 0) {
		    MARK_DISCONNECTED;
		    perror_with_name("RecvReply: Error reading socket.");
		}
		/*
		 * Check message number before the size because this could
		 * be an old packet.
		 */
		if (*(int *)replyBuffer != msgNum) {
		    printf("RecvReply: Old message number = %d, expecting %d\n",
			    *(int *)replyBuffer, msgNum);
		    fflush(stdout);
		    resendRequest = 1;
		    continue;
		}
		length = *( int *)(replyBuffer + 4);
		if (bytesRead - 8 != length) {
		    fprintf(stderr, "RecyReply: Short read for syslog data\n");
		    fflush(stderr);
		    length = bytesRead - 8;
		}
		if (length == 0) {
		    /*
		     * No data.
		     */
		    *readStatusPtr = 0;
		    return;
		}
		/*
		 * Dump out the buffer.
		 */
		write(1, replyBuffer + 8, length);
		*readStatusPtr = 1;
		return;
	    } else {
		/*
		 * Normal request so just read in the message which includes
		 * the message number.
		 */
		immediate_quit++;
		bytesRead = read(netSocket, replyBuffer, numBytes + 4);
		immediate_quit--;
		if (bytesRead < 0) {
		    MARK_DISCONNECTED;
		    perror_with_name("RecvReply: Error reading socket (2).");
		}
		/*
		 * Check message number before size because it could be
		 * an old packet.
		 */
		if (*(int *)replyBuffer != msgNum) {
		    printf("RecvReply: Old message number = %d, expecting %d\n",
			    *(int *)replyBuffer, msgNum);
		    fflush(stdout);
		    resendRequest = 1;
		    continue;
		}
		if (bytesRead != numBytes + 4) {
		    printf("RecvReply: Short read (2): op=%d exp=%d read=%d",
			    opcode, numBytes + 4, bytesRead);
		}
		if (*(int *)replyBuffer != msgNum) {
		    continue;
		}
		bcopy(replyBuffer + 4, destAddr, numBytes);
		return;
	    }
	} while (1);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *  WaitForKernel --
 *
 *      Wait for the kernel to send us a message to indicate that it is waiting
 *	to be debugged.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 * ----------------------------------------------------------------------------
 */
static void
WaitForKernel()
{
	int	dummy;

	RecvReply(DBG_CONTINUE, 4, &dummy, NULL, 0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * BlockInCache --
 *
 *     See if the given block at the given address is in the cache.
 *
 * Results:
 *     1 if found block in cache, 0 if didn't.
 *
 * Side effects:
 *     None.
 */
static int
BlockInCache(blockNum, addr)
    int		blockNum;
    char	*addr;
{
    blockNum = blockNum & CACHE_BLOCK_MASK;
    return((int) (cacheInfo[blockNum].version == currentVersion &&
	   (unsigned int) cacheInfo[blockNum].realAddr == 
		    ((unsigned int) (addr) & ~CACHE_BLOCK_OFFSET_MASK)));
}


/*
 * ----------------------------------------------------------------------------
 *
 * FetchBlock --
 *
 *     Fetch the given data block from the cache or the kernel if necessary.
 *
 * Results:
 *     1 if could fetch block into cache, 0 if couldn't.
 *
 * Side effects:
 *     Data cache modified.
 */
static int
FetchBlock(blockNum, srcAddr, opcode)
    int		blockNum;
    char	*srcAddr;
    Dbg_Opcode	opcode;
{
    int	successfulRead;

    blockNum = blockNum & CACHE_BLOCK_MASK;
    srcAddr = (char *) ((unsigned int) (srcAddr) & ~CACHE_BLOCK_OFFSET_MASK);

    if (BlockInCache(blockNum, srcAddr)) {
	return(1);
    }
    msg.opcode = opcode;
    msg.data.readMem.address = (int) srcAddr;
    msg.data.readMem.numBytes = cacheBlockSize;
    SendRequest(sizeof(msg.opcode) + sizeof(Dbg_ReadMem), 1);
    RecvReply(opcode, cacheBlockSize, 
		&dataCache[(unsigned int) (srcAddr) & CACHE_OFFSET_MASK],
		&successfulRead, 1);
    if (successfulRead) {
	cacheInfo[blockNum].version = currentVersion;
	cacheInfo[blockNum].realAddr = srcAddr;
    }
    return(successfulRead);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Kdbx_Trace --
 *
 *     Write the trace command over to the kernel.  
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
int 
Kdbx_Trace(opcode, srcAddr, destAddr, numBytes)
    Dbg_Opcode	opcode;		/* Which command */
    char	*srcAddr;	/* Where to read data from */
    char	*destAddr;	/* Where to write data to */
    int		numBytes;	/* The number of bytes to read or write */
{
    int			(*intrHandler)();
    int			i;
    int			retVal = 0;

#ifdef MYDEBUG
    fprintf(stdout, "Kdbx_Trace -- Opcode: %d, SrcAddr: %x, DestAddr: %x\n", opcode, (int)srcAddr, (int)destAddr);
    fflush(stdout);
#endif
    if (!initialized) {
	int	moreData;
	/*
	 * Setup the cache and initiate a conversation with the other kernel.
	 */
	if (cacheBlockSize == -1) {
	     {
		cacheBlockSize = 256;
		cacheBlockShift = 8;
	    }
	}
	dataCache = (char *) malloc(cacheSize);
	cacheInfo = (CacheInfo *) malloc(NUM_CACHE_BLOCKS * sizeof(CacheInfo));
	for (i = 0; i < NUM_CACHE_BLOCKS; i++) {
	    cacheInfo[i].version = 0;
	}
	StartDebugger();
	/*
	 * Dump the system log by faking a call command.
	 */
	printf("Dumping system log ...\n");
	fflush(stdout);
	msg.opcode = (short)DBG_BEGIN_CALL;
	SendRequest(sizeof(msg.opcode), 1);
	msg.opcode = (short)DBG_END_CALL;
	do {
	    SendRequest(sizeof(msg.opcode), 1);
	    RecvReply(msg.opcode, 0, NULL, &moreData, 1);
	} while (moreData);
	initialized = 1;
    }


    if (opcode == DBG_DATA_READ || opcode == DBG_INST_READ) {
	int			firstBlock;
	int			lastBlock;
	unsigned	int	cacheOffset;
	int			toRead;

	/*
	 * Read using the cache.
	 */
	firstBlock = GET_CACHE_BLOCK(srcAddr); 
	lastBlock = GET_CACHE_BLOCK(srcAddr + numBytes - 1);
	for (i = firstBlock; i <= lastBlock; i++) {
	    cacheOffset = ((unsigned int) srcAddr) & CACHE_OFFSET_MASK;
	    if (i == lastBlock) {
		toRead = numBytes;
	    } else if (i == firstBlock) {
		toRead = cacheBlockSize - 
			    (cacheOffset & CACHE_BLOCK_OFFSET_MASK);
	    } else {
		toRead = cacheBlockSize;
	    }
	    if (!FetchBlock(i, srcAddr, opcode)) {
#ifdef MYDEBUG
	      printf("Returning...7\n");
	      fflush(stdout);
#endif
	      return EIO;
	    }
	    bcopy(&dataCache[cacheOffset], destAddr, toRead);
	    srcAddr += toRead;
	    destAddr += toRead;
	    numBytes -= toRead;
	}
#ifdef MYDEBUG
	      printf("Returning...6\n");
	      fflush(stdout);
#endif
	return(0);
    }

    if (opcode == DBG_DATA_WRITE || opcode == DBG_INST_WRITE) {
	int	firstBlock;
	int	lastBlock;
	int	cacheOffset;
	int	toWrite;
	char	*tSrcAddr;
	char	*tDestAddr;
	int	tNumBytes;

	/*
	 * If the block that is being fetched is in the cache then write the
	 * data there first before sending it over to the kernel.
	 */
	tSrcAddr = srcAddr;
	tDestAddr = destAddr;
	tNumBytes = numBytes;

	firstBlock = GET_CACHE_BLOCK(destAddr); 
	lastBlock = GET_CACHE_BLOCK(destAddr + numBytes - 1);
	for (i = firstBlock; i <= lastBlock; i++) {
	    cacheOffset = ((int) tDestAddr) & CACHE_OFFSET_MASK;
	    if (i == lastBlock) {
		toWrite = tNumBytes;
	    } else if (i == firstBlock) {
		toWrite = cacheBlockSize - 
			    (cacheOffset & CACHE_BLOCK_OFFSET_MASK);
	    } else {
		toWrite = cacheBlockSize;
	    }
	    if (BlockInCache(i, tDestAddr)) {
		bcopy(tSrcAddr, &dataCache[cacheOffset], tNumBytes);
	    }
	    tSrcAddr += toWrite;
	    tDestAddr += toWrite;
	    tNumBytes -= toWrite;
	}
    }

    msg.opcode = (short) opcode;

    /*
     * Do the rest of the work for the desired operation.
     */

    switch (opcode) {

	/*
	 * For these operations the desired data is read from the other
	 * kernel and stored at destAddr.
	 */
	case DBG_READ_ALL_REGS:
	case DBG_GET_STOP_INFO:
	    SendRequest(sizeof(msg.opcode), 1);
	    RecvReply(opcode, numBytes, destAddr, NULL, 1);
	    break;

	/*
	 * For this operation the desired data is read from srcAddr
	 * and written to the other kernel.
	 */
	case DBG_SET_PID:
	    msg.data.pid = *(int *)srcAddr;
	    SendRequest(sizeof(msg.opcode) + sizeof(msg.data.pid), 1);
	    break;

	/*
	 * When writing a general purpose register first the address to write
	 * that is stored in destAddr must be given to the other kernel.
	 * Then the data itself which is stored at srcAddr can be written over.
	 */
	case DBG_WRITE_REG:
	    msg.data.writeReg.regNum = (int) destAddr;
	    msg.data.writeReg.regVal = *(int *) srcAddr;
	    SendRequest(sizeof(msg.opcode) + sizeof(Dbg_WriteReg), 1);
	    break;

	/*
	 * When writing to the kernels instruction or data space first the
	 * address of where to write to (destAddr) and then the number of
	 * bytes to write (numBytes) must be sent over.  Finally all of
	 * the data is read from srcAddr and written over.
	 */

	case DBG_INST_WRITE:
	case DBG_DATA_WRITE: {
	    char	writeStatus;

	    msg.data.writeMem.address = (int) destAddr;
	    msg.data.writeMem.numBytes = numBytes;
	    bcopy(srcAddr, msg.data.writeMem.buffer, numBytes);
	    SendRequest(sizeof(msg.opcode) + 2 * sizeof(int) + numBytes, 1);
	    RecvReply(opcode, 1, &writeStatus, NULL, 1);
	    if (writeStatus == 0) {
		retVal = EIO;
		fprintf(stderr, "ERROR: invalid write address 0x%x\n",destAddr);
	    } 
	    break;
	}
	case DBG_DIVERT_SYSLOG:
	    msg.data.syslogCmd = (Dbg_SyslogCmd)srcAddr;
	    SendRequest(sizeof(msg.opcode) + sizeof(Dbg_SyslogCmd), 1);
	    break;
	case DBG_BEGIN_CALL:
	    SendRequest(sizeof(msg.opcode), 1);
	    break;
	case DBG_END_CALL: {
	    Boolean	moreData;
	    do {
		SendRequest(sizeof(msg.opcode), 1);
		RecvReply(opcode, 0, NULL, &moreData, 1);
	    } while (moreData);
	    break;
	}

	case DBG_DETACH: {
	    msg.opcode = (short) DBG_DIVERT_SYSLOG;
	    msg.data.syslogCmd = DBG_SYSLOG_TO_ORIG;
	    SendRequest(sizeof(msg.opcode) + sizeof(Dbg_SyslogCmd), 1);

	    msg.opcode = (short) DBG_DETACH;
	    msg.data.pc = *(int *) srcAddr;
	    SendRequest(sizeof(msg.opcode) + sizeof(msg.data.pc), 1);
	    break;
	}

	case DBG_CONTINUE:
	case DBG_SINGLESTEP:
	    SendRequest(sizeof(msg.opcode) + sizeof(msg.data.pc), 1);
	    currentVersion++;
	    WaitForKernel();
	    break;

	case DBG_CALL_FUNCTION: {
	    int		returnValue;

	    msg.data.callFunc.address = (int) destAddr;
	    msg.data.callFunc.numBytes = numBytes;
	    bcopy(srcAddr, msg.data.callFunc.buffer, numBytes);
	    SendRequest(sizeof(msg.opcode) + 2 * sizeof(int) + numBytes, 1);
	    RecvReply(opcode, sizeof(returnValue), &returnValue, NULL, 1);
#ifdef MYDEBUG
	      printf("Returning...5\n");
	      fflush(stdout);
#endif
	    return (returnValue);
	}
	case DBG_REBOOT: {
	    msg.data.reboot.stringLength = numBytes;
	    bcopy(srcAddr, msg.data.reboot.string, numBytes);
	    SendRequest(sizeof(msg.opcode) + sizeof(int) + numBytes, 1);
#ifdef MYDEBUG
	      printf("Returning...4\n");
	      fflush(stdout);
#endif
	    return (0);
	}
	case DBG_GET_VERSION_STRING: {
	    SendRequest(sizeof(msg.opcode), 1);
	    RecvReply(opcode,numBytes , destAddr, NULL, 1);
#ifdef MYDEBUG
	      printf("Returning...3\n");
	      fflush(stdout);
#endif
	    return (0);
	}
	default:
	    printf("Unknown opcode %d\n", opcode);
#ifdef MYDEBUG
	      printf("Returning...2\n");
	      fflush(stdout);
#endif
	    return(-1);
    }
#ifdef MYDEBUG
	      printf("Returning...1, %d\n", retVal);
	      fflush(stdout);
#endif
    return(retVal);
}


remote_clean_up()
{
	MARK_DISCONNECTED;
}


read_kmem(memaddr, myaddr, len)
    char *memaddr;
    char *myaddr;
    int	  len;
{
    static int fd = -1;
    int count;

    if (fd < 0) { 
	char template[100];
	/*
	 * Open a temp file to write counters to. We unlink the file so it will
	 * disappear when we exit.
	 */
	strcpy(template, "/tmp/kgdbXXXXXXXX");
	fd = mkstemp(template);
	if (fd < 0) {
	    error("open kmem tmp file");
	}
	(void) unlink(template);
    }
    if (lseek(fd, 0, L_SET) < 0) {
	error("lseek kmem file");
    }
    count = write(fd, memaddr, len);
    if (count != len) {
	return EIO;
    }

    /*
     * Rewind the file and read the counters from it.
     */
    count = lseek(fd, 0, L_SET);
    if (count >= 0) {
        count = read(fd, myaddr, len);
    }
    if (count != len) {
	return EIO;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * remote_core_file_command --
 *
 *	kgdb.sun4 core_file command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
remote_core_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  /* Discard all vestiges of any previous core file
     and mark data and stack spaces as empty.  */

  if (remoteCoreChan >= 0)
    close (remoteCoreChan);
  remoteCoreChan = -1;
  if (filename == 0) {
    printf ("Corefile cleared.\n");
    return;
  }

  remoteCoreChan = open(filename, O_RDONLY, 0);
  if (remoteCoreChan < 0)
    perror_with_name (filename);

  {
    /*
     * Read the StopInfo and bounds and start the debugging session.
     */
    int val;
    
    val = bcopy (bfd_get_vm_core_stopinfo(core_bfd), &remoteStopInfo, sizeof remoteStopInfo);
    if (val < 0)
      perror_with_name (filename);
    val = bcopy (bfd_get_vm_core_bounds(core_bfd), &remoteBounds, sizeof remoteBounds);
    if (val < 0)
      perror_with_name (filename);
    remote_debugging = 1;
    remoteOffset = remoteBounds.kernelCodeStart - sizeof(remoteStopInfo) -
		    sizeof(remoteBounds);
  }
}
