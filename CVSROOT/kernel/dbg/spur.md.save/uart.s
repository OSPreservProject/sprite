/*
**  Copyright (C) 1988 by Douglas Johnson
**
**  This code implements some C callable routines to talk to the SPUR 
**  on board RS-232 port.  It is based on Garth Gibson's primitive 
**  debugger interface.  The routines implement initializing the port,
**  reading a character, and writing a character.
*/



#include "uart.h"
#include "machConst.h"
#include "reg.h"

	.asmreg    VOL_TEMP3


/* initialize the uart.  This must be called at power up.
** (void) init_uart();
*/
	.globl _uart_init
_uart_init:

	rd_kpsw VOL_TEMP1                     /* Go to physical mode to talk */
	add_nt  KPSW_REG,VOL_TEMP1,r0
	and	VOL_TEMP1,VOL_TEMP1,$~(MACH_KPSW_VIRT_DFETCH_ENA | MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw VOL_TEMP1,r0

	add_nt  VOL_TEMP1,r0,$UART_BASE          /* Start of the UART registers*/
	st_uart	($(TX_OFF|RX_OFF|RESET_RX), COMMAND)/* reset receive */
	st_uart	($(TX_OFF|RX_OFF|RESET_TX), COMMAND) /* reset transmit */
	st_uart	($(TX_OFF|RX_OFF|RESET_MR1), COMMAND)/* reset mode pointer */
	st_uart	($(TX_OFF|RX_OFF|EXIT_TX_BREAK),COMMAND)/*stop sending break */
	st_uart	($AUX_CMD_VAL, AUX_CMD )	/* mainly for show */
	st_uart	($MR1_VAL, MODE )		/* see uart.h */
	st_uart	($MR2_VAL, MODE )		/* see uart.h */
	st_uart	($BAUD_9600, BAUD )		/* set to 9600 baud */
	st_uart	($INTR_MASK_DUMB, INTR_MSK )	/* take no uart intrs */
	st_uart	($(TX_OFF|RX_OFF|CLR_ERR_STATUS), COMMAND ) /* clear errors */
	st_uart	($(TX_OFF|RX_OFF|CLR_BRK_STATUS), COMMAND )/* clear break info */
	st_uart	($(RX_ON|TX_ON),COMMAND)  /* turn on receive & transmit */


	wr_kpsw  KPSW_REG,r0
	return		r10, $8
	Nop



/* Subroutine to read a byte across the uart 
** (char) readchar();
*/

	.globl 	_readchar
_readchar:
	add_nt  VOL_TEMP1,r0,$UART_BASE        /* Start of the UART registers*/

@wait:                   /* wait for a character  */
	call	_MachRefreshCCWells	
	nop
	add_nt  OUTPUT_REG1,VOL_TEMP1,$XFER_STATUS
	call   _read_physical_word
	nop
	and     VOL_TEMP2,OUTPUT_REG1,$RX_NOT_EMPTY
	cmp_br_delayed	eq,VOL_TEMP2,r0,@waitb	
	nop
#ifdef no_cc_refresh
	cmp_trap always,r0,r0,$MACH_REFRESH_TRAP  /* Keep traps working */
#endif

				              /* Check for errors */
	and     VOL_TEMP2,VOL_TEMP2,$RXD_ERROR_MASK
	cmp_br_delayed	ne,VOL_TEMP2,r0,@errorf
	Nop

	add_nt  OUTPUT_REG1,VOL_TEMP1,$DATA
	call   _read_physical_word
	nop

	and     INPUT_REG1,OUTPUT_REG1,$0x7f         /* only seven bits */
	return		r10, $8
	Nop
				/*  Need to do something better here.  */
@error:
	jump		.
	Nop					



/*
** Subroutine to write a byte across the uart "void writechar(char)"
*/

	.globl  _writechar
_writechar:	
			    /* Start of the UART registers*/
	add_nt  VOL_TEMP1,r0,$UART_BASE   
 
@wait:			    /* Wait until there is space in the FIFO */
	call	_MachRefreshCCWells	
	nop
	add_nt  OUTPUT_REG1,VOL_TEMP1,$XFER_STATUS
	call   _read_physical_word
	nop
	and	OUTPUT_REG1,OUTPUT_REG1, $TX_NOT_FULL
	cmp_br_delayed	eq, OUTPUT_REG1, r0, @waitb
	nop
#ifdef no_cc_refresh
	cmp_trap always,r0,r0,$MACH_REFRESH_TRAP  /* Keep traps working */
#endif

			    /* Write the character  */
	add_nt  OUTPUT_REG1,VOL_TEMP1,$DATA
	and	OUTPUT_REG2,INPUT_REG1,$0x7f         /* only seven bits */
	call   _write_physical_word
	nop

	return		r10, $8
	Nop				

/*
** Subroutine to write a word into physical adddress space
** "void write_physical_word(address,data)"
*/

	.globl  _write_physical_word
_write_physical_word:	

	rd_kpsw VOL_TEMP1                    /* Go to physical mode to talk */
	add_nt  KPSW_REG,VOL_TEMP1,r0
	and	VOL_TEMP1,VOL_TEMP1,$~(MACH_KPSW_VIRT_DFETCH_ENA | MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw VOL_TEMP1,r0

	st_32   r12,r11,$0                /* Write the data */

	wr_kpsw  KPSW_REG,r0              /* Restore previous mode */
	return	r10, $8
	Nop				


/*
** Subroutine to write a word into physical memory 
** (int) read_physical_word(address)
*/

	.globl  _read_physical_word
_read_physical_word:	

	rd_kpsw VOL_TEMP1                     /* Go to physical mode to talk */
	add_nt  KPSW_REG,VOL_TEMP1,r0
	and	VOL_TEMP1,VOL_TEMP1,$~(MACH_KPSW_VIRT_DFETCH_ENA | MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw VOL_TEMP1,r0

	ld_32   r11,r11,$0                /* Read the data */

	wr_kpsw KPSW_REG,r0               /* Restore previous mode. */
	return  r10, $8
	Nop				
