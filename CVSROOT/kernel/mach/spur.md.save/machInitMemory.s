
#include "reg.h"
#include "spurMem.h"
#include "machConst.h"




#define BASE_REG  SAFE_TEMP1
#define CONTROL_REG  SAFE_TEMP2  
#define CONSTANT_REG SAFE_TEMP3


/*
**
**	The main memory boards have an initialization sequence that must
**	be executed.  This should be done on powerup reset.
**      
*/



mem_config_base: .long	MEM_CONFIG_BASE
big_cntrl_base:	 .long	BIG_CONTROL 
hmeg_cntrl_base: .long	HMEG_CONTROL 

		           /* Memory board sizes in bytes */
size_half:      .long   0x80000
size_8:  	.long   0x800000
size_16:   	.long   0x1000000
size_32:  	.long   0x2000000




/*
**	        (void) MachMemBoardSize (slot number);
*/

	.globl  _MachMemBoardSize         
_MachMemBoardSize:

lbase:	rd_special CONSTANT_REG, pc
	
				/*  Generate NuBus address from
				    slot number  */

	or	VOL_TEMP1,INPUT_REG1,$0xf0
	wr_insert $3
	insert  BASE_REG,r0,VOL_TEMP1   /* Base address of card */

	ld_32   CONTROL_REG,CONSTANT_REG,$mem_config_base-lbase
	nop
	add_nt  CONTROL_REG,CONTROL_REG,BASE_REG
	rd_kpsw	SAFE_TEMP1
	and	VOL_TEMP1,SAFE_TEMP1,$~(MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw	VOL_TEMP1,$0

				/* Exit if not a memory card */
	ld_32  VOL_TEMP1,CONTROL_REG,$MEM_LABEL
	add_nt VOL_TEMP2,r0,$0x4d                     /* 'M' */
	and    VOL_TEMP1,VOL_TEMP1,$0x7f  
	cmp_br_delayed ne,VOL_TEMP1,VOL_TEMP2,unknown
	ld_32  VOL_TEMP1,CONTROL_REG,$MEM_LABEL+4
	add_nt VOL_TEMP2,r0,$0x45                     /* 'E' */
	and    VOL_TEMP1,VOL_TEMP1,$0x7f  
	cmp_br_delayed ne,VOL_TEMP1,VOL_TEMP2,unknown
	ld_32  VOL_TEMP1,CONTROL_REG,$MEM_LABEL+8
	add_nt VOL_TEMP2,r0,$0x4d                     /* 'M' */
	and    VOL_TEMP1,VOL_TEMP1,$0x7f  
	cmp_br_delayed ne,VOL_TEMP1,VOL_TEMP2,unknown

				/* Find out what kind  */

	ld_32  VOL_TEMP1,CONTROL_REG,$MEM_SIZE
	wr_kpsw	SAFE_TEMP1,$0
	and    VOL_TEMP1,VOL_TEMP1,$0xff
	cmp_br_delayed eq,VOL_TEMP1,$HMEG_SIZE,ret
	ld_32  INPUT_REG1,CONSTANT_REG,$size_half-lbase	
	cmp_br_delayed eq,VOL_TEMP1,$MEG8_SIZE,ret
	ld_32  INPUT_REG1,CONSTANT_REG,$size_8-lbase
				/* These go to unknown for now
                                   because the code to handle
	                           them is more complex.  */
	cmp_br_delayed eq,VOL_TEMP1,$MEG16_SIZE,ret
	ld_32  INPUT_REG1,CONSTANT_REG,$size_16-lbase
	cmp_br_delayed eq,VOL_TEMP1,$MEG32_SIZE,ret
	ld_32  INPUT_REG1,CONSTANT_REG,$size_32-lbase

unknown:		   
	add_nt 	INPUT_REG1,r0,$-1
ret:
	return	r10, $8
	Nop


	.globl  _MachZeroMemBoard
_MachZeroMemBoard:
	add_nt VOL_TEMP2,INPUT_REG2,$0  /* Ending address */
	add_nt VOL_TEMP3,INPUT_REG1,$0  /* Starting address */
	rd_kpsw	VOL_TEMP1
	and	SAFE_TEMP1,VOL_TEMP1,$~(MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_INTR_TRAP_ENA)
@loop:
	wr_kpsw	SAFE_TEMP1,$0
	st_32	r0,VOL_TEMP3,$0
	st_32	r0,VOL_TEMP3,$4
	st_32	r0,VOL_TEMP3,$8
	st_32	r0,VOL_TEMP3,$12
	st_32	r0,VOL_TEMP3,$16
	st_32	r0,VOL_TEMP3,$20
	st_32	r0,VOL_TEMP3,$24
	st_32	r0,VOL_TEMP3,$28

	wr_kpsw	VOL_TEMP1,$0
	add_nt	VOL_TEMP3, VOL_TEMP3, $32
	cmp_br_delayed	ult, VOL_TEMP3, VOL_TEMP2, @loopb
	nop
	return	r10, $8
	Nop
