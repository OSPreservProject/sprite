/*
**	Values and addresses for initializing and maintaining
**	the NuBus memory boards.
**
*/


/*
** This is a displacment from the NuBus address to
** the memory configuration area.  
*/

#define MEM_CONFIG_BASE 0x00ffff00

#define MEM_LABEL       0x84    /* Board label "MEM"  */

#define MEM_SIZE	0x90    /* Memory size byte */
#define HMEG_SIZE       0x00    /* Half megabyte boards */
#define MEG8_SIZE       0x1d    /* 8 megabytes */
#define MEG16_SIZE      0x1e    /* 16 megabytes */
#define MEG32_SIZE      0x1f    /* 32 megabytes */



#define HMEG_CONTROL     0x00ffdf00  /* Half meg configuration address */


				/* offsets from config base*/
#define HMEG_CONFIG	0xe0	/* Config register */
#define HMEG_DISABLE_ECC 0x04
#define HMEG_ENABLE_ECC	 0x08


#define HMEG_CNTRL	0xe5	/* Control register */
#define HMEG_CLR_LED	0x0

/*
** This is a displacment from the NuBus address to
** the memory control area.  
*/

#define BIG_CONTROL     0x00ffc000  /* Big memory configuration address */

				/* Offsets from configuration address */
#define BIG_CONFIG     0x00
#define BIG_CLR_LED	0x0

#define BIG_BASE       0x10   

#define MEM_ERROR       0x10    /* Memory init error code, displayed 
                                   with slot number */












