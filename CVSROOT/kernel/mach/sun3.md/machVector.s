|* machVector.s -
|*
|*     Contains the exception vector tables.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

.data
.asciz "$Header$ SPRITE (Berkeley)"
.even
.text

.globl _machProtoVectorTable

| This is the prototype vector table
| which gets copied to location zero at system start up.
| The monitor's vector is preserved when zero.

_machProtoVectorTable:
	.long	MachReset		| 0	System stack on reset
	.long	MachReset		| 1	System reset routine
	.long	MachBusError		| 2	Bus Error
	.long	MachAddrError		| 3	Address Error
	.long	MachIllegalInst		| 4	Illegal Instruction
	.long	MachZeroDiv		| 5 	Zero divide
	.long	MachChkInst		| 6	CHK instruction
	.long	MachTrapv		| 7	TRAPV instruction
	.long	MachPrivVio		| 8	Privilege violation
	.long	MachTraceTrap		| 9	Trace trap
	.long	MachEmu1010		| 10	Line 1010 emulator trap
	.long	MachEmu1111		| 11	Line 1111 emulator trap
	.long	0			| 12	Reserved
	.long	0			| 13	Reserved
	.long	MachFmtError		| 14	68010 stack format error
	.long	MachUninitVect		| 15	Unitialized vector
	.long	0			| 16	Reserved
	.long	0			| 17	Reserved
	.long	0			| 18	Reserved
	.long	0			| 19	Reserved
	.long	0			| 20	Reserved
	.long	0			| 21	Reserved
	.long	0			| 22	Reserved
	.long	0			| 23	Reserved
	.long	MachSpurious		| 24	Spurious interrupt
	.long	MachLevel1Int		| 25	Level 1 software interrupt
	.long	MachLevel2Int		| 26	Level 2 interrupt
	.long	MachLevel3Int		| 27	Level 3 interrupt
	.long	MachLevel4Int		| 28	Level 4 interrupt
	.long	MachLevel5Int		| 29	Level 5 interrupt
	.long	MachLevel6Int		| 30	Level 6 interrupt
	.long	0			| 31	Level 7 interrupt (REFRESH)
	.long	MachUnixSyscallTrap	| 32	Trap instruction 0
	.long	MachSyscallTrap		| 33	Trap instruction 1 (System Call)
	.long	MachSigRetTrap		| 34	Trap instruction 2
	.long	MachBadTrap		| 35	Trap instruction 3
	.long	MachBadTrap		| 36	Trap instruction 4
	.long	MachBadTrap		| 37	Trap instruction 5
	.long	MachBadTrap		| 38	Trap instruction 6
	.long	MachBadTrap		| 39	Trap instruction 7
	.long	MachBadTrap		| 40	Trap instruction 8
	.long	MachBadTrap		| 41	Trap instruction 9
	.long	MachBadTrap		| 42	Trap instruction 10
	.long	MachBadTrap		| 43	Trap instruction 11
	.long	MachBadTrap		| 44	Trap instruction 12
	.long	MachBadTrap		| 45	Trap instruction 13
	.long	MachBadTrap		| 46	Trap instruction 14
	.long	MachBrkptTrap		| 47	Trap instruction 15 (debug 
					|	breakpoint)
#ifdef sun3
	.long	MachFpUnorderedCond     | 48	Reserved
	.long	MachFpInexactResult	| 49	Reserved
	.long	MachFpZeroDiv   	| 50	Reserved
	.long	MachFpUnderflow		| 51	Reserved
	.long	MachFpOperandError	| 52	Reserved
	.long	MachFpOverflow		| 53	Reserved
	.long	MachFpNaN		| 54	Reserved
#else
	.long	0                       | 48	Reserved
	.long	0			| 49	Reserved
	.long	0			| 50	Reserved
	.long	0			| 51	Reserved
	.long	0			| 52	Reserved
	.long	0			| 53	Reserved
	.long	0			| 54	Reserved
#endif	
	.long	0			| 55	Reserved
	.long	0			| 56	Reserved
	.long	0			| 57	Reserved
	.long	0			| 58	Reserved
	.long	0			| 59	Reserved
	.long	0			| 60	Reserved
	.long	0			| 61	Reserved
	.long	0			| 62	Reserved
	.long	0			| 63	Reserved
