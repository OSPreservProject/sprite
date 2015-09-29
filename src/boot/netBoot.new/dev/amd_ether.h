
/*	@(#)amd_ether.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * amd_ether.h: Defines for the AM7990 Ethernet chip.
 */

#define	AMD_E_RDP	0		/* AMD Register DATA port           */
#define	AMD_E_RAP	AMD_E_RDP+1	/* AMD Register address select port */
					/* The +1 is +1 short int (16 bits) */
#define AMD_E_CSR0      0               /* AMD CSR0 */
#define AMD_E_CSR1      1               /* AMD CSR1 */
#define AMD_E_CSR2      2               /* AMD CSR2 */
#define AMD_E_CSR3      3               /* AMD CSR3 */
#define AMD_E_STOP      4		/* Stop Command */
#define AMD_E_START     2		/* Start Command */
#define AMD_E_INIT      1		/* Initalize Command */


#define AMD_E_INITOK    ( 1<<8 )	/* Initalize OK flag */
#define AMD_E_INT       ( 1<<7 )	/* Interrupt flag    */
#define AMD_E_INEA      ( 1<<6 )	/* Interrupt Enable  */
#define AMD_E_OWN       ( 1<<15 )	/* OWN flag          */
#define AMD_E_PROM      ( 1<<15 )	/* Promiscus mode (rcv anything) */
#define AMD_E_INTL      ( 1<<6 )	/* Internal loopback */ 
#define AMD_E_DTCR      ( 1<<3 )	/* Disable TX CRC generator */ 
#define AMD_E_LOOPBACK  ( 1<<2 )	/* Activate loopback */
#define AMD_E_STP       ( 1<<9 )	/* Activate loopback */
#define AMD_E_ENP       ( 1<<8 )	/* Activate loopback */

#define AMD_E_TXERR     ( 1<<14 )	/* Error flag in tx-descriptor */
#define AMD_E_RXERR     ( 1<<14 )	/* Error flag in rx-descriptor */
#define AMD_E_ERR       ( 1<<15 )	/* Error flag in Status word   */
#define AMD_E_BABL      ( 1<<14 )       /* Error - babbling transmitter */
#define AMD_E_MISS      ( 1<<12 )       /* Error - missed packet */
#define AMD_E_MERR      ( 1<<11 )       /* Error - memory timeout */

/*
 * END of Defines for AMD lance chip
 */



/*
 * Definitions of structures used by these tests
 */

struct amd_init		/* AMD Initilization block */
	{
	u_short mode;		/* Mode control				*/
	u_short padr_lo;	/* Lo 16 bits of physical address       */
	u_short padr_mid;	/* Mid 16 bits 				*/
	u_short padr_hi;	/* Hi 16 bits				*/
	u_short ladr_lo;	/* Lo 16 bits of logical  address       */
	u_short ladr_mid_lo;	/* Mid_lo 16 bits			*/
	u_short ladr_mid_hi;	/* Mid_hi 16 bits			*/
	u_short ladr_hi;	/* Hi 16 bits				*/
	u_short rdra_lo;	/* Lo 16 bits of pointer to discriptor  */
				/* rings for receive			*/
	u_short rdra_hi;		/* Hi 8 bits of pointer to discriptor   */
				/* rings for receive & rcv ring length	*/
	u_short tdra_lo;	/* Lo 16 bits of pointer to discriptor  */
				/* rings for txmit			*/
	u_short tdra_hi		/* Hi 8 bits of pointer to discriptor   */
				/* rings for txmit & rcv ring length	*/
	};

struct amd_rcv_ring
	{
	u_short rmd0_ladr;	/* lo address of data buffer pointed to */
	u_short rmd1_hadr;	/* Hi 8 bits of buffer & info bits      */
	u_short rmd2_bcnt;	/* size of data buffer			*/
	u_short rmd3_mcnt;	/* size of message in this buffer       */
	};

struct amd_txmt_ring
	{
	u_short tmd0_ladr;	/* lo address of data buffer pointed to */
	u_short tmd1_hadr;	/* Hi 8 bits of buffer & info bits      */
	u_short tmd2_bcnt;	/* size of data buffer			*/
	u_short tmd3_tdr;	/* size of message in this buffer       */
	};

