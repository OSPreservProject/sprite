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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

#include "devZilog.h"

/*
 * The memory addresses for the PROM, and the EEPROM.
 * On the sun2 these addresses are actually 0x00EF??00
 * but only the bottom 24 bits are looked at so these still
 * work ok.
 */

#define EEPROM_BASE     0xffd04000
#define PROM_BASE       0xffe81000

/*
 * The table entry that describes a device.  It exists in the PROM; a
 * pointer to it is passed in MachMonBootParam.  It can be used to locate
 * PROM subroutines for opening, reading, and writing the device.
 *
 * When using this interface, only one device can be open at once.
 *
 * NOTE: I am not sure what arguments boot, open, close, and strategy take.
 * What is here is just translated verbatim from the sun monitor code.  We
 * should figure this out eventually if we need it.
 */

typedef struct {
	char	devName[2];		/* The name of the device */
	int	(*probe)();		/* probe() --> -1 or found controller
					   number */
	int	(*boot)();		/* boot(bp) --> -1 or start address */
	int	(*open)();		/* open(iobp) --> -1 or 0 */
	int	(*close)();		/* close(iobp) --> -1 or 0 */
	int	(*strategy)();		/* strategy(iobp,rw) --> -1 or 0 */
	char	*desc;			/* Printable string describing dev */
	/* Sun4 has struct devinfo here.  Do I need it? */
} MachMonBootTable;

/*
 * Structure set up by the boot command to pass arguments to the program that
 * is booted.
 */

typedef struct {
	char		*argPtr[8];	/* String arguments */
	char		strings[100];	/* String table for string arguments */
	char		devName[2];	/* Device name */
	int		ctlrNum;	/* Controller number */
	int		unitNum;	/* Unit number */
	int		partNum;	/* Partition/file number */
	char		*fileName;	/* File name, points into strings */
	MachMonBootTable   *bootTable;	/* Points to table entry for device */
} MachMonBootParam;

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
	void		(*startMon)();		/* Initial PC for hardware */

	int		*diagberr;		/* Bus err handler for diags */

	/*
	 * Monitor and hardware revision and identification
	 */

	MachMonBootParam **bootParam;		/* Info for bootstrapped pgm */
 	unsigned int	*memorySize;		/* Usable memory in bytes */

	/*
	 * Single-character input and output
	 */

	unsigned char	(*getChar)();		/* Get char from input source */
	void		(*putChar)();		/* Put char to output sink */
	int		(*mayGet)();		/* Maybe get char, or -1 */
	int		(*mayPut)();		/* Maybe put char, or -1 */
	unsigned char	*echo;			/* Should getchar echo? */
	unsigned char	*inSource;		/* Input source selector */
	unsigned char	*outSink;		/* Output sink selector */

	/*
	 * Keyboard input (scanned by monitor nmi routine)
	 */

	int		(*getKey)();		/* Get next key if one exists */
	void		(*initGetKey)();	/* Initialize get key */
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

	void		(*fbWriteChar)();	/* Write a character to FB */
	int		*fbAddr;		/* Address of frame buffer */
	char		**font;			/* Font table for FB */
	void		(*fbWriteStr)();	/* Quickly write string to FB */

	/*
	 * Reboot interface routine -- resets and reboots system.  No return.
	 */

	void		(*reBoot)();		/* e.g. reBoot("xy()vmunix") */

	/*
	 * Line input and parsing
	 */

	unsigned char	*lineBuf;		/* The line input buffer */
	unsigned char	**linePtr;		/* Cur pointer into linebuf */
	int		*lineSize;		/* length of line in linebuf */
	void		(*getLine)();		/* Get line from user */
	unsigned char	(*getNextChar)();	/* Get next char from linebuf */
	unsigned char	(*peekNextChar)();	/* Peek at next char */
	int		*fbThere;		/* =1 if frame buffer there */
	int		(*getNum)();		/* Grab hex num from line */

	/*
	 * Print formatted output to current output sink
	 */

	int		(*printf)();		/* Similar to "Kernel printf" */
	void		(*printHex)();		/* Format N digits in hex */

	/*
	 * Led stuff
	 */

	unsigned char	*leds;			/* RAM copy of LED register */
	void		(*setLeds)();		/* Sets LED's and RAM copy */

	/*
	 * Non-maskable interrupt  (nmi) information
	 */

	void		(*nmiAddr)();		/* Addr for level 7 vector */
	void		(*abortEntry)();	/* Entry for keyboard abort */
	int		*nmiClock;		/* Counts up in msec */

	/*
	 * Frame buffer type: see <sun/fbio.h>
	 */

	int		*fbType;

	/*
	 * Assorted other things
	 */

	unsigned int	romvecVersion;		/* Version # of Romvec */
	struct globram  *globRam;		/* monitor global variables */
	Dev_ZilogDevice *kbdZscc;		/* Addr of keyboard in use */

	int		*keyrInit;		/* ms before kbd repeat */
	unsigned char	*keyrTick; 		/* ms between repetitions */
	unsigned int	*memoryAvail;		/* V1: Main mem usable size */
	long		*resetAddr;		/* where to jump on a reset */
	long		*resetMap;		/* pgmap entry for resetaddr */
						/* Really struct pgmapent *  */
	void		(*exitToMon)();		/* Exit from user program */
	unsigned char	**memorybitmap;		/* V1: &{0 or &bits} */
	void		(*setcxsegmap)();	/* Set seg in any context */
	void		(**vector_cmd)();	/* V2: Handler for 'v' cmd */
	unsigned long	*expectedTrapSig;	/* V3: Location of the expected
						 * trap signal.  Was trap
						 * expected or not? */
	unsigned long	*trapVectorBaseTable;	/* V3: Address of the TRAP
						 * VECTOR TABLE which exists
						 * in RAM. */
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
 *	void getline(echop)
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
 *      void printhex(val,digs)
 *          register int val;
 *     	    register int digs;
 *
 * abortEntry -- Entry for keyboard abort.
 *
 *     void abortEntry()
 */

/*
 * Where the rom vector is defined.  Note that on a Sun-2 the address is
 * 0xef0000.  This is ok because only the low order 24-bits will be looked at.
 */

#define	romVectorPtr	((MachMonRomVector *) PROM_BASE)

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
 * IS THIS TRUE FOR SUN4 TOO?
 */

#define Mach_MonGetChar (romVectorPtr->getChar)
#define Mach_MonGetLine (romVectorPtr->getLine)
#define Mach_MonGetNextChar (romVectorPtr->getNextChar)
#define Mach_MonPeekNextChar (romVectorPtr->peekNextChar)


#ifdef NOTDEF
extern	void	Mach_MonTrap();
#endif /* NOTDEF */
/* Do these routines work with new virtual memory?  They were claimed not to. */
extern	void	Mach_MonStopNmi();
extern	void	Mach_MonStartNmi();

#endif /* _MACHMON */
