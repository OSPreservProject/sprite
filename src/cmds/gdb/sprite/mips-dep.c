/* Low level interface to ptrace, for GDB when running under Unix.
   Copyright (C) 1988, 1989 Free Software Foundation, Inc.

This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * The port to MIPS-based machines was mostly done
 * by Per Bothner, bothner@cs.wisc.edu.
 * The inspiration for the heuristics to deal with missing
 * symbol tables, as well as many of the specific ideas for doing so,
 * comes from Alessando Forin (Alessando.Forin@spice.cs.cmu.edu).
 */
#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "value.h"

#ifdef USG
#include <sys/types.h>
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/dir.h>
#include <signal.h>
#include <sys/ioctl.h>
/* #include <fcntl.h>  Can we live without this?  */

#ifdef COFF_ENCAPSULATE
#include "a.out.encap.h"
#else
#include <a.out.h>
#endif
#ifndef N_SET_MAGIC
#define N_SET_MAGIC(exec, val) ((exec).a_magic = (val))
#endif
#define VM_MIN_ADDRESS (unsigned)0x400000
#include <sys/user.h>		/* After a.out.h  */
#include <sys/file.h>
#include <sys/stat.h>
#include <ds3100.md/reg.h>

#ifdef COFF_FORMAT
#include "symtab.h"

#ifdef USG
#include <sys/types.h>
#include <fcntl.h>
#endif

#include <obstack.h>
/*#include <sys/param.h>*/
/*#include <sys/file.h>*/

#ifdef sprite
#include <syms.h>
#endif

#ifdef KGDB
#include <kernel/ds3100.md/machAsmDefs.h>
#endif

extern void close ();

extern PDR *proc_desc_table;
extern long proc_desc_length;

struct linked_proc_info
{
  mips_proc_info info;
  struct linked_proc_info *next;
} * linked_proc_desc_table = NULL;

extern int errno;
extern int attach_flag;

/* This function simply calls ptrace with the given arguments.  
   It exists so that all calls to ptrace are isolated in this 
   machine-dependent file. */
int
call_ptrace (request, pid, arg3, arg4)
     int request, pid, arg3, arg4;
{
  return ptrace (request, pid, arg3, arg4);
}

kill_inferior ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  ptrace (8, inferior_pid, 0, 0);
  wait (0);
  inferior_died ();
}

/* This is used when GDB is exiting.  It gives less chance of error.*/

kill_inferior_fast ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  ptrace (8, inferior_pid, 0, 0);
  wait (0);
}

/* Resume execution of the inferior process.
   If STEP is nonzero, single-step it.
   If SIGNAL is nonzero, give it that signal.  */

void
resume (step, signal)
     int step;
     int signal;
{
  errno = 0;
  if (remote_debugging)
    remote_resume (step, signal);
  else
    {
      ptrace (step ? 9 : 7, inferior_pid, 1, signal);
      if (errno)
	perror_with_name ("ptrace");
    }
}

static char register_ptrace_map[NUM_REGS] = {
    /* general registers */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    /* floating-point registers */
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    /* special registers */
    PC, /* 96, pc */
    CAUSE, /* 97,cause */
    MMHI, /* 98,mdhi */
    MMLO, /* 99,mdlow */
    FPC_CSR,/* 100,fpc_csr */
    FPC_EIR,/* 101,fpc_eir */
    -1 /* fp */
  };
    
void
fetch_inferior_registers ()
{
  register int regno;
  register int regaddr;
  char buf[MAX_REGISTER_RAW_SIZE];
  register int i;
  extern char registers[];

  if (remote_debugging)
      remote_fetch_registers (registers);
  else
    {
      for (regno = 0; regno < NUM_REGS; regno++)
	{
	  regaddr = register_ptrace_map[regno];
	  if (regaddr < 0) continue;
	  for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (int))
	    {
	      *(int *) &buf[i] = ptrace (3, inferior_pid, regaddr, 0);
	      regaddr++;
	    }
	  supply_register (regno, buf);
	}
    }
  /* for now, make FP = SP */
  *(long*)buf = read_register(SP_REGNUM);
  supply_register(FP_REGNUM, buf);
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

store_inferior_registers (regno)
     int regno;
{
  register int regaddr;
  char buf[80];
  extern char registers[];
  unsigned int i;

  if (regno > FP_REGNUM)
    return;
  if (remote_debugging) {
    remote_store_registers (registers, regno);
    return;
  }
  if (regno >= 0)
    {
      regaddr = register_ptrace_map[regno];
      for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof(int))
	{
	  errno = 0;
	  ptrace (PT_WRITE_U, inferior_pid, regaddr,
		  *(int *) &registers[REGISTER_BYTE (regno) + i]);
	  if (errno != 0 && regno != CAUSE_REGNUM)
	    {
	      sprintf (buf, "writing register number %d(ptrace#%d)",
		       regno, regaddr);
	      perror_with_name (buf);
	    }
	  regaddr++;
	}
      return;
    }

 if (read_register (0) != 0)
   {
     int zero = 0;
     error ("Cannot change register 0.");
     supply_register (regno, &zero);
   }
  for (regno = 1; regno < FP_REGNUM; regno++)
    {
      regaddr = register_ptrace_map[regno];
      if (regaddr < 0) continue;
      for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof(int))
	{
	  errno = 0;
	  ptrace (PT_WRITE_U, inferior_pid, regaddr,
		  *(int *) &registers[REGISTER_BYTE (regno) + i]);
	  if (errno != 0)
	    {
	      sprintf (buf, "writing all regs, number %d(ptrace#%d)",
		       regno, regaddr);
	      perror_with_name (buf);
	    }
	  regaddr++;
	}
    }
}

/* Copy LEN bytes from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR. 
   On failure (cannot read from inferior, usually because address is out
   of bounds) returns the value of errno. */

int
read_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & - sizeof (int);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (int) - 1) / sizeof (int);
  /* Allocate buffer of that many longwords.  */
  register int *buffer = (int *) alloca (count * sizeof (int));
  extern int errno;

  /* Read all the longwords */
  for (i = 0; i < count; i++, addr += sizeof (int))
    {
      errno = 0;
#if 0
This is now done by read_memory, because when this function did it,
  reading a byte or short int hardware port read whole longs, causing
  serious side effects
  such as bus errors and unexpected hardware operation.  This would
  also be a problem with ptrace if the inferior process could read
  or write hardware registers, but that's not usually the case.
      if (remote_debugging)
	buffer[i] = remote_fetch_word (addr);
      else
#endif
	buffer[i] = ptrace (1, inferior_pid, addr, 0);
      if (errno)
	return errno;
    }

  /* Copy appropriate bytes out of the buffer.  */
  bcopy ((char *) buffer + (memaddr & (sizeof (int) - 1)), myaddr, len);
  return 0;
}

/* Copy LEN bytes of data from debugger memory at MYADDR
   to inferior's memory at MEMADDR.
   On failure (cannot write the inferior)
   returns the value of errno.  */

int
write_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & - sizeof (int);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (int) - 1) / sizeof (int);
  /* Allocate buffer of that many longwords.  */
  register int *buffer = (int *) alloca (count * sizeof (int));
  extern int errno;

  /* Fill start and end extra bytes of buffer with existing memory data.  */

  if (remote_debugging)
    buffer[0] = remote_fetch_word (addr);
  else
    buffer[0] = ptrace (1, inferior_pid, addr, 0);

  if (count > 1)
    {
      if (remote_debugging)
	buffer[count - 1]
	  = remote_fetch_word (addr + (count - 1) * sizeof (int));
      else
	buffer[count - 1]
	  = ptrace (1, inferior_pid,
		    addr + (count - 1) * sizeof (int), 0);
    }

  /* Copy data to be written over corresponding part of buffer */

  bcopy (myaddr, (char *) buffer + (memaddr & (sizeof (int) - 1)), len);

  /* Write the entire buffer.  */

  for (i = 0; i < count; i++, addr += sizeof (int))
    {
      errno = 0;
      if (remote_debugging)
	remote_store_word (addr, buffer[i]);
      else
	ptrace (4, inferior_pid, addr, buffer[i]);
      if (errno)
	return errno;
    }

  return 0;
}

/* Work with core dump and executable files, for GDB. 
   This code would be in core.c if it weren't machine-dependent. */

#ifndef N_TXTADDR
#define N_TXTADDR(hdr) 0
#endif /* no N_TXTADDR */

#ifndef N_DATADDR
#define N_DATADDR(hdr) hdr.a_text
#endif /* no N_DATADDR */

/* Make COFF and non-COFF names for things a little more compatible
   to reduce conditionals later.  */

#ifdef COFF_FORMAT
#undef a_magic
#define a_magic magic
#endif

#ifndef COFF_FORMAT
#ifndef AOUTHDR
#define AOUTHDR struct exec
#endif
#endif

extern char *sys_siglist[];


/* Hook for `exec_file_command' command to call.  */

extern void (*exec_file_display_hook) ();
   
/* File names of core file and executable file.  */

extern char *corefile;
extern char *execfile;

/* Descriptors on which core file and executable file are open.
   Note that the execchan is closed when an inferior is created
   and reopened if the inferior dies or is killed.  */

extern int corechan;
extern int execchan;

/* Last modification time of executable file.
   Also used in source.c to compare against mtime of a source file.  */

extern int exec_mtime;

/* Virtual addresses of bounds of the two areas of memory in the core file.  */

extern CORE_ADDR data_start;
extern CORE_ADDR data_end;
extern CORE_ADDR stack_start;
extern CORE_ADDR stack_end;

/* Virtual addresses of bounds of two areas of memory in the exec file.
   Note that the data area in the exec file is used only when there is no core file.  */

extern CORE_ADDR text_start;
extern CORE_ADDR text_end;

extern CORE_ADDR exec_data_start;
extern CORE_ADDR exec_data_end;

/* Address in executable file of start of text area data.  */

extern int text_offset;

/* Address in executable file of start of data area data.  */

extern int exec_data_offset;

/* Address in core file of start of data area data.  */

extern int data_offset;

/* Address in core file of start of stack area data.  */

extern int stack_offset;

/* various coff data structures */

extern FILHDR file_hdr;
extern SCNHDR text_hdr;
extern SCNHDR data_hdr;

#endif /* not COFF_FORMAT */

/* a.out header saved in core file.  */
  
extern AOUTHDR core_aouthdr;

/* a.out header of exec file.  */

extern AOUTHDR exec_aouthdr;

extern void validate_files ();

#ifndef sprite
core_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;
  extern char registers[];

  /* Discard all vestiges of any previous core file
     and mark data and stack spaces as empty.  */

  if (corefile)
    free (corefile);
  corefile = 0;

  if (corechan >= 0)
    close (corechan);
  corechan = -1;

  data_start = 0;
  data_end = 0;
  stack_start = STACK_END_ADDR;
  stack_end = STACK_END_ADDR;

  /* Now, if a new core file was specified, open it and digest it.  */

  if (filename)
    {
      filename = tilde_expand (filename);
      make_cleanup (free, filename);
      
      if (have_inferior_p ())
	error ("To look at a core file, you must kill the inferior with \"kill\".");
      corechan = open (filename, O_RDONLY, 0);
      if (corechan < 0)
	perror_with_name (filename);
      /* 4.2-style (and perhaps also sysV-style) core dump file.  */
      {
	struct user u;

	unsigned int reg_offset;

	val = myread (corechan, &u, sizeof u);
	if (val < 0)
	  perror_with_name ("Not a core file: reading upage");
	if (val != sizeof u)
	  error ("Not a core file: could only read %d bytes", val);
	data_start = exec_data_start;

	data_end = data_start + NBPG * u.u_dsize;
	stack_start = stack_end - NBPG * u.u_ssize;
	data_offset = NBPG * UPAGES;
	stack_offset = NBPG * (UPAGES + u.u_dsize);

	/* Some machines put an absolute address in here and some put
	   the offset in the upage of the regs.  */
	reg_offset = (int) u.u_ar0;
	if (reg_offset > NBPG * UPAGES)
	  reg_offset -= KERNEL_U_ADDR;

	/* I don't know where to find this info.
	   So, for now, mark it as not available.  */
	N_SET_MAGIC (core_aouthdr, 0);

	/* Read the register values out of the core file and store
	   them where `read_register' will find them.  */

	{
	  register int regno;
	  char buf[MAX_REGISTER_RAW_SIZE];

	  *(long*)buf = 0;
	  supply_register (regno, buf);

	  for (regno = 1; regno < NUM_REGS; regno++)
	    {
	      int offset;

#define PCB_OFFSET(FIELD) ((int)&((struct user*)0)->u_pcb.FIELD)
	      if (regno < FP0_REGNUM)
		  offset =  UPAGES*NBPG-EF_SIZE+4*((regno)+EF_AT-1);
	      else if (regno < PC_REGNUM)
		  offset = PCB_OFFSET(pcb_fpregs[0]) + 4*(regno-FP0_REGNUM);
	      else if (regno == LO_REGNUM)
		  offset = UPAGES*NBPG-EF_SIZE+4*EF_MDLO;
	      else if (regno == HI_REGNUM)
		  offset = UPAGES*NBPG-EF_SIZE+4*EF_MDHI;
	      else if (regno == PC_REGNUM)
		  offset = UPAGES*NBPG-EF_SIZE+4*EF_EPC;
	      else if (regno == CAUSE_REGNUM)
		  offset = UPAGES*NBPG-EF_SIZE+4*EF_CAUSE;
	      else if (regno == FCRIR_REGNUM)
		  offset = PCB_OFFSET(pcb_fpc_eir);
	      else if (regno == FCRCS_REGNUM)
		  offset = PCB_OFFSET(pcb_fpc_csr);
	      else
		  continue;

	      val = lseek (corechan, offset, 0);
	      if (val < 0
		  || (val = myread (corechan, buf, sizeof buf)) < 0)
		{
		  char * buffer = (char *) alloca (strlen (reg_names[regno])
						   + 30);
		  strcpy (buffer, "Reading register ");
		  strcat (buffer, reg_names[regno]);
						   
		  perror_with_name (buffer);
		}

	      supply_register (regno, buf);
	    }
	}
      }
      if (filename[0] == '/')
	corefile = savestring (filename, strlen (filename));
      else
	{
	  corefile = concat (current_directory, "/", filename);
	}

      set_current_frame ( create_new_frame (read_register (FP_REGNUM),
					    read_pc ()));
      select_frame (get_current_frame (), 0);
      validate_files ();
    }
  else if (from_tty)
    printf ("No core file now.\n");
}
#endif

exec_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;

  /* Eliminate all traces of old exec file.
     Mark text segment as empty.  */

  if (execfile)
    free (execfile);
  execfile = 0;
  data_start = 0;
  data_end -= exec_data_start;
  text_start = 0;
  text_end = 0;
  exec_data_start = 0;
  exec_data_end = 0;
  if (execchan >= 0)
    close (execchan);
  execchan = -1;

  /* Now open and digest the file the user requested, if any.  */

  if (filename)
    {
      filename = tilde_expand (filename);
      make_cleanup (free, filename);
      
      execchan = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0,
			&execfile);
      if (execchan < 0)
	perror_with_name (filename);

      {
	int aout_hdrsize;
	int num_sections;

	if (read_file_hdr (execchan, &file_hdr) < 0)
	  error ("\"%s\": not in executable format.", execfile);

	aout_hdrsize = file_hdr.f_opthdr;
	num_sections = file_hdr.f_nscns;

	if (read_aout_hdr (execchan, &exec_aouthdr, aout_hdrsize) < 0)
	  error ("\"%s\": can't read optional aouthdr", execfile);

	if (read_section_hdr (execchan, _TEXT, &text_hdr, num_sections) < 0)
	  error ("\"%s\": can't read text section header", execfile);

	if (read_section_hdr (execchan, _DATA, &data_hdr, num_sections) < 0)
	  error ("\"%s\": can't read data section header", execfile);

	text_start = exec_aouthdr.text_start;
	text_end = text_start + exec_aouthdr.tsize;
	text_offset = 0; /* text_hdr.s_scnptr */
	exec_data_start = exec_aouthdr.data_start;
	exec_data_end = exec_data_start + exec_aouthdr.dsize;
	exec_data_offset = exec_aouthdr.tsize; /*data_hdr.s_scnptr;*/
	data_start = exec_data_start;
	data_end += exec_data_start;
	exec_mtime = file_hdr.f_timdat;
      }

      validate_files ();
    }
  else if (from_tty)
    printf ("No exec file now.\n");

  /* Tell display code (if any) about the changed file name.  */
  if (exec_file_display_hook)
    (*exec_file_display_hook) (filename);
}

#define READ_FRAME_REG(fi, regno) read_next_frame_reg((fi)->next, regno)

int
read_next_frame_reg(fi, regno)
     FRAME fi;
     int regno;
{
  for (; fi; fi = fi->next)
      if (regno == SP_REGNUM) return fi->frame;
      else if (fi->saved_regs->regs[regno])
	return read_memory_integer(fi->saved_regs->regs[regno], 4);
  return read_register(regno);
}

int
frame_saved_pc(frame)
     FRAME frame;
{
  mips_proc_info *proc_desc = (mips_proc_info*)frame->proc_desc;
  int pcreg = proc_desc ? PROC_PC_REG(proc_desc) : RA_REGNUM;
  if (proc_desc && PROC_DESC_IS_DUMMY(proc_desc))
      return read_memory_integer(frame->frame - 4, 4);
#if 0
  /* If in the procedure prologue, RA_REGNUM might not have been saved yet.
   * Assume non-leaf functions start with:
   *	addiu $sp,$sp,-frame_size
   *	sw $ra,ra_offset($sp)
   * This if the pc is pointing at either of these instructions,
   * then $ra hasn't been trashed.
   * If the pc has advanced beyond these two instructions,
   * then $ra has been saved.
   * critical, and much more complex. Handling $ra is enough to get
   * a stack trace, but some register values with be wrong.
   */
  if (frame->proc_desc && frame->pc < PROC_LOW_ADDR(proc_desc) + 8)
      return read_register(pcreg);
#endif
#ifdef KGDB
  /*
   * Things are a little bit tricky here. Leaf routines may have a zero-size
   * frame. Unfortunately, we may get an exception or interrupt while
   * we are in a leaf routine, so that additional frames are pushed on the
   * stack. The return address of the leaf frame is not on the stack (there
   * is no room for it). Instead, the routine Mach_KernGenException pushes
   * all registers onto the stack, and we can find the return address there.
   * The current frame is the zero-sized one, and we have to go back a
   * couple to get the frame address we want.
   */
  if (proc_desc && (PROC_FRAME_OFFSET(proc_desc) == 0)) {
      int	addr;
      if (frame->next == NULL) {
	  return read_register(pcreg);
      }
      if (frame->next == NULL) {
	  return 0;
      }
      if (frame->next->next == NULL) {
	  return 0;
      }
      addr = frame->next->next->frame + 
	  STAND_FRAME_SIZE + 8 + SAVED_REG_SIZE - 4;
      return read_memory_integer(addr, 4);
  }
#endif
  return read_next_frame_reg(frame, pcreg);
}

static PDR temp_proc_desc;
static struct frame_saved_regs temp_saved_regs;

CORE_ADDR heuristic_proc_start(pc)
    CORE_ADDR pc;
{

    CORE_ADDR start_pc = pc;
    CORE_ADDR fence = start_pc - 10000;
    if (fence < VM_MIN_ADDRESS) fence = VM_MIN_ADDRESS;
    /* search back for previous return */
    for (start_pc -= 4; ; start_pc -= 4)
	if (start_pc < fence) return 0; 
	else if (ABOUT_TO_RETURN(start_pc))
	    break;

    start_pc += 8; /* skip return, and iys delay slot */
#if 0
    /* skip nops (usually 1) 0 - is this */
    while (start_pc < pc && read_memory_integer (start_pc, 4) == 0)
	start_pc += 4;
#endif
    return start_pc;
}

PDR *heuristic_proc_desc(start_pc, limit_pc, next_frame)
    CORE_ADDR start_pc, limit_pc;
    FRAME next_frame;
{
    CORE_ADDR sp = next_frame ? next_frame->frame : read_register (SP_REGNUM);
    CORE_ADDR cur_pc;
    extern int breakpoints_inserted;
    int frame_size;
    int has_frame_reg = 0;
    int reg30; /* Value of $r30. Used by gcc for frame-pointer */
    unsigned long reg_mask = 0;
    if (start_pc == 0) return NULL;
    bzero(&temp_proc_desc, sizeof(PDR));
    bzero(&temp_saved_regs, sizeof(struct frame_saved_regs));
    if (start_pc + 200 < limit_pc) limit_pc = start_pc + 200;
    remove_breakpoints ();
    breakpoints_inserted = 0;
  restart:
    frame_size = 0;
    for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += 4) {
	unsigned long word = read_memory_integer(cur_pc, 4);
	if ((word & 0xFFFF0000) == 0x27bd0000) /* addiu $sp,$sp,-i */
	    frame_size += (-word) & 0xFFFF;
	else if ((word & 0xFFFF0000) == 0x23bd0000) /* addu $sp,$sp,-i */
	    frame_size += (-word) & 0xFFFF;
	else if ((word & 0xFFE00000) == 0xafa00000) { /* sw reg,offset($sp) */
	    int reg = (word & 0x001F0000) >> 16;
	    reg_mask |= 1 << reg;
	    temp_saved_regs.regs[reg] = sp + (short)word;
	}
	else if ((word & 0xFFFF0000) == 0x27be0000) { /* addiu $30,$sp,size */
	    if ((unsigned short)word != frame_size)
		reg30 = sp + (unsigned short)word;
	    else if (!has_frame_reg) {
		int alloca_adjust;
		has_frame_reg = 1;
		reg30 = read_next_frame_reg(next_frame, 30);
		alloca_adjust = reg30 - (sp + (unsigned short)word);
		if (alloca_adjust > 0) {
		    /* FP > SP + frame_size. This may be because
		    /* of an alloca or somethings similar.
		     * Fix sp to "pre-alloca" value, and try again.
		     */
		    sp += alloca_adjust;
		    goto restart;
		}
	    }
	}
	else if ((word & 0xFFE00000) == 0xafc00000) { /* sw reg,offset($30) */
	    int reg = (word & 0x001F0000) >> 16;
	    reg_mask |= 1 << reg;
	    temp_saved_regs.regs[reg] = reg30 + (short)word;
	}
    }
    if (has_frame_reg) {
	PROC_FRAME_REG(&temp_proc_desc) = 30;
	PROC_FRAME_OFFSET(&temp_proc_desc) = 0;
    }
    else {
	PROC_FRAME_REG(&temp_proc_desc) = SP_REGNUM;
	PROC_FRAME_OFFSET(&temp_proc_desc) = frame_size;
    }
    PROC_REG_MASK(&temp_proc_desc) = reg_mask;
    PROC_PC_REG(&temp_proc_desc) = RA_REGNUM;
    return &temp_proc_desc;
}

mips_proc_info *
find_proc_desc(pc, next_frame)
    CORE_ADDR pc;
    FRAME next_frame;
{
  mips_proc_info *proc_desc;

  int lo = 0;
  int hi = proc_desc_length-1;
  if (hi >= 0 && PROC_LOW_ADDR(&proc_desc_table[0]) <= pc
      && PROC_HIGH_ADDR(&proc_desc_table[hi]) > pc)
    {
      for (;;) {
	int new = (lo + hi) >> 1;
	proc_desc = &proc_desc_table[new];
	if (PROC_LOW_ADDR(proc_desc) > pc) hi = new;
	else if (PROC_HIGH_ADDR(proc_desc) <= pc) lo = new;
	else {
	    /* IF this is the topmost frame AND
	     * (this proc does not have debugging information OR
	     * the PC is in the procedure prologue)
	     * THEN create a "hueristic" proc_desc (by analyzing
	     * the actual code) to replace the "official" proc_desc.
	     */
	    if (next_frame == NULL) {
		struct symtab_and_line val;
		struct symbol *proc_symbol = PROC_SYMBOL(proc_desc);
		if (proc_symbol) {
		    val = find_pc_line (BLOCK_START
					  (SYMBOL_BLOCK_VALUE(proc_symbol)),
					  0);
		    val.pc = val.end ? val.end : pc;
		}
		if (!proc_symbol || pc < val.pc) {
		    mips_proc_info *found_heuristic =
			heuristic_proc_desc(PROC_LOW_ADDR(proc_desc),
					    pc, next_frame);
		    if (found_heuristic) proc_desc = found_heuristic;
		}
	    }
	    break;
	}
	if (hi-lo <= 1) { proc_desc = 0; break; }
      }
    }
  else
    {
      register struct linked_proc_info *link;
      for (link = linked_proc_desc_table; link; link = link->next)
	  if (PROC_LOW_ADDR(&link->info) <= pc
	      && PROC_HIGH_ADDR(&link->info) > pc)
	      return &link->info;
      proc_desc =
	  heuristic_proc_desc(heuristic_proc_start(pc), pc, next_frame);
    }
  return proc_desc;
}

PDR *cached_proc_desc;

FRAME_ADDR frame_chain(frame)
    FRAME frame;
{
    extern CORE_ADDR startup_file_start;	/* From blockframe.c */
    mips_proc_info *proc_desc;
    CORE_ADDR saved_pc = frame_saved_pc(frame);
#ifndef KGDB
    if (startup_file_start)
      { /* has at least the __start symbol */
	if (saved_pc == 0 || !outside_startup_file (saved_pc)) return 0;
      }
    else
      { /* This hack depends on the internals of __start. */
	/* We also assume the breakpoints are *not* inserted */
        if (read_memory_integer (saved_pc + 8, 4) == 0xd) /* break */
	    return 0;
      }
#else
    if (saved_pc == 0) 
	return 0;
#endif
    proc_desc = find_proc_desc(saved_pc, frame);
    if (!proc_desc) return 0;
    cached_proc_desc = proc_desc;
    return read_next_frame_reg(frame, PROC_FRAME_REG(proc_desc))
	+ PROC_FRAME_OFFSET(proc_desc);
}

void
init_extra_frame_info(fci)
     struct frame_info *fci;
{
  extern struct obstack frame_cache_obstack;
  /* Use proc_desc calculated in frame_chain */
  PDR *proc_desc = fci->next ? cached_proc_desc :
      find_proc_desc(fci->pc, fci->next);
  fci->saved_regs = (struct frame_saved_regs*)
    obstack_alloc (&frame_cache_obstack, sizeof(struct frame_saved_regs));
  bzero(fci->saved_regs, sizeof(struct frame_saved_regs));
  fci->proc_desc =
      proc_desc == &temp_proc_desc ? (char*)NULL : (char*)proc_desc;
  if (proc_desc)
    {
      int ireg;
      CORE_ADDR reg_position;
      unsigned long mask;
      /* r0 bit means kernel trap */
      int kernel_trap = PROC_REG_MASK(proc_desc) & 1;

      /* Fixup frame-pointer - only needed for top frame */
      /* This may not be quite right, if procedure has a real frame register */
      if (fci->pc == PROC_LOW_ADDR(proc_desc))
	  fci->frame = read_register (SP_REGNUM);
      else
	  fci->frame = READ_FRAME_REG(fci, PROC_FRAME_REG(proc_desc))
	      + PROC_FRAME_OFFSET(proc_desc);

      if (proc_desc == &temp_proc_desc)
	  *fci->saved_regs = temp_saved_regs;
      else
      {
	  /* find which general-purpose registers were saved */
	  reg_position = fci->frame + PROC_REG_OFFSET(proc_desc);
	  mask = kernel_trap ? 0xFFFFFFFF : PROC_REG_MASK(proc_desc);
	  for (ireg= 31; mask; --ireg, mask <<= 1)
	      if (mask & 0x80000000)
	      {
		  fci->saved_regs->regs[ireg] = reg_position;
		  reg_position -= 4;
	      }
	  /* find which floating-point registers were saved */
	  reg_position = fci->frame + PROC_FREG_OFFSET(proc_desc);
	  /* The freg_offset points to where the first *double* register is saved.
	   * So skip to the high-order word. */
	  reg_position += 4;
	  mask = kernel_trap ? 0xFFFFFFFF : PROC_FREG_MASK(proc_desc);
	  for (ireg = 31; mask; --ireg, mask <<= 1)
	      if (mask & 0x80000000)
	      {
		  fci->saved_regs->regs[32+ireg] = reg_position;
		  reg_position -= 4;
	      }
      }

      /* hack: if argument regs are saved, guess these contain args */
      if ((PROC_REG_MASK(proc_desc) & 0xF0) == 0) fci->num_args = -1;
      else if ((PROC_REG_MASK(proc_desc) & 0xF0) == 0xF0) fci->num_args = 4;
      else if ((PROC_REG_MASK(proc_desc) & 0x70) == 0x70) fci->num_args = 3;
      else if ((PROC_REG_MASK(proc_desc) & 0x30) == 0x30) fci->num_args = 2;
      else if ((PROC_REG_MASK(proc_desc) & 0x10) == 0x10) fci->num_args = 1;

      fci->saved_regs->regs[PC_REGNUM] = fci->saved_regs->regs[RA_REGNUM];
    }
  if (fci->next == 0)
      supply_register(FP_REGNUM, &fci->frame);
}


CORE_ADDR mips_push_arguments(nargs, args, sp, struct_return, struct_addr)
  int nargs;
  value *args;
  CORE_ADDR sp;
  int struct_return;
  CORE_ADDR struct_addr;
{
  CORE_ADDR buf;
  register i;
  int accumulate_size = struct_return ? 4 : 0;
  struct mips_arg { char *contents; int len; int offset; };
  struct mips_arg *mips_args =
      (struct mips_arg*)alloca(nargs * sizeof(struct mips_arg));
  register struct mips_arg *m_arg;
  for (i = 0, m_arg = mips_args; i < nargs; i++, m_arg++) {
    extern value value_arg_coerce();
    value arg = value_arg_coerce (args[i]);
    m_arg->len = TYPE_LENGTH (VALUE_TYPE (arg));
    /* This entire mips-specific routine is because doubles must be aligned
     * on 8-byte boundaries. It still isn't quite right, because MIPS decided
     * to align 'struct {int a, b}' on 4-byte boundaries (even though this
     * breaks their varargs implementation...). A correct solution
     * requires an simulation of gcc's 'alignof' (and use of 'alignof'
     * in stdarg.h/varargs.h).
     */
    if (m_arg->len > 4) accumulate_size = (accumulate_size + 7) & -8;
    m_arg->offset = accumulate_size;
    accumulate_size = (accumulate_size + m_arg->len + 3) & -4;
    m_arg->contents = VALUE_CONTENTS(arg);
  }
  accumulate_size = (accumulate_size + 7) & (-8);
  if (accumulate_size < 16) accumulate_size = 16; 
  sp -= accumulate_size;
  for (i = nargs; m_arg--, --i >= 0; )
    write_memory(sp + m_arg->offset, m_arg->contents, m_arg->len);
  if (struct_return) {
    buf = struct_addr;
    write_memory(sp, &buf, sizeof(CORE_ADDR));
}
  return sp;
}

/* MASK(i,j) == (1<<i) + (1<<(i+1)) + ... + (1<<j)). Assume i<=j<31. */
#define MASK(i,j) ((1 << (j)+1)-1 ^ (1 << (i))-1)

void
push_dummy_frame()
{
  int ireg;
  struct linked_proc_info *link = (struct linked_proc_info*)
      xmalloc(sizeof(struct linked_proc_info));
  mips_proc_info *proc_desc = &link->info;
  CORE_ADDR sp = read_register (SP_REGNUM);
  CORE_ADDR save_address;
  REGISTER_TYPE buffer;
  link->next = linked_proc_desc_table;
  linked_proc_desc_table = link;
#define PUSH_FP_REGNUM 16 /* must be a register preserved across calls */
#define GEN_REG_SAVE_MASK MASK(1,16)|MASK(24,28)|(1<<31)
#define GEN_REG_SAVE_COUNT 22
#define FLOAT_REG_SAVE_MASK MASK(0,19)
#define FLOAT_REG_SAVE_COUNT 20
#define SPECIAL_REG_SAVE_COUNT 4
  /*
   * The registers we must save are all those not preserved across
   * procedure calls. Dest_Reg (see m-mips.h) must also be saved.
   * In addition, we must save the PC, and PUSH_FP_REGNUM.
   * (Ideally, we should also save MDLO/-HI and FP Control/Status reg.)
   *
   * Dummy frame layout:
   *  (high memory)
   * 	Saved PC
   *	Saved MMHI, MMLO, FPC_CSR
   *	Saved R31
   *	Saved R28
   *	...
   *	Saved R1
   *    Saved D18 (i.e. F19, F18)
   *    ...
   *    Saved D0 (i.e. F1, F0)
   *	CALL_DUMMY (subroutine stub; see m-mips.h)
   *	Parameter build area (not yet implemented)
   *  (low memory)
   */
  PROC_REG_MASK(proc_desc) = GEN_REG_SAVE_MASK;
  PROC_FREG_MASK(proc_desc) = FLOAT_REG_SAVE_MASK;
  PROC_REG_OFFSET(proc_desc) = /* offset of (Saved R31) from FP */
      -sizeof(long) - 4 * SPECIAL_REG_SAVE_COUNT;
  PROC_FREG_OFFSET(proc_desc) = /* offset of (Saved D18) from FP */
      -sizeof(double) - 4 * (SPECIAL_REG_SAVE_COUNT + GEN_REG_SAVE_COUNT);
  /* save general registers */
  save_address = sp + PROC_REG_OFFSET(proc_desc);
  for (ireg = 32; --ireg >= 0; )
    if (PROC_REG_MASK(proc_desc) & (1 << ireg))
      {
	buffer = read_register (ireg);
	write_memory (save_address, &buffer, sizeof(REGISTER_TYPE));
	save_address -= 4;
      }
  /* save floating-points registers */
  save_address = sp + PROC_FREG_OFFSET(proc_desc);
  for (ireg = 32; --ireg >= 0; )
    if (PROC_FREG_MASK(proc_desc) & (1 << ireg))
      {
	buffer = read_register (ireg);
	write_memory (save_address, &buffer, 4);
	save_address -= 4;
      }
  write_register (PUSH_FP_REGNUM, sp);
  PROC_FRAME_REG(proc_desc) = PUSH_FP_REGNUM;
  PROC_FRAME_OFFSET(proc_desc) = 0;
  buffer = read_register (PC_REGNUM);
  write_memory (sp - 4, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (HI_REGNUM);
  write_memory (sp - 8, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (LO_REGNUM);
  write_memory (sp - 12, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (FCRCS_REGNUM);
  write_memory (sp - 16, &buffer, sizeof(REGISTER_TYPE));
  sp -= 4 * (GEN_REG_SAVE_COUNT+FLOAT_REG_SAVE_COUNT+SPECIAL_REG_SAVE_COUNT);
  write_register (SP_REGNUM, sp);
  PROC_HIGH_ADDR(proc_desc) = sp;
  PROC_LOW_ADDR(proc_desc) = sp - CALL_DUMMY_SIZE;
  SET_PROC_DESC_IS_DUMMY(proc_desc);
  PROC_PC_REG(proc_desc) = RA_REGNUM;
}

void
pop_frame()
{ register int regnum;
  FRAME frame = get_current_frame ();
  CORE_ADDR new_sp = frame->frame;
  mips_proc_info *proc_desc = (PDR*)frame->proc_desc;
  if (PROC_DESC_IS_DUMMY(proc_desc))
    {
      struct linked_proc_info **ptr = &linked_proc_desc_table;;
      for (; &ptr[0]->info != proc_desc; ptr = &ptr[0]->next )
	  if (ptr[0] == NULL) abort();
      *ptr = ptr[0]->next;
      free (ptr[0]);
      write_register (HI_REGNUM, read_memory_integer(new_sp - 8, 4));
      write_register (LO_REGNUM, read_memory_integer(new_sp - 12, 4));
      write_register (FCRCS_REGNUM, read_memory_integer(new_sp - 16, 4));
    }
  write_register (PC_REGNUM, FRAME_SAVED_PC(frame));
  if (frame->proc_desc) {
    for (regnum = 32; --regnum >= 0; )
      if (PROC_REG_MASK(proc_desc) & (1 << regnum))
	write_register (regnum,
		  read_memory_integer (frame->saved_regs->regs[regnum], 4));
    for (regnum = 64; --regnum >= 32; )
      if (PROC_FREG_MASK(proc_desc) & (1 << regnum))
	write_register (regnum,
		  read_memory_integer (frame->saved_regs->regs[regnum], 4));
  }
  write_register (SP_REGNUM, new_sp);
  flush_cached_frames ();
  set_current_frame (create_new_frame (new_sp, read_pc ()));
}

static mips_print_register(regnum, all)
     int regnum, all;
{
      unsigned char raw_buffer[8];
      REGISTER_TYPE val;

      read_relative_register_raw_bytes (regnum, raw_buffer);

      if (!(regnum & 1) && regnum >= FP0_REGNUM && regnum < FP0_REGNUM+32) {
	  read_relative_register_raw_bytes (regnum+1, raw_buffer+4);
	  printf_filtered ("(d%d: ", regnum&31);
	  val_print (builtin_type_double, raw_buffer, 0,
		     stdout, 0, 1, 0, Val_pretty_default);
	  printf_filtered ("); ", regnum&31);
      }
      fputs_filtered (reg_names[regnum], stdout);
#ifndef NUMERIC_REG_NAMES
      if (regnum < 32)
	  printf_filtered ("(r%d): ", regnum);
      else
#endif
	  printf_filtered (": ");

      /* If virtual format is floating, print it that way.  */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT
	  && ! INVALID_FLOAT (raw_buffer, REGISTER_VIRTUAL_SIZE(regnum))) {
	  val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0,
		     stdout, 0, 1, 0, Val_pretty_default);
      }
      /* Else print as integer in hex.  */
      else
	{
	  long val;

	  bcopy (raw_buffer, &val, sizeof (long));
	  if (val == 0)
	    printf_filtered ("0");
	  else if (all)
	    printf_filtered ("0x%x", val);
	  else
	    printf_filtered ("0x%x=%d", val, val);
	}
}

mips_do_registers_info(regnum)
     int regnum;
{
  if (regnum != -1) {
      mips_print_register (regnum, 0);
      printf_filtered ("\n");
  }
  else {
      for (regnum = 0; regnum < NUM_REGS; ) {
	  mips_print_register (regnum, 1);
	  regnum++;
	  if ((regnum & 3) == 0 || regnum == NUM_REGS)
	      printf_filtered (";\n");
	  else
	      printf_filtered ("; ");
      }
  }
}
#ifdef ATTACH_DETACH

/* Start debugging the process whose number is PID.  */

int
attach (pid)
     int pid;
{
  errno = 0;
  ptrace (PT_ATTACH, pid, 0, 0);
  if (errno)
    perror_with_name ("ptrace");
  attach_flag = 1;
  return pid;
}

/* Stop debugging the process whose number is PID
   and continue it with signal number SIGNAL.
   SIGNAL = 0 means just continue it.  */

void
detach (signal)
     int signal;
{
  errno = 0;
  ptrace (PT_DETACH, inferior_pid, 1, signal);
  if (errno)
    perror_with_name ("ptrace");
  attach_flag = 0;
}
#endif /* ATTACH_DETACH */

