/* Definitions of target machine for GNU compiler.  ISI 68000/68020 version.
   Intended only for use with GAS, and not ISI's assembler, which is buggy
   Copyright (C) 1988 Free Software Foundation, Inc.

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

#include "tm-m68k.h"

/* See tm-m68k.h.  7 means 68020 with 68881. */

#define TARGET_DEFAULT 7

/* Define __HAVE_FPU__ in preprocessor, unless -msoft-float is specified.
   This will control the use of inline 68881 insns in certain macros.  */

#define CPP_SPEC "%{!msoft-float:-D__HAVE_FPU__}"

/* If the 68881 is used, link must load libmc.a instead of libc.a */

#define LIB_SPEC "%{msoft-float:%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}}%{!msoft-float:%{!p:%{!pg:-lmc}}%{p:-lmc_p}%{pg:-lmc_p}}"

/* Names to predefine in the preprocessor for this target machine.  */

#define CPP_PREDEFINES "-Dunix -Dmc68000 -Dis68k"

/* This is BSD, so it wants DBX format.  */

#define DBX_DEBUGGING_INFO

/* Override parts of tm-m68000.h to fit the ISI 68k machine.  */

#undef FUNCTION_VALUE
#undef LIBCALL_VALUE
#undef FUNCTION_VALUE_REGNO_P
#undef ASM_FILE_START

/* If TARGET_68881, return SF and DF values in f0 instead of d0.  */

#define FUNCTION_VALUE(VALTYPE,FUNC) LIBCALL_VALUE (TYPE_MODE (VALTYPE))

#define LIBCALL_VALUE(MODE) \
 gen_rtx (REG, (MODE), ((TARGET_68881 && ((MODE) == SFmode || (MODE) == DFmode)) ? 16 : 0))

/* 1 if N is a possible register number for a function value.
   D0 may be used, and F0 as well if -m68881 is specified.  */

#define FUNCTION_VALUE_REGNO_P(N) \
 ((N) == 0 || (TARGET_68881 && (N) == 16))

/* Also output something to cause the correct _doprnt to be loaded.  */
#define ASM_FILE_START(FILE) fprintf (FILE, "#NO_APP\n.globl fltused\n")
