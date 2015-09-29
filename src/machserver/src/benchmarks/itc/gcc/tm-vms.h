/* Output variables, constants and external declarations, for GNU compiler.
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

#include "tm-vax.h"

#undef CPP_PREDEFINES
#undef TARGET_VERSION
#undef TARGET_DEFAULT
#undef CALL_USED_REGISTERS
#undef MAYBE_VMS_FUNCTION_PROLOGUE

/* Predefine this in CPP because VMS limits the size of command options
   and GNU CPP is not used on VMS except with GNU C.  */
#define CPP_PREDEFINES "-Dvax -Dvms -DVMS -D__GNU__ -D__GNUC__"

/* By default, allow $ to be part of an identifier.  */
#define DOLLARS_IN_IDENTIFIERS 1

#define TARGET_DEFAULT 1
#define TARGET_VERSION printf ("(vax vms)");

#define CALL_USED_REGISTERS {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}

#define MAYBE_VMS_FUNCTION_PROLOGUE(FILE)	\
{ extern char *current_function_name;		\
  if (!strcmp ("main", current_function_name))	\
    fprintf(FILE, "\tjsb _c$main_args\n"); }

#define ASM_OUTPUT_EXTERNAL(FILE,DECL,NAME)		\
{ if (DECL_INITIAL (DECL) == 0 && TREE_CODE (DECL) != FUNCTION_DECL)	\
    fprintf (FILE, ".comm _%s,0\n", NAME); }
