/* Definitions of target machine for GNU compiler.  
   AT&T UNIX PC version (pc7300, 3b1)

   Written by Alex Crain
   bug reports to alex@umbc3.umd.edu

   Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU CC General Public
License for full details.

Everyone is granted permission to copy, modify and redistribute
GNU CC, but only under the conditions described in the
GNU CC General Public License.   A copy of this license is
supposed to have been given to you along with GNU CC so you
can know your rights and responsibilities.  It should be in a
file named COPYING.  Among other things, the copyright notice
and this notice must be preserved on all copies.  */

#define SGS_3B1

#include "tm-hp9k320.h"

/* See tm-m68k.h.  7 means 680[01]0 with no 68881.  */

#undef TARGET_DEFAULT
#define	TARGET_DEFAULT 0

/* -m68020 requires special flags to the assembler.  */

#undef ASM_SPEC
#define ASM_SPEC "%{m68020:-68020}%{!m68020:-68010}"

/* we use /lib/libp/lib*  when profiling */

#undef LIB_SPEC
#define LIB_SPEC "%{!shlib:%{p:-L/lib/libp} %{pg:-L/lib/libp} -lc}"

/* shared libraries need to use crt0s.o  */

#undef STARTFILE_SPEC
#define STARTFILE_SPEC \
  "%{!shlib:%{pg:mcrt0.o%s}%{!pg:%{p:mcrt0.o%s}%{!p:crt0.o%s}}}\
   %{shlib:crt0s.o%s shlib.ifile%s} "

/* cpp has to support a #sccs directive for the /usr/include files */

#define SCCS_DIRECTIVE

/* Names to predefine in the preprocessor for this target machine.  */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dmc68000 -Dmc68k -Dm68k -Dunix -Dunixpc"

/* Override parts of tm-m68k.h to fit the HPUX assembler.  */

#undef TARGET_VERSION
#undef ASM_OUTPUT_DOUBLE
#undef ASM_OUTPUT_FLOAT
#undef ASM_OUTPUT_ALIGN
#undef ASM_OUTPUT_SOURCE_FILENAME
#undef ASM_OUTPUT_SOURCE_LINE
#undef PRINT_OPERAND_ADDRESS
#undef ASM_OUTPUT_INTERNAL_LABEL
#undef ASM_OUTPUT_OPCODE
#undef ASM_OUTPUT_LOCAL
#undef ASM_OUTPUT_LABELREF
#undef ASM_OUTPUT_ASCII

#define TARGET_VERSION printf (" (68k, SGS/AT&T unixpc syntax)");

/* The unixpc doesn't know about double's and float's */

#define ASM_OUTPUT_DOUBLE(FILE,VALUE)  \
do { union { double d; long l[2]; } tem;		\
     tem.d = (VALUE);					\
     fprintf(FILE, "\tlong 0x%x,0x%x\n", tem.l[0], tem.l[1]); \
   } while (0)

#define ASM_OUTPUT_FLOAT(FILE,VALUE)  \
do { union { float f; long l;} tem;			\
     tem.f = (VALUE);					\
     fprintf (FILE, "\tlong 0x%x\n", tem.l);	\
   } while (0)

#define ASM_OUTPUT_ALIGN(FILE,LOG)	\
  if ((LOG) == 1)			\
    fprintf (FILE, "\teven\n");	\
  else if ((LOG) != 0)			\
    abort ();

/* The beginnings of sdb support... */

#define ASM_OUTPUT_SOURCE_FILENAME(FILE, FILENAME) \
  fprintf (FILE, "\tfile\t\"%s\"\n", FILENAME);

#define ASM_OUTPUT_SOURCE_LINE(FILE, LINENO) \
  fprintf (FILE, "\tln\t%d\n", LINENO);

/* Yet another null terminated string format. */

#define ASM_OUTPUT_ASCII(FILE,PTR,LEN) \
  { register int sp = 0, lp = 0; \
    fprintf (FILE, "\tbyte\t"); \
  loop: \
    if (PTR[sp] > ' ' && ! (PTR[sp] & 0x80) && PTR[sp] != '\\') \
      { lp += 3; \
	fprintf (FILE, "'%c", PTR[sp]); } \
    else \
      { lp += 5; \
	fprintf (FILE, "0x%x", PTR[sp]); } \
    if (++sp < LEN) \
      {	if (lp > 60) \
	  { lp = 0; \
	    fprintf (FILE, "\n%s ", ASCII_DATA_ASM_OP); }	\
	else \
	  putc (',', FILE); \
	goto loop; } \
    putc ('\n', FILE); }

/* Note that in the case of the movhi which fetches an element of
   an ADDR_DIFF_VEC the offset output is too large by 2.
   This is because the 3b1 assembler refuses to subtract 2.
   ASM_OUTPUT_CASE_LABEL, below, compensates for this.  */

#define PRINT_OPERAND_ADDRESS(FILE, ADDR)  \
{ register rtx reg1, reg2, breg, ireg;					\
  register rtx addr = ADDR;						\
  rtx offset;								\
  switch (GET_CODE (addr))						\
    {									\
    case REG:								\
      fprintf (FILE, "(%s)", reg_name [REGNO (addr)]);			\
      break;								\
    case PRE_DEC:							\
      fprintf (FILE, "-(%s)", reg_name [REGNO (XEXP (addr, 0))]);	\
      break;								\
    case POST_INC:							\
      fprintf (FILE, "(%s)+", reg_name [REGNO (XEXP (addr, 0))]);	\
      break;								\
    case PLUS:								\
      reg1 = 0;	reg2 = 0;						\
      ireg = 0;	breg = 0;						\
      offset = 0;							\
      if (CONSTANT_ADDRESS_P (XEXP (addr, 0)))				\
	{								\
	  offset = XEXP (addr, 0);					\
	  addr = XEXP (addr, 1);					\
	}								\
      else if (CONSTANT_ADDRESS_P (XEXP (addr, 1)))			\
	{								\
	  offset = XEXP (addr, 1);					\
	  addr = XEXP (addr, 0);					\
	}								\
      if (GET_CODE (addr) != PLUS) ;					\
      else if (GET_CODE (XEXP (addr, 0)) == SIGN_EXTEND)		\
	{								\
	  reg1 = XEXP (addr, 0);					\
	  addr = XEXP (addr, 1);					\
	}								\
      else if (GET_CODE (XEXP (addr, 1)) == SIGN_EXTEND)		\
	{								\
	  reg1 = XEXP (addr, 1);					\
	  addr = XEXP (addr, 0);					\
	}								\
      else if (GET_CODE (XEXP (addr, 0)) == MULT)			\
	{								\
	  reg1 = XEXP (addr, 0);					\
	  addr = XEXP (addr, 1);					\
	}								\
      else if (GET_CODE (XEXP (addr, 1)) == MULT)			\
	{								\
	  reg1 = XEXP (addr, 1);					\
	  addr = XEXP (addr, 0);					\
	}								\
      else if (GET_CODE (XEXP (addr, 0)) == REG)			\
	{								\
	  reg1 = XEXP (addr, 0);					\
	  addr = XEXP (addr, 1);					\
	}								\
      else if (GET_CODE (XEXP (addr, 1)) == REG)			\
	{								\
	  reg1 = XEXP (addr, 1);					\
	  addr = XEXP (addr, 0);					\
	}								\
      if (GET_CODE (addr) == REG || GET_CODE (addr) == MULT		\
	  || GET_CODE (addr) == SIGN_EXTEND)				\
	{ if (reg1 == 0) reg1 = addr; else reg2 = addr; addr = 0; }	\
/*  for OLD_INDEXING							\
      else if (GET_CODE (addr) == PLUS)					\
	{								\
	  if (GET_CODE (XEXP (addr, 0)) == REG)				\
	    {								\
	      reg2 = XEXP (addr, 0);					\
	      addr = XEXP (addr, 1);					\
	    }								\
	  else if (GET_CODE (XEXP (addr, 1)) == REG)			\
	    {								\
	      reg2 = XEXP (addr, 1);					\
	      addr = XEXP (addr, 0);					\
	    }								\
	}								\
  */									\
      if (offset != 0) { if (addr != 0) abort (); addr = offset; }	\
      if ((reg1 && (GET_CODE (reg1) == SIGN_EXTEND			\
		    || GET_CODE (reg1) == MULT))			\
	  || (reg2 != 0 && REGNO_OK_FOR_BASE_P (REGNO (reg2))))		\
	{ breg = reg2; ireg = reg1; }					\
      else if (reg1 != 0 && REGNO_OK_FOR_BASE_P (REGNO (reg1)))		\
	{ breg = reg1; ireg = reg2; }					\
      if (ireg != 0 && breg == 0 && GET_CODE (addr) == LABEL_REF)	\
        { int scale = 1;						\
	  if (GET_CODE (ireg) == MULT)					\
	    { scale = INTVAL (XEXP (ireg, 1));				\
	      ireg = XEXP (ireg, 0); }					\
	  if (GET_CODE (ireg) == SIGN_EXTEND)				\
	    fprintf (FILE, "L%d-LI%d(%%pc,%s.w",			\
		     CODE_LABEL_NUMBER (XEXP (addr, 0)),		\
		     CODE_LABEL_NUMBER (XEXP (addr, 0)),		\
		     reg_name[REGNO (XEXP (ireg, 0))]); 		\
	  else								\
	    fprintf (FILE, "L%d-LI%d(%%pc,%s.l",			\
		     CODE_LABEL_NUMBER (XEXP (addr, 0)),		\
		     CODE_LABEL_NUMBER (XEXP (addr, 0)),		\
		     reg_name[REGNO (ireg)]);				\
	  if (scale != 1) fprintf (FILE, ":%d", scale);			\
	  fprintf (FILE, ")");						\
	  break; }							\
      if (ireg != 0 || breg != 0)					\
	{ int scale = 1;						\
	  if (breg == 0)						\
	    abort ();							\
	  if (addr != 0)						\
	    output_addr_const (FILE, addr);				\
	  fprintf (FILE, "(%s", reg_name[REGNO (breg)]);		\
	  if (ireg != 0)					        \
	    putc (',', FILE);						\
	  if (ireg != 0 && GET_CODE (ireg) == MULT)			\
	    { scale = INTVAL (XEXP (ireg, 1));				\
	      ireg = XEXP (ireg, 0); }					\
	  if (ireg != 0 && GET_CODE (ireg) == SIGN_EXTEND)		\
	    fprintf (FILE, "%s.w", reg_name[REGNO (XEXP (ireg, 0))]);	\
	  else if (ireg != 0)						\
	    fprintf (FILE, "%s.l", reg_name[REGNO (ireg)]);		\
	  if (scale != 1) fprintf (FILE, ":%d", scale);			\
	  putc (')', FILE);						\
	  break;							\
	}								\
      else if (reg1 != 0 && GET_CODE (addr) == LABEL_REF)		\
	{ fprintf (FILE, "L%d-LI%d(%%pc,%s.w)",				\
		   CODE_LABEL_NUMBER (XEXP (addr, 0)),			\
		   CODE_LABEL_NUMBER (XEXP (addr, 0)),			\
		   reg_name[REGNO (reg1)]);				\
	  break; }							\
    default:								\
      if (GET_CODE (addr) == CONST_INT					\
	  && INTVAL (addr) < 0x8000					\
	  && INTVAL (addr) >= -0x8000)					\
	fprintf (FILE, "%d", INTVAL (addr));				\
      else								\
        output_addr_const (FILE, addr);					\
    }}

#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM)	\
    fprintf (FILE, "%s%d:\n", PREFIX, NUM)

#define ASM_OUTPUT_CASE_LABEL(FILE,PREFIX,NUM,TABLE)	\
    fprintf (FILE, "\tswbeg &%d\n%s%d:\n\tshort 0",	\
	     XVECLEN (PATTERN (TABLE), 1) + 1, PREFIX, NUM)

#define ASM_OUTPUT_OPCODE(FILE, PTR)			\
{ if ((PTR)[0] == 'j' && (PTR)[1] == 'b')		\
    { ++(PTR);						\
      while (*(PTR) != ' ')				\
	{ putc (*(PTR), (FILE)); ++(PTR); }		\
      fprintf ((FILE), ".w"); }				\
  else if ((PTR)[0] == 'f')				\
    {							\
      if (!strncmp ((PTR), "fmove", 5))			\
	{ fprintf ((FILE), "fmov"); (PTR) += 5; }	\
      else if (!strncmp ((PTR), "ftst", 4))		\
	{ fprintf ((FILE), "ftest"); (PTR) += 4; }	\
    }							\
/* MOVE, MOVEA, MOVEQ, MOVEC ==> MOV	*/		\
  else if ((PTR)[0] == 'm' && (PTR)[1] == 'o'		\
	   && (PTR)[2] == 'v' && (PTR)[3] == 'e')	\
    { fprintf ((FILE), "mov"); (PTR) += 4;		\
       if ((PTR)[0] == 'q' || (PTR)[0] == 'a' ||	\
	   (PTR)[0] == 'c') (PTR)++; }			\
/* SUB, SUBQ, SUBA, SUBI ==> SUB */			\
  else if ((PTR)[0] == 's' && (PTR)[1] == 'u' 		\
	   && (PTR)[2] == 'b')				\
    { fprintf ((FILE), "sub"); (PTR) += 3;		\
       if ((PTR)[0] == 'q' || (PTR)[0] == 'i' || 	\
	   (PTR)[0] == 'a') (PTR)++; }			\
/* CMP, CMPA, CMPI, CMPM ==> CMP	*/		\
  else if ((PTR)[0] == 'c' && (PTR)[1] == 'm'		\
	   && (PTR)[2] == 'p')				\
    { fprintf ((FILE), "cmp"); (PTR) += 3;		\
       if ((PTR)[0] == 'a' || (PTR)[0] == 'i' || 	\
	   (PTR)[0] == 'm') (PTR)++; }			\
}

#define ASM_OUTPUT_LOCAL(FILE, NAME, SIZE)  \
( fputs ("\tlcomm ", (FILE)),			\
  assemble_name ((FILE), (NAME)),		\
  fprintf ((FILE), ",%d\n", (SIZE)))

#define ASM_OUTPUT_LABELREF(FILE,NAME)	\
  fprintf (FILE, "%s", NAME)
