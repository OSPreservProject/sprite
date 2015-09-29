
/*
 *      @(#)devio.h	4.1.1.8	(ULTRIX)	9/17/88
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986-1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * devio.h
 *
 * Modification history
 *
 * Common structures and definitions for device drivers and ioctl
 *
 * 17-Sep-88 - Ricky Palmer
 *
 *	Updated PMAX SCSI support.
 *
 * 01-Aug-88 - Ricky Palmer
 *
 *	Added PMAX SCSI support.
 *
 * 17-Apr-88 - Ricky Palmer
 *
 *	Added DEV_DSSC and DEV_MSI for MSI support.
 *
 * 20-Feb-88 - Tim Burke
 *
 *      Added DEV_DHB32, a 16 line BI bus terminal mux.
 *
 * 28-Sep-87 - Ricky Palmer
 *
 *	Added new "rctlr_num" field to "devget" structure.
 *
 * 17-May-87 - Ricky Palmer
 *
 *	Updated field for tu78/ta78.
 *
 * 19-Mar-87 -- Fred Canter
 *
 *	Added DEV_XOS for X in kernel special device.
 *
 * 10-Mar-87 - rsp (Ricky Palmer)
 *
 *	Added defines for cx series of controllers, updated dh defines.
 *
 * 14-Jan-87 - Robin
 *
 *	Added rqdx4, rd35
 *
 *  6-Jan-87 -	Fred Canter
 *
 *	Minor changes to some comments.
 *
 *  4-Mar-86 -	Ricky Palmer
 *
 *	Created original file and its contents. V2.0
 *
 * 13-Jun-86 - Jim Woodward
 *
 *	Fix to uba reset and drivers.
 *
 * 11-Jul-86 - Ricky Palmer
 *
 *	Added adpt, nexus fields to basic devget structure. V2.0
 *
 *  5-Aug-86 - Fred Canter
 *
 *	Added defines needed for devioctl support in VAXstation
 *	2000 device drivers.
 *	Changed RD3X to RD32.
 *
 * 6-Aug-86 - Robin Lewis
 *
 *	Added tape density for tk70 and added device entries for
 *	tk70, ra70, ra90, rv80, tu82
 *
 * 7-Aug-86 - Ricky Palmer
 *
 *	Added defines for VT3?? series of terminals. V2.0
 *
 * 27-Aug-86 -- Fred Canter
 *	Bug fix: removed the comma after DEV_MOUSE and DEV_TABLET.
 */

/* Basic amount of storage for "interface" and "device" below */
#define DEV_SIZE	0x08		/* Eight bytes			*/

/* DEV_UGH uprintf macro for driver backward compatibility */
#define DEV_UGH(x,y,z)	uprintf("%s: unit# %d: %s\n",x,y,z)

/* Structure for DEVIOCGET ioctl - device get status command */
struct	devget	{
	short	category;		/* Category			*/
	short	bus;			/* Bus				*/
	char	interface[DEV_SIZE];	/* Interface (string)		*/
	char	device[DEV_SIZE];	/* Device (string)		*/
	short	adpt_num;		/* Adapter number		*/
	short	nexus_num;		/* Nexus or node on adapter no. */
	short	bus_num;		/* Bus number			*/
	short	ctlr_num;		/* Controller number		*/
	short	rctlr_num;		/* Remote controller number	*/
	short	slave_num;		/* Plug or line number		*/
	char	dev_name[DEV_SIZE];	/* Ultrix device pneumonic	*/
	short	unit_num;		/* Ultrix device unit number	*/
	unsigned soft_count;		/* Driver soft error count	*/
	unsigned hard_count;		/* Driver hard error count	*/
	long	stat;			/* Generic status mask		*/
	long	category_stat;		/* Category specific mask	*/
};

/* Get status definitions for category word (category) */
#define DEV_TAPE	0x00		/* Tape category		*/
#define DEV_DISK	0x01		/* Disk category		*/
#define DEV_TERMINAL	0x02		/* Terminal category		*/
#define DEV_PRINTER	0x03		/* Printer category		*/
#define DEV_SPECIAL	0x04		/* Special category		*/

/* Get status definitions for bus word (bus) */
#define DEV_UB		0x00		/* Unibus bus			*/
#define DEV_QB		0x01		/* Qbus bus			*/
#define DEV_MB		0x02		/* Massbus bus			*/
#define DEV_BI		0x03		/* BI bus			*/
#define DEV_CI		0x04		/* CI bus			*/
#define DEV_NB		0x05		/* No Bus (single board VAX CPU)*/
#define DEV_MSI		0x06		/* MSI bus			*/
#define DEV_SCSI	0x07		/* SCSI bus			*/

/* Definition for any unsupported/unknown interface or device */
#define DEV_UNKNOWN	"UNKNOWN"	/* Unknown interface/device	*/

/* Definitions for interface character array (interface) */
#define DEV_TM03	"TM03"		/* TM03 tape formatter		*/
#define DEV_TM78	"TM78"		/* TM78 tape formatter		*/
#define DEV_TSU11	"TSU11" 	/* TSU11 tape controller	*/
#define DEV_TUU80	"TU80"	 	/* TU80 tape controller		*/
#define DEV_TSU05	"TSU05" 	/* TSU05 tape controller	*/
#define DEV_TSV05	"TSV05" 	/* TSV05 tape controller	*/
#define DEV_TQK50	"TQK50" 	/* TQK50 tape controller	*/
#define DEV_TUK50	"TUK50" 	/* TUK50 tape controller	*/
#define DEV_TQK70	"TQK70" 	/* TQK70 tape controller	*/
#define DEV_TUK70	"TUK70" 	/* TUK70 tape controller	*/
#define DEV_KLESI	"KLESI" 	/* KLESI disk/tape controller	*/
#define DEV_DEBNT	"DEBNT" 	/* DEBNT network/tape controller*/
#define DEV_DSSC	"DSSC"		/* DSSC MSI controller		*/
#define DEV_HSC50	"HSC50" 	/* HSC50 intelligent controller */
#define DEV_HSC70	"HSC70" 	/* HSC70 intelligent controller */
#define DEV_HSB50	"HSB50" 	/* HSB50 intelligent controller */
#define DEV_RK711	"RK711" 	/* RK711 disk controller	*/
#define DEV_RLU211	"RLU211"	/* RLU211 disk controller	*/
#define DEV_IDC 	"IDC"		/* IDC integral disk controller */
#define DEV_UDA50	"UDA50" 	/* UDA50 disk controller	*/
#define DEV_UDA50A	"UDA50A"	/* UDA50 enhanced disk cont.	*/
#define DEV_RUX50	"RUX50" 	/* RUX50 disk controller	*/
#define DEV_RH		"RH"		/* RH disk controller		*/
#define DEV_RLV211	"RLV211"	/* RLV211 disk controller	*/
#define DEV_RRD40	"RRD40"		/* RRD40 disk controller	*/
#define DEV_RRD50	"RRD50" 	/* RRD50 disk controller	*/
#define DEV_RQDX1	"RQDX1" 	/* RQDX1 disk controller	*/
#define DEV_RQDX2	"RQDX2" 	/* RQDX2 disk controller	*/
#define DEV_RQDX3	"RQDX3" 	/* RQDX3 disk controller	*/
#define DEV_RQDX4	"RQDX4" 	/* RQDX4 disk controller	*/
#define DEV_KDA50	"KDA50" 	/* KDA50 disk controller	*/
#define DEV_KDB50	"KDB50" 	/* KDB50 disk controller	*/
#define DEV_KFQSA	"KFQSA" 	/* KFQSA disk controller	*/
#define DEV_KFBTA	"KFBTA" 	/* KFBTA disk controller	*/
#define DEV_AIO 	"AIO"		/* AIO disk controller		*/
#define DEV_DZ11	"DZ11"		/* DZ11 terminal mux.		*/
#define DEV_DZ32	"DZ32"		/* DZ32 terminal mux.		*/
#define DEV_DHU11	"DHU11" 	/* DHU11 terminal mux.		*/
#define DEV_DMF32	"DMF32" 	/* DMF32 terminal mux.		*/
#define DEV_DMZ32	"DMZ32" 	/* DMZ32 terminal mux.		*/
#define DEV_DZV11	"DZV11" 	/* DZV11 terminal mux.		*/
#define DEV_DZQ11	"DZQ11" 	/* DZQ11 terminal mux.		*/
#define DEV_DHQVCXY	"DHQVCXY"	/* DH(QV)11/CXY08 terminal mux. */
#define DEV_CXAB16	"CXAB16"	/* CX(AB)16 terminal mux.	*/
#define DEV_DMB32	"DMB32" 	/* DMB32 terminal mux.		*/
#define DEV_DHB32	"DHB32"		/* DHB32 terminal mux.          */
#define DEV_VCB01	"VCB01" 	/* VCB01 workstation controller */
#define DEV_VCB02	"VCB02" 	/* VCB02 workstation controller */
#define DEV_LAT 	"LAT"		/* LAT terminal server		*/
#define DEV_SCSI_GEN	"SCSI"		/* SCSI generic string		*/
/* VAXstation 2000 device name definitions */
#define DEV_VS_SLU	"VS_SLU"	/* Serial line controller	*/
#define DEV_VS_DISK	"VS_DISK"	/* Disk controller		*/
#define DEV_VS_TAPE	"VS_TAPE"	/* TZK50 tape controller	*/
#define DEV_VS_NI	"VS_NI" 	/* Ethernet controller		*/
#define DEV_TM_SLE	"TM_SLE"	/*				*/

/* Definitions for device character array (device) */
#define DEV_TE16	"TE16"		/* TE16 tape drive		*/
#define DEV_TU45	"TU45"		/* TU45 tape drive		*/
#define DEV_TU77	"TU77"		/* TU77 tape drive		*/
#define DEV_TU78	"TU78/9"	/* TU78/TU79 tape drive 	*/
#define DEV_TS11	"TS11"		/* TS11 tape drive		*/
#define DEV_TU80	"TU80"		/* TU80 tape drive		*/
#define DEV_TS05	"TS05"		/* TS05 tape drive		*/
#define DEV_TU81	"TU81"		/* TU81 tape drive		*/
#define DEV_TU81E	"TU81E" 	/* TU81E tape drive		*/
#define DEV_TK50	"TK50"		/* TK50 tape drive		*/
#define DEV_TK70	"TK70"		/* TK70 tape drive		*/
#define DEV_TZ30	"TZ30"		/* TZ30 tape drive		*/
#define DEV_TZ88	"TZ88"		/* TZ88 tape drive		*/
#define DEV_RV20	"RV20"		/* RV20 tape drive		*/
#define DEV_TA78	"TA78/9"	/* TA78/TA79 tape drive 	*/
#define DEV_TA81	"TA81"		/* TA81 tape drive		*/
#define DEV_TA79	"TA79"		/* TA79 tape drive		*/
#define DEV_TA90	"TA90"		/* TA90 tape drive		*/
#define DEV_RV60	"RV60"		/* RV60 tape drive		*/
#define DEV_SVS00	"SVS00"		/* SVS00 tape drive		*/
#define DEV_ESE20	"ESE20"		/* ESE20 disk drive		*/
#define DEV_RK07	"RK07"		/* RK07 disk drive		*/
#define DEV_RL02	"RL02"		/* RL02 disk drive		*/
#define DEV_R80 	"R80"		/* R80 disk drive		*/
#define DEV_RA60	"RA60"		/* RA60 disk drive		*/
#define DEV_RA80	"RA80"		/* RA80 disk drive		*/
#define DEV_RA81	"RA81"		/* RA81 disk drive		*/
#define DEV_RA82	"RA82"		/* RA82 disk drive		*/
#define DEV_RA70	"RA70"		/* RA70 disk drive		*/
#define DEV_RA90	"RA90"		/* RA90 disk drive		*/
#define DEV_RC25	"RC25"		/* RC25 disk drive		*/
#define DEV_RC25F	"RC25F"		/* RC25 fixed disk drive	*/
#define DEV_RD31	"RD31"		/* RD31 disk drive		*/
#define DEV_RD32	"RD32"		/* RD32 disk drive		*/
#define DEV_RD33	"RD33"		/* RD33 disk drive		*/
#define DEV_RD51	"RD51"		/* RD51 disk drive		*/
#define DEV_RD52	"RD52"		/* RD52 disk drive		*/
#define DEV_RD53	"RD53"		/* RD53 disk drive		*/
#define DEV_RF30	"RF30"		/* RF30 disk drive		*/
#define DEV_RF71	"RF71"		/* RF71 disk drive		*/
#define DEV_RD54	"RD54"		/* RD54 disk drive		*/
#define DEV_RX18	"RX18"		/* RX18 disk drive		*/
#define DEV_RX33	"RX33"		/* RX33 disk drive		*/
#define DEV_RX35	"RX35"		/* RX33 disk drive		*/
#define DEV_RX50	"RX50"		/* RX50 disk drive		*/
#define DEV_RZ22	"RZ22"		/* RZ22 disk drive		*/
#define DEV_RZ23	"RZ23"		/* RZ23 disk drive		*/
#define DEV_RZ55	"RZ55"		/* RZ55 disk drive		*/
#define DEV_RM03	"RM03"		/* RM03 disk drive		*/
#define DEV_RM05	"RM05"		/* RM05 disk drive		*/
#define DEV_RM80	"RM80"		/* RM80 disk drive		*/
#define DEV_RP05	"RP05"		/* RP05 disk drive		*/
#define DEV_RP06	"RP06"		/* RP06 disk drive		*/
#define DEV_RP07	"RP07"		/* RP07 disk drive		*/
#define DEV_RAMDISK	"RAMDISK"	/* RAM memory disk		*/
#define DEV_VT100	"VT100" 	/* VT100 terminal		*/
#define DEV_VT101	"VT101" 	/* VT101 terminal		*/
#define DEV_VT102	"VT102" 	/* VT102 terminal		*/
#define DEV_VT125	"VT125" 	/* VT125 terminal		*/
#define DEV_VT220	"VT220" 	/* VT220 terminal		*/
#define DEV_VT240	"VT240" 	/* VT240 terminal		*/
#define DEV_VT241	"VT241" 	/* VT241 terminal		*/
#define DEV_VT320	"VT320" 	/* VT320 terminal		*/
#define DEV_VT330	"VT330" 	/* VT330 terminal		*/
#define DEV_VT340	"VT340" 	/* VT340 terminal		*/
#define DEV_VR100	"VR100" 	/* VR100 terminal		*/
#define DEV_VR260	"VR260" 	/* VR260 terminal		*/
#define DEV_VR290	"VR290" 	/* VR290 terminal		*/
#define DEV_MOUSE	"VSXXXAA"	/* Graphics serial mouse	*/
#define DEV_TABLET	"VSXXXAB"	/* Graphics tablet		*/
#define DEV_TRACE	"TRACE" 	/* TRACE special device 	*/
#define DEV_XOS 	"XOS"		/* X in kernel special device	*/

/* Definitions for stat longword (stat) */
#define DEV_BOM 	0x01		/* Beginning-of-medium (BOM)	*/
#define DEV_EOM 	0x02		/* End-of-medium (EOM)		*/
#define DEV_OFFLINE	0x04		/* Offline			*/
#define DEV_WRTLCK	0x08		/* Write locked 		*/
#define DEV_BLANK	0x10		/* Blank media			*/
#define DEV_WRITTEN	0x20		/* Write on last operation	*/
#define DEV_CSE 	0x40		/* Cleared serious exception	*/
#define DEV_SOFTERR	0x80		/* Device soft error		*/
#define DEV_HARDERR	0x100		/* Device hard error		*/
#define DEV_DONE	0x200		/* Operation complete		*/
#define DEV_RETRY	0x400		/* Retry			*/
#define DEV_ERASED	0x800		/* Erased			*/

/* Definitions for category_stat longword (category_stat) */
#define DEV_TPMARK	0x01		/* Unexpected tape mark 	*/
#define DEV_SHRTREC	0x02		/* Short record 		*/
#define DEV_RDOPP	0x04		/* Read opposite		*/
#define DEV_RWDING	0x08		/* Rewinding			*/
#define DEV_800BPI	0x10		/* 800 bpi tape density 	*/
#define DEV_1600BPI	0x20		/* 1600 bpi tape density	*/
#define DEV_6250BPI	0x40		/* 6250 bpi tape density	*/
#define DEV_6666BPI	0x80		/* 6666 bpi tape density	*/
#define DEV_10240BPI	0x100		/* 10240 bpi tape density	*/
#define DEV_DISKPART	minor(dev)%0x08 /* Disk partition macro 	*/
#define DEV_MODEM	0x01		/* Line supports modem control	*/
#define DEV_MODEM_ON	0x02		/* Modem control is turned on	*/
