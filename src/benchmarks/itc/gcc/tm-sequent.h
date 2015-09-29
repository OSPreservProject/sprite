/* Definitions of target machine for GNU compiler.  SEQUENT NS32000 version.
   Copyright (C) 1987 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@mcc.com)

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

/* Two flags to control how addresses are printed in assembler insns.  */
#define SEQUENT_ADDRESS_BUG 1
#define SEQUENT_BASE_REGS

#include "tm-ns32k.h"

/* This is BSD, so it wants DBX format.  */
#define DBX_DEBUGGING_INFO

/* Sequent has some changes in the format of DBX symbols.  */
#define DBX_NO_XREFS 1

/* Don't split DBX symbols into continuations.  */
#define DBX_CONTIN_LENGTH 0

#define TARGET_DEFAULT 1

#undef TARGET_VERSION
#undef CPP_PREDEFINES
#undef PRINT_OPERAND
#undef PRINT_OPERAND_ADDRESS

/* Print subsidiary information on the compiler version in use.  */
#define TARGET_VERSION printf (" (32000, Sequent syntax)");

#define CPP_PREDEFINES "-Dns32000 -Dsequent -Dunix"

/* %$ means print the prefix for an immediate operand.
   On the sequent, no prefix is used for such.  */

#define PRINT_OPERAND(FILE, X, CODE)  \
{ if (CODE == '$') ;							\
  else if (CODE == '?');						\
  else if (GET_CODE (X) == REG)						\
    fprintf (FILE, "%s", reg_name [REGNO (X)]);				\
  else if (GET_CODE (X) == MEM)						\
    {									\
      rtx xfoo;								\
      xfoo = XEXP (X, 0);						\
      switch (GET_CODE (xfoo))						\
	{								\
	case MEM:							\
	  if (GET_CODE (XEXP (xfoo, 0)) == REG)				\
	    if (REGNO (XEXP (xfoo, 0)) == STACK_POINTER_REGNUM)		\
	      fprintf (FILE, "0(0(sp))");				\
	    else fprintf (FILE, "0(0(%s))",				\
			  reg_name [REGNO (XEXP (xfoo, 0))]);		\
	  else								\
	    {								\
	      fprintf (FILE, "0(");					\
	      output_address (xfoo);					\
	      putc (')', FILE);						\
	    }								\
	  break;							\
	case REG:							\
	  fprintf (FILE, "0(%s)", reg_name [REGNO (xfoo)]);		\
	  break;							\
	case PRE_DEC:							\
	case POST_INC:							\
	  fprintf (FILE, "tos");					\
	  break;							\
	case CONST_INT:							\
	  fprintf (FILE, "@%d", INTVAL (xfoo));				\
	  break;							\
	default:							\
	  output_address (xfoo);					\
	  break;							\
	}								\
    }									\
  else if (GET_CODE (X) == CONST_DOUBLE)				\
    if (GET_MODE (X) == DFmode)						\
      { union { double d; int i[2]; } u;				\
	u.i[0] = XINT (X, 0); u.i[1] = XINT (X, 1);			\
	fprintf (FILE, "0d%.20e", u.d); }				\
    else { union { double d; int i[2]; } u;				\
	   u.i[0] = XINT (X, 0); u.i[1] = XINT (X, 1);			\
	   fprintf (FILE, "0f%.20e", u.d); }				\
  else output_addr_const (FILE, X); }

#define PRINT_OPERAND_ADDRESS(FILE, ADDR)  print_operand_address(FILE, ADDR)
