/*
 * machMon.h --
 *
 *     Structures, constants and defines for access to the sun monitor.
 *     These are translated from the sun monitor header file "sunromvec.h".
 *
 * NOTE: The file keyboard.h in the monitor directory has all sorts of useful
 *       keyboard stuff defined.  I haven't attempted to translate that file
 *       because I don't need it.  If anyone wants to use it, be my guest.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/boot/sunprom/sun3.md/RCS/machMon.h,v 1.1 90/09/17 10:57:28 rab Exp Locker: rab $ SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

#include <sys/types.h>

#if defined(sun4)
/*
 * The memory address for the PROM vector.
 */
#define PROM_BASE	0xffe81000
#endif

#if defined(sun3) || (sun2)
/*
 * The memory addresses for the PROM, and the EEPROM.
 * On the sun2 these addresses are actually 0x00EF??00
 * but only the bottom 24 bits are looked at so these still
 * work ok.
 */

#define EEPROM_BASE     0x0fe04000
#define PROM_BASE       0x0fef0000
#endif

/*
 * Where the rom vector is defined.
 */

#define	romVectorPtr	((MachMonRomVector *) PROM_BASE)


/*
 * Here is the structure of the vector table which is at the front of the boot
 * rom.  The functions defined in here are explained below.
 *
 * NOTE: This struct has references to the structures keybuf and globram which
 *       I have not translated.  If anyone needs to use these they should
 *       translate these structs into Sprite format.
 */

typedef struct {
	char		*initSp;		/* Initial system stack ptr  
						 * for hardware */
	int		(*startMon)();		/* Initial PC for hardware */

	int		*diagberr;		/* Bus err handler for diags */

	/* 
	 * Monitor and hardware revision and identification
	 */

	struct MachMonBootParam **bootParam;	/* Info for bootstrapped pgm */
 	unsigned	*memorySize;		/* Usable memory in bytes */

	/* 
	 * Single-character input and output 
	 */

	unsigned char	(*getChar)();		/* Get char from input source */
	int		(*putChar)();		/* Put char to output sink */
	int		(*mayGet)();		/* Maybe get char, or -1 */
	int		(*mayPut)();		/* Maybe put char, or -1 */
	unsigned char	*echo;			/* Should getchar echo? */
	unsigned char	*inSource;		/* Input source selector */
	unsigned char	*outSink;		/* Output sink selector */

	/* 
	 * Keyboard input (scanned by monitor nmi routine) 
	 */

	int		(*getKey)();		/* Get next key if one exists */
	int		(*initGetKey)();	/* Initialize get key */
	unsigned int	*translation;		/* Kbd translation selector 
						   (see keyboard.h in sun 
						    monitor code) */
	unsigned char	*keyBid;		/* Keyboard ID byte */
	int		*screen_x;		/* V2: Screen x pos (R/O) */
	int		*screen_y;		/* V2: Screen y pos (R/O) */
	struct keybuf	*keyBuf;		/* Up/down keycode buffer */

	/*
	 * Monitor revision level.
	 */

	char		*monId;

	/* 
	 * Frame buffer output and terminal emulation 
	 */

	int		(*fbWriteChar)();	/* Write a character to FB */
	int		*fbAddr;		/* Address of frame buffer */
	char		**font;			/* Font table for FB */
	int		(*fbWriteStr)();	/* Quickly write string to FB */

	/* 
	 * Reboot interface routine -- resets and reboots system.  No return. 
	 */

	int		(*reBoot)();		/* e.g. reBoot("xy()vmunix") */

	/* 
	 * Line input and parsing 
	 */

	unsigned char	*lineBuf;		/* The line input buffer */
	unsigned char	**linePtr;		/* Cur pointer into linebuf */
	int		*lineSize;		/* length of line in linebuf */
	int		(*getLine)();		/* Get line from user */
	unsigned char	(*getNextChar)();	/* Get next char from linebuf */
	unsigned char	(*peekNextChar)();	/* Peek at next char */
	int		*fbThere;		/* =1 if frame buffer there */
	int		(*getNum)();		/* Grab hex num from line */

	/* 
	 * Print formatted output to current output sink 
	 */

	int		(*printf)();		/* Similar to "Kernel printf" */
	int		(*printHex)();		/* Format N digits in hex */

	/*
	 * Led stuff 
	 */

	unsigned char	*leds;			/* RAM copy of LED register */
	int		(*setLeds)();		/* Sets LED's and RAM copy */

	/* 
	 * Non-maskable interrupt  (nmi) information
	 */ 

	int		(*nmiAddr)();		/* Addr for level 7 vector */
	int		(*abortEntry)();	/* Entry for keyboard abort */
	int		*nmiClock;		/* Counts up in msec */

	/*
	 * Frame buffer type: see <sun/fbio.h>
	 */

	int		*fbType;

	/* 
	 * Assorted other things 
	 */

	unsigned	romvecVersion;		/* Version # of Romvec */ 
	struct globram  *globRam;		/* monitor global variables */
	Address		kbdZscc;		/* Addr of keyboard in use */

	int		*keyrInit;		/* ms before kbd repeat */
	unsigned char	*keyrTick; 		/* ms between repetitions */
	unsigned	*memoryAvail;		/* V1: Main mem usable size */
	long		*resetAddr;		/* where to jump on a reset */
	long		*resetMap;		/* pgmap entry for resetaddr */
						/* Really struct pgmapent *  */
	int		(*exitToMon)();		/* Exit from user program */
	unsigned char	**memorybitmap;		/* V1: &{0 or &bits} */
	void		(*setcxsegmap)();	/* Set seg in any context */
	void		(**vector_cmd)();	/* V2: Handler for 'v' cmd */
	int		dummy1z;
	int		dummy2z;
	int		dummy3z;
	int		dummy4z;
} MachMonRomVector;

/*
 * Functions defined in the vector:
 *
 *
 * getChar -- Return the next character from the input source
 *
 *     unsigned char getChar()
 *
 * putChar -- Write the given character to the output source.
 *
 *     void putChar(ch)
 *	   char ch;	
 *
 * mayGet -- Maybe get a character from the current input source.  Return -1 
 *           if don't return a character.
 *
 * 	int mayGet()
 *	
 * mayPut -- Maybe put a character to the current output source.   Return -1
 *           if no character output.
 *
 *	int  mayPut(ch)
 *	    char ch;
 *
 * getKey -- Returns a key code (if up/down codes being returned),
 * 	     a byte of ASCII (if that's requested),
 * 	     NOKEY (if no key has been hit).
 *
 *	int getKey()
 *	
 * initGetKey --  Initialize things for get key.
 *
 *	void initGetKey()
 *
 * fbWriteChar -- Write a character to the frame buffer
 *
 *	void fwritechar(ch)
 *	    unsigned char ch;
 *
 * fbWriteStr -- Write a string to the frame buffer.
 *
 *   	void fwritestr(addr,len)
 *  	    register unsigned char *addr;	/ * String to be written * /
 *  	    register short len;			/ * Length of string * /
 *
 * getLine -- read the next input line into a global buffer
 *
 *	getline(echop)
 *          int echop;	/ * 1 if should echo input, 0 if not * /
 *
 * getNextChar -- return the next character from the global line buffer.
 *
 *	unsigned char getNextChar()
 *
 * peekNextChar -- look at the next character in the global line buffer.
 *
 *	unsigned char peekNextChar()
 *
 * getNum -- Grab hex num from the global line buffer.
 *
 *	int getNum()
 *
 * printf -- Scaled down version of C library printf.  Only %d, %x, %s, and %c
 * 	     are recognized.
 *
 * printhex -- prints rightmost <digs> hex digits of <val>
 *
 *      printhex(val,digs)
 *          register int val;
 *     	    register int digs;
 *
 * abortEntry -- Entry for keyboard abort.
 *
 *     abortEntry()
 */

/*
 * THE FOLLOWING STRUCTURES ARE TAKEN VERBATUM FROM sunromvec.h.
 * The goal is to use the device driver routines in the PROM, and
 * so the following structures are copied in order to retain the interface.
 */

/*
 * Structure set up by the boot command to pass arguments to the program that
 * is booted.  (This is struct bootparam with a facelift.)
 */

typedef struct MachMonBootParam {
	char		*argPtr[8];	/* String arguments */
	char		strings[100];	/* String table for string arguments */
	char		devName[2];	/* Device name */
	int		ctlrNum;	/* Controller number */
	int		unitNum;	/* Unit number */
	int		partNum;	/* Partition/file number */
	char		*fileName;	/* File name, points into strings */
	struct boottab   *bootDevice;	/* Defined below */
} MachMonBootParam;

/*
 * The table entry that describes a device.  It exists in the PROM; a
 * pointer to it is passed in MachMonBootParam.  It can be used to locate
 * PROM subroutines for opening, reading, and writing the device.
 *
 * When using this interface, only one device can be open at once.
 */

typedef struct boottab {
	char	b_dev[2];		/* The name of the device */
	int	(*b_probe)();		/* probe() --> -1 or found controller 
					   number */
	int	(*b_boot)();		/* boot(bp) --> -1 or start address */
	int	(*b_open)();		/* open(iobp) --> -1 or 0 */
	int	(*b_close)();		/* close(iobp) --> -1 or 0 */
	int	(*b_strategy)();	/* strategy(iobp,rw) --> -1 or 0 */
	char	*b_desc;		/* Printable string describing dev */
	struct b_devinfo *b_devinfo;	/* Information to configure device */
} MachMonBootDevice;

enum MAPTYPES { /* Page map entry types. */
  MAP_MAINMEM, 
  MAP_OBIO, 
  MAP_MBMEM, 
  MAP_MBIO,
  MAP_VME16A16D, 
  MAP_VME16A32D,
  MAP_VME24A16D, 
  MAP_VME24A32D,
  MAP_VME32A16D, 
  MAP_VME32A32D
};

/*
 * This table gives information about the resources needed by a device.  
 */
typedef struct b_devinfo {
  unsigned int      d_devbytes;   /* Bytes occupied by device in IO space.  */
  unsigned int      d_dmabytes;   /* Bytes needed by device in DMA memory.  */
  unsigned int      d_localbytes; /* Bytes needed by device for local info. */
  unsigned int      d_stdcount;   /* How many standard addresses.           */
  unsigned long     *d_stdaddrs;  /* The vector of standard addresses.      */
  enum     MAPTYPES d_devtype;    /* What map space device is in.           */
  unsigned int      d_maxiobytes; /* Size to break big I/O's into.          */
} MachMonDevInfo;


/*
 * A "stand alone I/O request", taken from SunOS saio.h (struct saioreq)
 * This is passed as the main argument to the PROM I/O routines
 * in the MachMonBootDevice structure.
 */

#if 0 
typedef int daddr_t;
typedef int off_t;
#endif

typedef struct saioreg {
	char	si_flgs;
	struct boottab *si_boottab;	/* Points to boottab entry if any */
	char	*si_devdata;		/* Device-specific data pointer */
	int	si_ctlr;		/* Controller number or address */
	int	si_unit;		/* Unit number within controller */
	daddr_t	si_boff;		/* Partition number within unit */
	daddr_t	si_cyloff;
	off_t	si_offset;
	daddr_t	si_bn;			/* Block number to R/W */
	char	*si_ma;			/* Memory address to R/W */
	int	si_cc;			/* Character count to R/W */
	struct	saif *si_sif;		/* interface pointer (not defined yet)*/
	char 	*si_devaddr;		/* Points to mapped in device */
	char	*si_dmaaddr;		/* Points to allocated DMA space */
} MachMonIORequest;

/*
 * Functions and defines to access the monitor.
 */

#define Mach_MonPrintf (romVectorPtr->printf)
extern	void 	Mach_MonPutChar ();
extern	int  	Mach_MonMayPut();
extern	void	Mach_MonAbort();
extern	void	Mach_MonReboot();

/*
 * These routines no longer work correctly with new virtual memory.
 */

#define Mach_MonGetChar (romVectorPtr->getChar)
#define Mach_MonGetLine (romVectorPtr->getLine)
#define Mach_MonGetNextChar (romVectorPtr->getNextChar)
#define Mach_MonPeekNextChar (romVectorPtr->peekNextChar)


extern	void	Mach_MonTrap();
extern	void	Mach_MonStopNmi();
extern	void	Mach_MonStartNmi();

#endif /* _MACHMON */
