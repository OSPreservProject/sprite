/*
 * machMon.h --
 *
 *     Structures, constants and defines for access to the sun monitor.
 *     These are translated from the sun monitor header file "openprom.h".
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

/*
 * The memory addresses for the PROM, and the EEPROM.
 */
#define EEPROM_BASE     0xffd04000
#define PROM_BASE       0xffe80010

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
 * Memory layout stuff for the sun4c prom.
 */
typedef	struct	Mach_MemList {
    struct		Mach_MemList	*next;
    unsigned int	address;
    unsigned int	size;
} Mach_MemList;


/*
 * Here is the structure of the vector table which is at the front of the boot
 * rom.  The functions defined in here are explained below.
 */

typedef struct {
    unsigned int	v_magic;	  /* magic mushroom */
    unsigned int      	v_romvec_version; /* Version number of "romvec" */
    unsigned int	v_plugin_version; /* Plugin Architecture version */
    unsigned int	monId;		  /* version # of monitor firmware */
    Mach_MemList	**physMemory;	  /* total physical memory list */
    Mach_MemList	**virtMemory;	  /* taken virtual memory list */
    Mach_MemList	**availMemory;    /* available physical memory */
#ifdef NOTDEF
    struct config_ops	*v_config_ops;	  /* dev_info configuration access */
#else
    /* for now, since I don't know the format of this. */
    int			*v_config_ops;	  /* dev_info configuration access */
#endif
    /*
     * storage device access facilities
     */
    char		**v_bootcommand;  /* expanded with PROM defaults */
    unsigned int	(*v_open)(/* char *name */);
    unsigned int	(*v_close)(/* unsigned int fileid */); 
    /*
     * block-oriented device access
     */
    unsigned int	(*v_read_blocks)();
    unsigned int	(*v_write_blocks)();
    /*
     * network device access
     */
    unsigned int	(*v_xmit_packet)();
    unsigned int	(*v_poll_packet)();
    /*
     * byte-oriented device access
     */
    unsigned int	(*v_read_bytes)();
    unsigned int	(*v_write_bytes)();

    /*
     * 'File' access - i.e.,  Tapes for byte devices.  TFTP for network devices
     */
    unsigned int 	(*v_seek)();
    /*
     * single character I/O
     */
    unsigned char	*inSource;       /* Current source of input */
    unsigned char	*outSink;        /* Currrent output sink */
    unsigned char	(*getChar)();    /* Get a character from input */ 
    void		(*putChar)();    /* Put a character to output sink. */
    int			(*mayGet)();     /* Maybe get a character, or "-1". */
    int			(*mayPut)();     /* Maybe put a character, or "-1". */
    /* 
     * Frame buffer
     */
    void		(*fbWriteStr)();  /* write a string to framebuffer */
    /*
     * Miscellaneous Goodies
     */
    void		(*reBoot)();	   /* reboot machine */
    int			(*printf)();	   /* handles fmt string plus 5 args */
    void		(*abortEntry)();   /* Entry for keyboard abort. */
    int 		*nmiClock;	   /* Counts in milliseconds. */
    void		(*exitToMon)();/* Exit from user program. */
    void		(**v_vector_cmd)();/* Handler for the vector */
    void		(*v_interpret)();  /* interpret forth string */

/*
 *  This may actually be old boot params, depending on this #define:
 *  #ifdef SAIO_COMPAT
 *  struct bootparam	**v_bootparam;
 * 			boot parameters and `old' style device access
 *  The fill int is the else part of the ifdef:
 */
    int			v_fill0;

    unsigned int	(*v_mac_address)(/* int fd; caddr_t buf */);
					    /* Copyout ether address */
    int			*v_reserved[31];
    /*
     * Beginning of machine-dependent portion.
     */
    void		(*SetSegInContext)();	/* set seg map in another
						 * context without worrying
						 * about whether it's mapped. */
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
 * fbWriteStr -- Write a string to the frame buffer.
 *
 *   	void fwritestr(addr,len)
 *  	    register unsigned char *addr;	/ * String to be written * /
 *  	    register short len;			/ * Length of string * /
 *
 * printf -- Scaled down version of C library printf.  Only %d, %x, %s, and %c
 * 	     are recognized.
 *
 * abortEntry -- Entry for keyboard abort.
 *
 *     void abortEntry()
 *
 * SetSegInContext(c, v, p)
 *	context		c;
 *	virtual addr	v;
 *	pmeg_num	p;
 */

/*
 * For accessing the romVector.
 */
#define	romVectorPtr	((MachMonRomVector *) PROM_BASE)

/*
 * Functions and defines to access the monitor.
 */

#define Mach_MonPrintf (romVectorPtr->printf)
#define Mach_MonGetChar (romVectorPtr->getChar)
extern	void 	Mach_MonPutChar ();
extern	int  	Mach_MonMayPut();
extern	void	Mach_MonAbort();
extern	void	Mach_MonReboot();
extern	void	Mach_MonStopNmi();
extern	void	Mach_MonStartNmi();

#endif /* _MACHMON */
