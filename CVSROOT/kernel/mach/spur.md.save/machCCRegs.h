/*
 * machCCRegs.h --
 *
 *	Declarations of routines for reading and writting SPUR Cache Controller
 *	registers.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _HEADER
#define _HEADER

#include "machConst.h"

/*
 * machCCRegs provides a C language callable interface to reading and 
 * writing the register in the SPUR Cacher Controller.  Users of these 
 * routines should consult with the SPUR-MSA (see reference [Wood87] in
 * machCCRegs.h) before using these routines.  For best results, interrupts
 * should be disabled while read and almost certainly should be disabled 
 * while writing. The routines assume that the registers are not being
 * modified while being read. This imples that the performance counters and
 * timers should be turned off before being accessed. Also note that some
 * registers, such as the Istatus register, behave differently from normal
 * memory.  For example, a write to the Istatus registers ands the ones
 * complement of the data with the old values of the register to get the 
 * new value.  
 * 	Routines:
 *
 *	unsigned int RegValue = Mach_Read32bitCCReg(register_address)
 *			unsigned int register_address;
 *
 *	Read a 32bit CacheController register starting at the specified 
 *	address. register_address is an integer with bits <12:8> specifying
 *	the the low order byte of register to read. Possble register_address 
 *	are the low order byte of T1, T2, IStatus, IMask, FEStatus, 
 *	C0, C1, ..., C15. For example, o read the value of the IMask 
 *	register call Mach_Read32bitCCReg(MACH_INTR_STATUS_0)
 *
 *	void Mach_Write8bitCCReg(register_address,RegValue)
 *			unsigned int register_address, RegValue;
 *	Write a 32bit CachController register starting at the specified 
 * 	address. register_address is the same as Mach_Read32bitCCReg and
 *	RegValue is the value to write to the register.
 *
 *	unsigned int RegValue = Mach_Read8bitCCReg(register_address)
 *			unsigned int register_address;
 *
 *	Read a 8bit CacheController register starting at the specified 
 *	address. register_address is an integer with bits <12:8> specifying
 *	the the byte of register to read. Possble register_address 
 *	Mode, SlotId, or one byte of number of registers.
 *
 *	void Mach_Write8bitCCReg(register_address,RegValue)
 *			unsigned int register_address, RegValue;
 *	Write a 8bit CachController register starting at the specified 
 * 	address. register_address is the same as Mach_Read8bitCCReg and
 *	RegValue contains in the low order byte value to write to the register.
 *
 */

/* procedures */

extern unsigned int 	Mach_Read32bitCCReg();
extern void 	Mach_Write32bitCCReg();
extern unsigned int 	Mach_Read8bitCCReg();
extern void 	Mach_Write8bitCCReg();

#endif _HEADER

