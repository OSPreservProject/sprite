|* excVector.s -
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

.globl _exc_ProtoVectorTable

| This is the prototype vector table
| which gets copied to location zero at system start up.
| The monitor's vector is preserved when zero.

_exc_ProtoVectorTable:
	.long	Exc_Reset		| 0	System stack on reset
	.long	Exc_Reset		| 1	System reset routine
	.long	Exc_BusError		| 2	Bus Error
	.long	Exc_AddrError		| 3	Address Error
	.long	Exc_IllegalInst		| 4	Illegal Instruction
	.long	Exc_ZeroDiv		| 5 	Zero divide
	.long	Exc_ChkInst		| 6	CHK instruction
	.long	Exc_Trapv		| 7	TRAPV instruction
	.long	Exc_PrivVio		| 8	Privilege violation
	.long	Exc_TraceTrap		| 9	Trace trap
	.long	Exc_Emu1010		| 10	Line 1010 emulator trap
	.long	Exc_Emu1111		| 11	Line 1111 emulator trap
	.long	0			| 12	Reserved
	.long	0			| 13	Reserved
	.long	Exc_FmtError		| 14	68010 stack format error
	.long	Exc_UninitVect		| 15	Unitialized vector
	.long	0			| 16	Reserved
	.long	0			| 17	Reserved
	.long	0			| 18	Reserved
	.long	0			| 19	Reserved
	.long	0			| 20	Reserved
	.long	0			| 21	Reserved
	.long	0			| 22	Reserved
	.long	0			| 23	Reserved
	.long	Exc_Spurious		| 24	Spurious interrupt
	.long	Exc_Level1Int		| 25	Level 1 software interrupt
	.long	Exc_Level2Int		| 26	Level 2 interrupt
	.long	Exc_Level3Int		| 27	Level 3 interrupt
	.long	Exc_Level4Int		| 28	Level 4 interrupt
	.long	Exc_Level5Int		| 29	Level 5 interrupt
	.long	Exc_Level6Int		| 30	Level 6 interrupt
	.long	0			| 31	Level 7 interrupt (REFRESH)
	.long	Exc_BadTrap		| 32	Trap instruction 0
	.long	Exc_SyscallTrap		| 33	Trap instruction 1 (System Call)
	.long	Exc_SigRetTrap		| 34	Trap instruction 2
	.long	Exc_BadTrap		| 35	Trap instruction 3
	.long	Exc_BadTrap		| 36	Trap instruction 4
	.long	Exc_BadTrap		| 37	Trap instruction 5
	.long	Exc_BadTrap		| 38	Trap instruction 6
	.long	Exc_BadTrap		| 39	Trap instruction 7
	.long	Exc_BadTrap		| 40	Trap instruction 8
	.long	Exc_BadTrap		| 41	Trap instruction 9
	.long	Exc_BadTrap		| 42	Trap instruction 10
	.long	Exc_BadTrap		| 43	Trap instruction 11
	.long	Exc_BadTrap		| 44	Trap instruction 12
	.long	Exc_BadTrap		| 45	Trap instruction 13
	.long	Exc_BadTrap		| 46	Trap instruction 14
	.long	Exc_BrkptTrap		| 47	Trap instruction 15 (debug 
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
