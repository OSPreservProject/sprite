/* DevTimerT0.s --
 *
 * 	Routines for reading and writing the T0 free running timer on the
 *	Spur's Cache controller chip. 
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 *
 *
 * The routines in this file are callable from C. The C definitions are:
 *
 * extern ReturnStatus Dev_TimerT0Read(valPtr) 
 *		DevCounter	*valPtr;
 * extern ReturnStatus Dev_TimerT0Write(valPtr)
 *		DevCounter	*valPtr;
 *
 *
 *----------------------------------------------------------------------
 * _Dev_TimerT0Read --
 *
 *	Read the current value of the SPUR Cache Controller register T0.
 *
 *	Results:
 *		ReturnStatus	SUCCESS or FAILURE 
 *
 *	Side effects:
 *		Node.
 *
 *----------------------------------------------------------------------
 *
 *
 * Since the T0 register is used as a free running counter to keep track of
 * time, we can not stop it to read it. Furthermore, the register can only
 * be accessed one byte at a time. (See Wood, Eggers, Gibson : "SPUR 
 * Memory System Architecture" UCB/CSD 87/394 for details [Wood87]). 
 * In order to insure an accurate reading, we read the lower 32 bits of the
 * T0 twice and check for excessive changes. The basic algorithm used in 
 * presented in the following program:
 *
 * #include <sprite.h>
 * 
 *  *
 *  * TIMERMATCHBITS - Mask of bits of T0's lower 32 bits to be used to compare
 *  *		for a stable value.
 *  
 * #define MAXLOOPCOUNT	10
 * #define TIMERMATCHBITS	~017
 * 
 * 
 * typedef struct timer_val {
 * 	unsigned int	low;
 * 	unsigned int	high;
 * } DevCounter ;
 * 
 * ReturnStatus
 * Dev_TimerT0Read(val)
 * 	DevCounter	*val;
 * {
 * 	register unsigned int	T0low, T0high, T0low2, temp1, temp2;
 * 	unsigned int	get_timer();
 * 
 * 	T0low <-- get_timer(LOW_ORDER);
 * 	do { 
 * 		T0high <-- get_timer(HIGH_ORDER); 
 * 		T0low2 <-- get_timer(LOW_ORDER);
 * 		temp1 = T0low & TIMERMATCHBITS;
 * 		temp2 = T0low2 & TIMERMATCHBITS;
 * 		T0low = T0low2;
 * 	} while (temp1 != temp2);
 * 	val->low = T0low;
 * 	val->high = T0high;
 * 	return SUCCESS;
 * }
 */
	.text
	.align 2
	.globl _Dev_TimerT0Read

 /*
  * Register assigment: 
  * 	Local registers:
  *		T0LOW	- First value of T0<31:0>
  *		T0HIGH  - First value of T0<63:32>
  *		T0LOW2	- Second value of T0<31:0>
  *		TEMP	_ Temp register and masked T0LOW
  *		TEMP1	- Masked T0LOW2
  *	Inputs:
  *		VALPTR  - Address of TimerT0Value, Input argument.
  *	Outputs:
  *		RESULT	- Function result return register.
  */
#define	VALPTR	r11
#define	RESULT	r11
#define	T0LOW	r12
#define	T0HIGH	r13
#define	T0LOW2	r14
#define	TEMP	r15
#define	TEMP1	r16

 /*
  * TIMERMASKBITS - a 14 bit mask (sign extented to form a 32bit mask) that is
  * used to select the bits that must remain constant on consecutive reads of
  * T0<31:0> in order the read to declared valid.  T0 is reportly updated
  * every clock cycle so masking out too few bits will cause this routine
  * to hang.  Masking out too many bits will cause the timer to possbly be
  * discontinous.
  *
  */
#define	TIMERMASKBITS	0xffffff00

/*
 * Make it into a 14bit signed constant.
 */
#define	TIMERMASKBITS14BIT	-((~TIMERMASKBITS)+1)

#define	SUCCESS	0

_Dev_TimerT0Read:

	/* 
	 * T0LOW <- T0<31:0> a byte at a time.
	 */
	ld_external	T0LOW, r0, $T00|RDREG
	ld_external	TEMP, r0, $T01|RDREG
	wr_insert	$1
	insert		T0LOW, T0LOW, TEMP
	ld_external	TEMP, r0,  $T02|RDREG
	wr_insert	$2
	insert		T0LOW, T0LOW, TEMP
	ld_external	TEMP, r0,  $T03|RDREG
	wr_insert	$3
	insert		T0LOW, T0LOW, TEMP
ReadTimerT0ReadAgain:
	/* 
	 * T0HIGH <- T0<63:32> a byte at a time.
	 */
	ld_external	T0HIGH, r0, $T04|RDREG
	ld_external	TEMP, r0, $T05|RDREG
	wr_insert	$1
	insert		T0HIGH, T0HIGH, TEMP
	ld_external	TEMP, r0,  $T06|RDREG
	wr_insert	$2
	insert		T0HIGH, T0HIGH, TEMP
	ld_external	TEMP, r0,  $T07|RDREG
	wr_insert	$3
	insert		T0HIGH, T0HIGH, TEMP
	/* 
	 * T0LOW2 <- T0<31:0> a byte at a time. 
	 */
	ld_external	T0LOW2, r0, $T00|RDREG
	ld_external	TEMP, r0, $T01|RDREG
	wr_insert	$1
	insert		T0LOW2, T0LOW2, TEMP
	ld_external	TEMP, r0,  $T02|RDREG
	wr_insert	$2
	insert		T0LOW2, T0LOW2, TEMP
	ld_external	TEMP, r0,  $T03|RDREG
	wr_insert	$3
	insert		T0LOW2, T0LOW2, TEMP
	/*
	 * Mask in TEMP and TEMP2 the bits of the two readings of T0<31:0>. 
	 * If ((LT0LOW2 & TIMERMASKBITS) != (T0LOW1 & TIMERMASKBITS)) { 
	 *		- The timer changed too much for our liking.  Try 
	 *		- reading the timer over again and hope for the 
	 *		- best. Since we have a "fresh" T0<31:0> in T0LOW2 
	 *		- we use it as a starting value and transfer it into
	 *		- T0LOW and loop and try for a better read. 
	 *  } else {
	 *		- We got a good read.  Return it to the caller. 
	 *		- We will use the second read, "T0LOW2", because
	 *		- it is new than T0LOW.
	 *  }
	 */
	and		TEMP, T0LOW, $TIMERMASKBITS14BIT
	and		TEMP1, T0LOW2, $TIMERMASKBITS14BIT
	cmp_br_delayed	ne, TEMP, TEMP1, ReadTimerT0ReadAgain
	add_nt		T0LOW, r0, T0LOW2

	/*
	 * We have a good value in T0LOW and T0HIGH. Store it thru the 
	 * pointer argument VALPTR.
	 */
	st_32		T0LOW, VALPTR, $0
	st_32		T0HIGH, VALPTR, $4
	/*
	 * Return the integer of SUCCESS
	 */
	 add_nt		RESULT, r0, $SUCCESS
	 return		r10,$8
	 nop

/*
 *----------------------------------------------------------------------
 * _Dev_TimerT0Write --
 *
 *	Write the SPUR Cache Controller register T0. For best results 
 *	interrupts should be disabled before calling this routine and 
 *	the timer turned off.
 *
 *	Results:
 *		ReturnStatus	SUCCESS or FAILURE 
 *
 *	Side effects:
 *		The timer is updated.
 *
 *----------------------------------------------------------------------
 */
	.text
	.align 2
	.globl _Dev_TimerT0Write
 /* 	
  *	ReturnStatus
  *	Dev_TimerT0Write(val)
  * 		DevCounter	*val;
  * 
  */
_Dev_TimerT0Write: 

 /*
  * Register assigment: 
  * 	Local registers:
  *		TEMP	_ Temp register
  *		HALF	- 32 bit half of new value to write.
  *	Inputs:
  *		VALPTR  - Address of DevCounter value.
  *	Outputs:
  *		RESULT	- Function result return register.
  */
#define	VALPTR	r11
#define	RESULT	r11
#define	HALF	r12
#define	TEMP	r15
	/*
	 * Write register high order bytes first. 
	 */
	ld_32		HALF, VALPTR, $4
	wr_insert	$3
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T07|WRREG
	wr_insert	$2
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T06|WRREG
	wr_insert	$1
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T05|WRREG
	st_external	HALF, r0, $T04|WRREG
	/*
	 * Then write low order bytes.
	 */
	ld_32		HALF, VALPTR, r0
	wr_insert	$3
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T03|WRREG
	wr_insert	$2
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T02|WRREG
	wr_insert	$1
	extract		TEMP, HALF, HALF
	st_external	TEMP, r0, $T01|WRREG
	st_external	HALF, r0, $T00|WRREG
	add_nt		RESULT, r0, $STATUS
	return		r10,$8
	nop

