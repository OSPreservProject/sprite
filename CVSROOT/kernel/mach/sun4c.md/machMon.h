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
#define SBUS_BASE	0xf8000000	/* physical */
#define SBUS_SIZE	0x02000000
#define OBIO_BASE	0xf0000000	/* physical */

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

typedef unsigned int MachMonIhandle;
typedef unsigned int MachMonPhandle;

struct	config_ops {
        int (*devr_next)(/* int nodeid */);
        int (*devr_child)(/* int nodeid */);
        int (*devr_getproplen)(/* int nodeid; caddr_t name; */);
        int (*devr_getprop)(/* int nodeid; caddr_t name; addr_t value; */);
        int (*devr_setprop)(/* int nodeid; caddr_t name; addr_t value; int len;
*/);
        int (*devr_nextprop)(/* int nodeid; caddr_t previous; */);
};

#define MACHMON_MAGIC	0x10010407
/*
 * Here is the structure of the vector table which is at the front of the boot
 * rom.  The functions defined in here are explained below.  Fields marked
 * with 01 are only valid with prom v_romvec_version less than 2.
 */

typedef struct {
    unsigned int	v_magic;	  /* magic mushroom.  Should be
					   * MACHMON_MAGIC. */
    unsigned int      	v_romvec_version; /* Version number of "romvec" */
    unsigned int	v_plugin_version; /* Plugin Architecture version */
    unsigned int	monId;		  /* version # of monitor firmware */
    Mach_MemList	**physMemory;	  /* 01 total physical memory list */
    Mach_MemList	**virtMemory;	  /* 01 taken virtual memory list */
    Mach_MemList	**availMemory;    /* 01 available physical memory */
    struct config_ops	*v_config_ops;	  /* dev_info configuration access */
    /*
     * storage device access facilities
     */
    char		**v_bootcommand;  /* 01 expanded with PROM defaults */
    unsigned int	(*v_open)(/* 01 char *name */);
    unsigned int	(*v_close)(/* 01 unsigned int fileid */);
    /*
     * block-oriented device access
     */
    unsigned int	(*v_read_blocks)();	/* 01 */
    unsigned int	(*v_write_blocks)();	/* 01 */
    /*
     * network device access
     */
    unsigned int	(*v_xmit_packet)();	/* 01 */
    unsigned int	(*v_poll_packet)();	/* 01 */
    /*
     * byte-oriented device access
     */
    unsigned int	(*v_read_bytes)();	/* 01 */
    unsigned int	(*v_write_bytes)();	/* 01 */

    /*
     * 'File' access - i.e.,  Tapes for byte devices.  TFTP for network devices
     */
    unsigned int 	(*v_seek)();		/* 01 */
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
    MachMonBootParam	**bootParam;	   /* 01 boot parameters. */

    unsigned int	(*v_mac_address)(/* int fd; caddr_t buf */);
					/* Copyout ether address */
    char	**bootpath;		/* V2: Full path name of boot device */
    char	**bootargs;		/* V2: Boot cmd line after dev spec */

    MachMonIhandle *op_stdin;		/* V2: Console input device */
    MachMonIhandle *op_stdout;		/* V2: Console output device */

    MachMonPhandle (*op_phandle)(/* MachMonIhandle ihandle */);
					/* V2: Convert ihandle to phandle */

    char *	(*op_alloc)(/* caddr_t virthint, u_int size */);
					/* V2: Allocate physical memory */

    void	(*op_free)(/* caddr_t virt, u_int size */);
					/* V2: Deallocate physical memory */

    char *	(*op_map)(/* caddr_t virthint, u_int space, u_int phys,
		u_int size */);		/* V2: Create device mapping */

    void	(*op_unmap)(/* caddr_t virt, u_int size */);
					/* V2: Destroy device mapping */

    MachMonIhandle (*op_open)(/* V2: char *name */);
    int (*op_close)(/* V2: MachMonIhandle fileid */);
    int (*op_read)(/* V2: MachMonIhandle fileid, caddr_t buf, u_int len */);
    int (*op_write)(/* V2: MachMonIhandle fileid, caddr_t buf, u_int len */);
    int (*op_seek)(/* V2: MachMonIhandle fileid, u_int offsh, u_int offsl */);
    void	(*op_chain)(/* V2: caddr_t virt, u_int size, caddr_t entry,
		caddr_t argaddr, u_int arglen */);
    void	(*op_release)(/* V2: caddr_t virt, u_int size */);
    int			*v_reserved[15];

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

extern MachMonRomVector	*machRomVectorPtr;

#define	romVectorPtr	machRomVectorPtr

/*
 * Structures returned by PROM.
 */

typedef struct {
    int bustype;	/* 0 for mainmem, 1 for I/O, slot# for SBus devices */
    char *addr;
    unsigned int size;
} MachDevReg;

typedef struct {
    int pri, vec;
} MachDevIntr;

/*
 * Functions and defines to access the monitor.
 */

#define Mach_MonPrintf (romVectorPtr->printf)
#define Mach_MonGetChar (romVectorPtr->getChar)

/* from SunOS /usr/include/mon/sunromvec.h 1.19 */
/*
 * The possible values for "*romp->v_insource" and "*romp->v_outsink" are
 * listed below.  These may be extended in the future.  Your program should
 * cope with this gracefully (e.g. by continuing to vector through the ROM
 * I/O routines if these are set in a way you don't understand).
 */
#define INKEYB    0 /* Input from parallel keyboard. */
#define INUARTA   1 /* Input or output to Uart A.    */
#define INUARTB   2 /* Input or output to Uart B.    */
#define INUARTC   3 /* Input or output to Uart C.    */
#define INUARTD   4 /* Input or output to Uart D.    */
#define OUTSCREEN 0 /* Output to frame buffer.       */
#define OUTUARTA  1 /* Input or output to Uart A.    */
#define OUTUARTB  2 /* Input or output to Uart B.    */
#define OUTUARTC  3 /* Input or output to Uart C.    */
#define OUTUARTD  4 /* Input or output to Uart D.    */
/* end of /usr/include/mon/sunromvec.h */

/* from SunOS /usr/include/mon/idprom.h 1.18 */

struct idprom {
	unsigned char   id_format;      /* format identifier */
	/* The following fields are valid only in format IDFORM_1. */
	unsigned char   id_machine;     /* machine type */
	unsigned char   id_ether[6];    /* ethernet address */
	long            id_date;        /* date of manufacture */
	unsigned        id_serial:24;   /* serial number */
	unsigned char   id_xsum;        /* xor checksum */
	unsigned char   id_undef[16];   /* undefined */
};

#define IDFORM_1	1	/* Format number for first ID proms */

/* end of /usr/include/mon/idprom.h */


extern	void 	Mach_MonPutChar _ARGS_((int ch));
extern	int  	Mach_MonMayPut _ARGS_((int ch));
extern	void	Mach_MonAbort _ARGS_((void));
extern	void	Mach_MonReboot _ARGS_((char *rebootString));
extern  void    Mach_MonTrap _ARGS_((Address address_to_trap_to));
extern	void	Mach_MonStopNmi _ARGS_((void));
extern	void	Mach_MonStartNmi _ARGS_((void));
extern	void	Mach_MonTraverseDevTree _ARGS_((unsigned int node, int (*func)(), Address clientData));
extern	int	Mach_MonSearchProm _ARGS_((char *name, char *attr, char *buf, int buflen));
extern	void	Mach_MonTraverseAndPrintDevTree _ARGS_((void));


#endif /* _MACHMON */
