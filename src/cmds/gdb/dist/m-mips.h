/* Definitions to make GDB run on a MIPS-based machines.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.

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

/* Define the bit, byte, and word ordering of the machine.  */
#ifndef DECSTATION
#define BITS_BIG_ENDIAN
#define BYTES_BIG_ENDIAN
#define WORDS_BIG_ENDIAN
#endif

#define IEEE_FLOAT

#define FLOAT_INFO { printf("FPA coprocessor available.\n"); }

/* Get rid of any system-imposed stack limit if possible.  */

#define SET_STACK_LIMIT_HUGE

/* Define this if the C compiler puts an underscore at the front
   of external names before giving them to the linker.  */

#undef NAMES_HAVE_UNDERSCORE

/* Debugger information will be a variant of COFF */

/*#define READ_DBX_FORMAT*/
#define COFF_FORMAT
#define THIRD_EYE_FORMAT

/* We need to remember some procedure-specific values
   to do stack traversal. Use the mips_proc_info type. */

#define mips_proc_info PDR
#define PROC_LOW_ADDR(proc) ((proc)->lnLow) /* least address */
#define PROC_HIGH_ADDR(proc) ((proc)->lnHigh) /* upper address bound */
#define PROC_FRAME_OFFSET(proc) ((proc)->frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->framereg)
#define PROC_REG_MASK(proc) ((proc)->regmask)
#define PROC_FREG_MASK(proc) ((proc)->fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->fregoffset)
#define PROC_PC_REG(proc) ((proc)->pcreg)
#define PROC_SYMBOL(proc) (*(struct symbol**)&(proc)->isym)
#define _PROC_MAGIC_ 0x0F0F0F0F
#define PROC_DESC_IS_DUMMY(proc) ((proc)->isym == _PROC_MAGIC_)
#define SET_PROC_DESC_IS_DUMMY(proc) ((proc)->isym = _PROC_MAGIC_)

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(pc)	\
{ register int op = read_memory_integer (pc, 4);	\
  if ((op & 0xffff0000) == 0x27bd0000)			\
    pc += 4;   /* Skip addiu $sp,$sp,offset */		\
}

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) (CORE_ADDR)read_register(RA_REGNUM)

/* This is the amount to subtract from u.u_ar0
   to get the offset in the core file of the register values.  */

#define KERNEL_U_ADDR (0x80000000 - (UPAGES * NBPG))

/* Address of end of stack space.  */

#define STACK_END_ADDR 0x7ffff000

/* Stack grows downward.  */

#define INNER_THAN <

/* Stack has strict alignment. However, use PUSH_ARGUMENTS
   to take care of it. */
/*#define STACK_ALIGN(ADDR) (((ADDR)+7)&-8)*/

#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
    sp = mips_push_arguments(nargs, args, sp, struct_return, struct_addr)

/* Sequence of bytes for breakpoint instruction.  */

#ifdef BYTES_BIG_ENDIAN
#define BREAKPOINT {0, 0, 0, 13}
#else
#define BREAKPOINT {13, 0, 0, 0}
#endif

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Nonzero if instruction at PC is a return instruction.  */
/* For Mips, this is jr $ra */

#define ABOUT_TO_RETURN(pc) \
  (read_memory_integer (pc, 4) == 0x03e00008)

/* Return 1 if P points to an invalid floating point value.
   LEN is the length in bytes -- not relevant on the Vax.  */

#define INVALID_FLOAT(p, len) ((*(short *) p & 0xff80) == 0x8000)

/* Largest integer type */
#define LONGEST long

/* Name of the builtin type for the LONGEST type above. */
#define BUILTIN_TYPE_LONGEST builtin_type_long

/* Say how long (ordinary) registers are.  */

#define REGISTER_TYPE long

/* Number of machine registers */

#define NUM_REGS 71

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

/* Note that the symbol table parsing of scRegister symbols
 * assumes r0=0, ..., r3=31, and f0=32, ... f31=32+31 */

#ifdef NUMERIC_REG_NAMES
#define REGISTER_NAMES {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",\
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",\
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",\
  "r24", "r25", "r26", "r27", "gp", "sp", "r30", "ra",	\
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",	\
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",	\
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",\
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",\
  "pc", "cause", "hi", "lo", "fcrcs", "fcrir", "fp" };
#else /* !NUMERIC_REG_NAMES */
#define REGISTER_NAMES {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",\
  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",\
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",\
  "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",	\
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",	\
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",	\
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",\
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",\
  "pc", "cause", "hi", "lo", "fcrcs", "fcrir", "fp" };
#endif /* !NUMERIC_REG_NAMES */

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

/* Note the FP is a "virtual" register */
#define SP_REGNUM 29		/* Contains address of top of stack */
#define RA_REGNUM 31		/* Contains return address value */
#define FP0_REGNUM 32		/* Floating point register 0 (single float) */
#define PC_REGNUM 64		/* Contains program counter */
#define CAUSE_REGNUM 65
#define HI_REGNUM 66		/* Multiple/divide temp */
#define LO_REGNUM 67		/* ... */
#define FCRCS_REGNUM 68		/* FP control/status */
#define FCRIR_REGNUM 69		/* FP implementation/revision */
#define FP_REGNUM 70		/* Pseudo register that contains true address of executing stack frame */

/* Define DO_REGISTERS_INFO() to do machine-specific formatting
   of register dumps. */

#define DO_REGISTERS_INFO(_regnum) mips_do_registers_info(_regnum)

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES (4*NUM_REGS)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) ((N) * 4)

/* Number of bytes of storage in the actual machine representation
   for register N.*/

#define REGISTER_RAW_SIZE(N) 4

/* Number of bytes of storage in the program's representation
   for register N.*/

#define REGISTER_VIRTUAL_SIZE(N) 4

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 4

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 4

/* Nonzero if register N requires conversion
   from raw format to virtual format.  */

#define REGISTER_CONVERTIBLE(N) 0

/* Convert data from raw format for register REGNUM
   to virtual format for register REGNUM.  */

#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,FROM,TO)	\
{ bcopy ((FROM), (TO), REGISTER_RAW_SIZE(REGNUM)); }

/* Convert data from virtual format for register REGNUM
   to raw format for register REGNUM.  */

#define REGISTER_CONVERT_TO_RAW(REGNUM,FROM,TO)	\
{ bcopy ((FROM), (TO), REGISTER_RAW_SIZE(REGNUM)); }

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
 ((N) < FP0_REGNUM ? builtin_type_int : (N) < PC_REGNUM ? 	\
   builtin_type_float : builtin_type_int)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) SP = push_word(SP, ADDR)

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  bcopy (REGBUF+REGISTER_BYTE (TYPE_CODE (TYPE) == TYPE_CODE_FLT ? FP0_REGNUM : 2), VALBUF, TYPE_LENGTH (TYPE))

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes (REGISTER_BYTE (TYPE_CODE (TYPE) == TYPE_CODE_FLT ? FP0_REGNUM : 2), VALBUF, TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) (*(int *)((REGBUF)+16))

/* Compensate for lack of `vprintf' function.  */
#ifndef HAVE_VPRINTF
#define vprintf(format, ap) _doprnt (format, ap, stdout)
#endif /* not HAVE_VPRINTF */

/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.

   FRAME_CHAIN_COMBINE takes the chain pointer and the frame's nominal address
   and produces the nominal address of the caller frame.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.
   In that case, FRAME_CHAIN_COMBINE is not used.  */

/* In the case of the Vax, the frame's nominal address is the FP value,
   and 12 bytes later comes the saved previous FP value as a 4-byte word.  */

#define EXTRA_FRAME_INFO \
  char *proc_desc; /* actually, a PDR* */\
  int num_args;\
  struct frame_saved_regs *saved_regs;
#define INIT_EXTRA_FRAME_INFO(fci) init_extra_frame_info(fci)

#define FRAME_CHAIN(thisframe) (FRAME_ADDR)frame_chain(thisframe)

#define FRAME_CHAIN_VALID(chain, thisframe) (chain != 0)

#define FRAME_CHAIN_COMBINE(chain, thisframe) (chain)

/* Define other aspects of the stack frame.  */

/* A macro that tells us whether the function invocation represented
   by FI does not have a frame on the stack associated with it.  If it
   does not, FRAMELESS is set to 1, else 0.  */
#define FRAMELESS_FUNCTION_INVOCATION(FI, FRAMELESS)  {(FRAMELESS) = 0;}

/* Saved Pc.  */

#define FRAME_SAVED_PC(FRAME) frame_saved_pc(FRAME) 

/* If the argument is on the stack, it will be here. */
#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)

#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(numargs, fi)  (numargs = (fi)->num_args)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) ( \
  (frame_saved_regs) = *(frame_info)->saved_regs, \
  (frame_saved_regs).regs[SP_REGNUM] = (frame_info)->frame)

/* Things needed for making the inferior call functions.  */

/* Push an empty stack frame, to record the current PC, etc.  */

#define PUSH_DUMMY_FRAME {push_dummy_frame();}
#if 0
{ register CORE_ADDR sp = read_register (SP_REGNUM);\
  register int regnum;				    \
  sp = push_word (sp, 0); /* arglist */		    \
  for (regnum = 11; regnum >= 0; regnum--)	    \
    sp = push_word (sp, read_register (regnum));    \
  sp = push_word (sp, read_register (PC_REGNUM));   \
  sp = push_word (sp, read_register (FP_REGNUM));   \
  sp = push_word (sp, 0); 			    \
  write_register (SP_REGNUM, sp);		    \
  write_register (FP_REGNUM, sp);      }
#endif

/* Discard from the stack the innermost frame, restoring all registers.  */

#define POP_FRAME {pop_frame();}

#define MK_OP(op,rs,rt,offset) (((op)<<26)|((rs)<<21)|((rt)<<16)|(offset))
#define CALL_DUMMY_SIZE 48
#define Dest_Reg 2
#define CALL_DUMMY {\
 (017<<26)| (Dest_Reg << 16),	/*lui $r31,<target upper 16 bits>*/ \
 MK_OP(13,Dest_Reg,Dest_Reg,0),	/*ori $r31,$r31,<lower 16 bits>*/ \
 MK_OP(061,SP_REGNUM,12,0),	/* lwc1 $f12,0(sp) */\
 MK_OP(061,SP_REGNUM,13,4),	/* lwc1 $f13,4(sp) */\
 MK_OP(061,SP_REGNUM,14,8),	/* lwc1 $f14,8(sp) */\
 MK_OP(061,SP_REGNUM,15,12),	/* lwc1 $f15,12(sp) */\
 MK_OP(043,SP_REGNUM,4,0),	/* lw $r4,0(sp) */\
 MK_OP(043,SP_REGNUM,5,4),	/* lw $r5,4(sp) */\
 MK_OP(043,SP_REGNUM,6,8),	/* lw $r6,8(sp) */\
 (Dest_Reg<<21) | (31<<11) | 9,	/* jalr $r31 */\
 MK_OP(043,SP_REGNUM,7,12),	/* lw $r7,12(sp) */\
 13,				/* bpt */\
}

#define CALL_DUMMY_START_OFFSET 0  /* Start execution at beginning of dummy */

/* Insert the specified number of args and function address
   into a call sequence of the above form stored at DUMMYNAME.  */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, type)   \
  (*(int *)dummyname |= (((unsigned long)(fun)) >> 16), \
   ((int*)dummyname)[1] |= (unsigned short)(fun))


/* Interface definitions for kernel debugger KDB.  */

/* Map machine fault codes into signal numbers.
   First subtract 0, divide by 4, then index in a table.
   Faults for which the entry in this table is 0
   are not handled by KDB; the program's own trap handler
   gets to handle then.  */

#define FAULT_CODE_ORIGIN 0
#define FAULT_CODE_UNITS 4
#define FAULT_TABLE    \
{ 0, SIGKILL, SIGSEGV, 0, 0, 0, 0, 0, \
  0, 0, SIGTRAP, SIGTRAP, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0}

/* Start running with a stack stretching from BEG to END.
   BEG and END should be symbols meaningful to the assembler.
   This is used only for kdb.  */

#define INIT_STACK(beg, end)  \
{ asm (".globl end");         \
  asm ("movl $ end, sp");      \
  asm ("clrl fp"); }

/* Push the frame pointer register on the stack.  */
#define PUSH_FRAME_PTR        \
  asm ("pushl fp");

/* Copy the top-of-stack to the frame pointer register.  */
#define POP_FRAME_PTR  \
  asm ("movl (sp), fp");

/* After KDB is entered by a fault, push all registers
   that GDB thinks about (all NUM_REGS of them),
   so that they appear in order of ascending GDB register number.
   The fault code will be on the stack beyond the last register.  */

#define PUSH_REGISTERS   { abort(); }

/* Assuming the registers (including processor status) have been
   pushed on the stack in order of ascending GDB register number,
   restore them and return to the address in the saved PC register.  */

#define POP_REGISTERS { abort(); }
