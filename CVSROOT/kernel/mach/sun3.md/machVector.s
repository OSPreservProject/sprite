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
	.long	Mach_Reset		| 0	System stack on reset
	.long	Mach_Reset		| 1	System reset routine
	.long	Mach_BusError		| 2	Bus Error
	.long	Mach_AddrError		| 3	Address Error
	.long	Mach_IllegalInst	| 4	Illegal Instruction
	.long	Mach_ZeroDiv		| 5 	Zero divide
	.long	Mach_ChkInst		| 6	CHK instruction
	.long	Mach_Trapv		| 7	TRAPV instruction
	.long	Mach_PrivVio		| 8	Privilege violation
	.long	Mach_TraceTrap		| 9	Trace trap
	.long	Mach_Emu1010		| 10	Line 1010 emulator trap
	.long	Mach_Emu1111		| 11	Line 1111 emulator trap
	.long	0			| 12	Reserved
	.long	0			| 13	Reserved
	.long	Mach_FmtError		| 14	68010 stack format error
	.long	Mach_UninitVect		| 15	Unitialized vector
	.long	0			| 16	Reserved
	.long	0			| 17	Reserved
	.long	0			| 18	Reserved
	.long	0			| 19	Reserved
	.long	0			| 20	Reserved
	.long	0			| 21	Reserved
	.long	0			| 22	Reserved
	.long	0			| 23	Reserved
	.long	Mach_Spurious		| 24	Spurious interrupt
	.long	Mach_Level1Int		| 25	Level 1 software interrupt
	.long	Mach_Level2Int		| 26	Level 2 interrupt
	.long	Mach_Level3Int		| 27	Level 3 interrupt
	.long	Mach_Level4Int		| 28	Level 4 interrupt
	.long	Mach_Level5Int		| 29	Level 5 interrupt
	.long	Mach_Level6Int		| 30	Level 6 interrupt
	.long	0			| 31	Level 7 interrupt (REFRESH)
	.long	Mach_BadTrap		| 32	Trap instruction 0
	.long	Mach_SyscallTrap	| 33	Trap instruction 1 (System Call)
	.long	Mach_SigRetTrap		| 34	Trap instruction 2
	.long	Mach_BadTrap		| 35	Trap instruction 3
	.long	Mach_BadTrap		| 36	Trap instruction 4
	.long	Mach_BadTrap		| 37	Trap instruction 5
	.long	Mach_BadTrap		| 38	Trap instruction 6
	.long	Mach_BadTrap		| 39	Trap instruction 7
	.long	Mach_BadTrap		| 40	Trap instruction 8
	.long	Mach_BadTrap		| 41	Trap instruction 9
	.long	Mach_BadTrap		| 42	Trap instruction 10
	.long	Mach_BadTrap		| 43	Trap instruction 11
	.long	Mach_BadTrap		| 44	Trap instruction 12
	.long	Mach_BadTrap		| 45	Trap instruction 13
	.long	Mach_BadTrap		| 46	Trap instruction 14
	.long	Mach_BrkptTrap		| 47	Trap instruction 15 (debug 
					|	breakpoint)
	.long	0			| 48	Reserved
	.long	0			| 49	Reserved
	.long	0			| 50	Reserved
	.long	0			| 51	Reserved
	.long	0			| 52	Reserved
	.long	0			| 53	Reserved
	.long	0			| 54	Reserved
	.long	0			| 55	Reserved
	.long	0			| 56	Reserved
	.long	0			| 57	Reserved
	.long	0			| 58	Reserved
	.long	0			| 59	Reserved
	.long	0			| 60	Reserved
	.long	0			| 61	Reserved
	.long	0			| 62	Reserved
	.long	0			| 63	Reserved
