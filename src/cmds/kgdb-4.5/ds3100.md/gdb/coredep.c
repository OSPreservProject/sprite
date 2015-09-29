/* Extract registers from a "standard" core file, for GDB.
   Copyright (C) 1988-1991  Free Software Foundation, Inc.

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

/* core.c is supposed to be the more machine-independent aspects of this;
   this file is more machine-specific.  */

#include "defs.h"
#include <sys/types.h>
#include <sys/param.h>
#include "gdbcore.h"

/* These are needed on various systems to expand REGISTER_U_ADDR.  */
#ifndef USG
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/user.h>
#include "ptrace.h"
#endif
#include <sprite.h>
#include <kernel/ds3100.md/machTypes.h>

/* Extract the register values out of the core file and store
   them where `read_register' will find them.

   CORE_REG_SECT points to the register values themselves, read into memory.
   CORE_REG_SIZE is the size of that area.
   WHICH says which set of registers we are handling (0 = int, 2 = float
         on machines where they are discontiguous).
   REG_ADDR is the offset from u.u_ar0 to the register values relative to
            core_reg_sect.  This is used with old-fashioned core files to
	    locate the registers in a large upage-plus-stack ".reg" section.
	    Original upage address X is at location core_reg_sect+x+reg_addr.
 */

extern char registers[];

void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned reg_addr;
{
  register int regno;
  register unsigned int addr;
  int bad_reg = -1;
  int i;

#define gregs ((Mach_RegState *)core_reg_sect)
  bcopy (gregs->regs,
	 &registers[REGISTER_BYTE (ZERO_REGNUM)],
	 32 * REGISTER_RAW_SIZE (ZERO_REGNUM));
  bcopy (gregs->fpRegs,
	 &registers[REGISTER_BYTE (FP0_REGNUM)],
	 32 * REGISTER_RAW_SIZE (ZERO_REGNUM));
  
  *(int *)&registers[REGISTER_BYTE (HI_REGNUM)] = gregs->mfhi;
  *(int *)&registers[REGISTER_BYTE (LO_REGNUM)] = gregs->mflo;
  *(int *)&registers[REGISTER_BYTE (PC_REGNUM)] = gregs->pc;
  *(int *)&registers[REGISTER_BYTE (FCRCS_REGNUM)] = gregs->fpStatusReg;
  *(int *)&registers[REGISTER_BYTE (FP_REGNUM)] = gregs->regs[29];
  for (i = 0; i < NUM_REGS; i++) {
    supply_register (i, &registers[REGISTER_BYTE(i)]);
  }
}


#ifdef REGISTER_U_ADDR

/* Return the address in the core dump or inferior of register REGNO.
   BLOCKEND is the address of the end of the user structure.  */

unsigned int
register_addr (regno, blockend)
     int regno;
     int blockend;
{
  int addr;

  if (regno < 0 || regno >= NUM_REGS)
    error ("Invalid register number %d.", regno);

  REGISTER_U_ADDR (addr, blockend, regno);

  return addr;
}

#endif /* REGISTER_U_ADDR */
