#include "tm-m68k.h"

/* See tm-m68k.h.  0 means 68000 with no 68881.  */

#define TARGET_DEFAULT 0

/* -m68020 requires special flags to the assembler.  */

#define ASM_SPEC "%{m68020:-mc68020}%{!m68020:-mc68010}"

/* Names to predefine in the preprocessor for this target machine.  */

#define CPP_PREDEFINES "-Dmc68000 -Dsun -Dunix"

/* Alignment of field after `int : 0' in a structure.  */

#undef EMPTY_FIELD_BOUNDARY
#define EMPTY_FIELD_BOUNDARY 16

/* Every structure or union's size must be a multiple of 2 bytes.  */

#define STRUCTURE_SIZE_BOUNDARY 16

/* This is BSD, so it wants DBX format.  */

#define DBX_DEBUGGING_INFO
