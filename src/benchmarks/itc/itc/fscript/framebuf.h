	/*	framebuf.h	1.1	83/02/11	*/
/*
 * framebuf.h - constants for the SUN graphics board version 1
 *
 * Bill Nowicki 	June 14, 1981
 * Vaughan Pratt	added GXleft etc. July 18, 1981
 * Bill Nowicki		October 26, 1981
 *	Updated for PC board
 *	Removed redundant definitions
 * Dave Brown		January 19, 1982
 *	GXsetMask, GXpattern, GXsetSource now use reg. set 3 
 *	(previously they used set 0 -- probably not a good idea)
 * JCGilmore		12 April 1982
 *	Make GXBase (rather than GXUnit0Base) which is resolved
 *	at execution time, to make it easy to relocate the gfx board in
 *	virtual or physical memory.  Change a few "|"s to "+"s for 
 *	code optimization.  Add GXaddrRange for gfx.c device driver.
 * JCGilmore		23 April 1982
 *	Added GXConfig, GXDeviceName, and GXFile, which come from GXfind
 *	(hopefully soon to be made compatible with Standford GXprobe)
 * JCGilmore		21 January 1983
 *	Added GfxSave, which defines a save area where (with kernel help)
 *	multiple processes can share the frame buffer.
 */

/* 
 * GXBase contains the starting address, in the current virtual
 * address space, of the frame buffer.  It is typically initialized 
 * from an ioctl() call on /dev/gfx.  Standalones just punt with
 * an external constant selected at link time.
 */ 
extern int GXBase;

/* 
 * GXConfig contains a bunch of configuration bits which are returned
 * by the GXfind/GXprobe routine.  These indicate whether or not the
 * frame buffer is Present, whether it is Landscape mode (1024 wide by
 * 800 high) or portrait mode (800 wide by 1024 high), and if it is
 * a Color frame buffer.  More bits can be added upon request.
 */
extern long GXConfig;
#define GXCPresent	0x00000001
#define GXCLandscape	0x00000002
#define GXCColor	0x00000004

/*
 * GXDeviceName and GXFile are relevant only under Unix.  They indicate
 * the string containing the device name (default is "/dev/gfx" but can
 * be overridden from the environment (see environ(5) ) by specifying
 * environment variable GraphicsDev.  GXFile is the file descriptor
 * returned by open()ing the file.  Right now it is not too useful,
 * but as more graphics system calls are added, it may prove handy.
 */
extern char *GXDeviceName;
extern int GXFile;

/*
 * The address space occupied by the frame buffer is GXaddrRange bytes
 * long.
 */
#define GXaddrRange	0x20000

/*
 * The low order 11 bits consist of the X or Y address times 2.
 * The lowest order bit is ignored, so word addressing works efficiently.
 */

# define GXselectX (0<<11)	/* the address is loaded into an X register */
# define GXselectx (0<<11)	/* the address is loaded into an X register */
# define GXselectY (1<<11)	/* the address is loaded into an Y register */
# define GXselecty (1<<11)	/* the address is loaded into an Y register */

/*
 * There are four sets of X and Y register pairs, selected by the following bits
 */

# define GXaddressSet0  (0<<12)
# define GXaddressSet1  (1<<12)
# define GXaddressSet2  (2<<12)
# define GXaddressSet3  (3<<12)
# define GXset0  (0<<12)
# define GXset1  (1<<12)
# define GXset2  (2<<12)
# define GXset3  (3<<12)

/*
 * The following bits indicate which registers are to be loaded
 */

# define GXnone    (0<<14)
# define GXothers  (1<<14)
# define GXsource  (2<<14)
# define GXmask    (3<<14)
# define GXpat     (3<<14)

# define GXupdate (1<<16)	/* actually update the frame buffer */


/*
 * These registers can appear on the left of an assignment statement.
 * Note they clobber X register 3.
 */

# define GXfunction	*(short *)(GXBase + GXset3 + GXothers + (0<<1) )
# define GXwidth	*(short *)(GXBase + GXset3 + GXothers + (1<<1) )
# define GXcontrol	*(short *)(GXBase + GXset3 + GXothers + (2<<1) )
# define GXintClear	*(short *)(GXBase + GXset3 + GXothers + (3<<1) )

# define GXsetMask	*(short *)(GXBase + GXset3 + GXmask )
# define GXsetSource	*(short *)(GXBase + GXset3 + GXsource )
# define GXpattern	*(short *)(GXBase + GXset3 + GXpat )

/*
 * The following bits are written into the GX control register.
 * It is reset to zero on hardware reset and power-up.
 * The high order three bits determine the Interrupt level (0-7)
 */

# define GXintEnable   (1<<8)
# define GXvideoEnable (1<<9)
# define GXintLevel0	(0<<13)
# define GXintLevel1	(1<<13)
# define GXintLevel2	(2<<13)
# define GXintLevel3	(3<<13)
# define GXintLevel4	(4<<13)
# define GXintLevel5	(5<<13)
# define GXintLevel6	(6<<13)
# define GXintLevel7	(7<<13)

/*
 * The following are "function" encodings loaded into the function register
 */

# define GXnoop			0xAAAA
# define GXinvert		0x5555
# define GXcopy        		0xCCCC
# define GXcopyInverted 	0x3333
# define GXclear		0x0000
# define GXset			0xFFFF
# define GXpaint		0xEEEE
# define GXpaintInverted 	0x2222
# define GXxor			0x6666

/*
 * The following permit functions to be expressed as Boolean combinations
 * of the three primitive functions 'source', 'mask', and 'dest'.  Thus
 * GXpaint is equal to GXSOURCE|GXDEST, while GXxor is GXSOURCE^GXDEST.
 */

# define GXSOURCE		0xCCCC
# define GXMASK			0xF0F0
# define GXDEST			0xAAAA


/*
 * These may appear in statement contexts to just
 * set the X and Y registers of set number zero to the given values.
 */

# define GXsetX(X)	*(short *)(GXBase + GXselectX + (X<<1)) = 1;
# define GXsetY(Y)	*(short *)(GXBase + GXselectY + (Y<<1)) = 1;

/*
 * This structure can be used to save and restore all the values of
 * registers in the frame buffer.  It takes some discipline, but it's
 * worth it.
 */
struct GfxSave {
	short ReadFifo;		/* Next word that Reads will return */
	short Function;
	short Width;
	short Control;	
	short Mask;
	short Source;
	/*
	 * The X and Y registers get special treatment.  Some routines,
	 * such as Bresenham line drawing, don't know exactly which 
	 * register (X or Y) they are using in the inner loop.  They
	 * therefore just pick a convenient place (eg X[0]) and save
	 * their register there.  Also, these registers get initialized
	 * to zero; referencing them then would produce a bus error.
	 * We could go thru a lot of checking, etc, whether they were
	 * valid or not and which X or Y reg they loaded, but it's not
	 * worth it.  If one of the 8 locations is nonzero, it is touched.
	 * (Throwaway data is written to it...therefore, you must never
	 * store pointers in here with the Update bit set!)  If it is zero,
	 * it is skipped.  The reloading routine doesn't care which registers,
	 * if any, get reloaded in what order.  It's the user program's
	 * resposibility to deal with that.  This means that, to avoid
	 * confusing things (like reloading the same reg twice, with an
	 * old value the second time), routines like Bresenham that put
	 * non-standard pointers in here (e.g. pointers that do not 
	 * set the corresponding register, eg X2 sets X reg 2), must clear
	 * the pointer to zero on exit.
	 */
	short *X[4];		/* The X registers */
	short *Y[4];		/* The Y registers */
#	define X0 X[0]
#	define X1 X[1]
#	define X2 X[2]
#	define X3 X[3]
#	define Y0 Y[0]
#	define Y1 Y[1]
#	define Y2 Y[2]
#	define Y3 Y[3]
	short *BaseAddress;	/* Base address of frame buffer in user
				   virtual memory (for relocating X/Y's) */
};

/*
 * "The" save area, set up for use by gxfind() and thereafter used by
 * the kernel to restore graphics board registers.
 */
extern struct GfxSave GfxSave[1];

