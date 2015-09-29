/* 
 * machCode.c --
 *
 *     C code for the mach module.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include "sprite.h"
/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel;
/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */
int mach_NumDisableInterrupts[1];
int *mach_NumDisableIntrsPtr = mach_NumDisableInterrupts;
#ifdef notdef
Mach_SetHandler() { }
ReturnStatus
Mach_Probe(size, srcAddress, destAddress)
  int               size;   
  Address     srcAddress;    
  Address     destAddress;    
{
    switch (size) {
    case 1:
	* (char *) destAddress = * ( char *) srcAddress;
	break;
    case 2:
	* (short *) destAddress = * ( short *) srcAddress;
	break;
    case 4:
	* (int *) destAddress = * ( int *) srcAddress;
	break;
    }
    return (SUCCESS);
}
#endif
