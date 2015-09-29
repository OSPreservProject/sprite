/*
 * sunromvec.h
 *
 * @(#)sunromvec.h 1.7 88/02/08 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sunromvec_h
#define	_sunromvec_h

/*
 * Structure set up by the boot command to pass arguments to the booted program.
 */
struct bootparam {
  char            *bp_argv[8];     /* String arguments.                     */
  char            bp_strings[100]; /* String table for string arguments.    */
  char            bp_dev[2];       /* Device name.                          */
  int             bp_ctlr;         /* Controller Number.                    */
  int             bp_unit;         /* Unit Number.                          */
  int             bp_part;         /* Partition/file Number.                */
  char            *bp_name;        /* File name.  Points into "bp_strings". */
  struct boottab  *bp_boottab;     /* Points to table entry for device.     */
};

/*
 * This table entry describes a device.  It exists in the PROM.  A pointer to
 * it is passed in "bootparam".  It can be used to locate ROM subroutines for 
 * opening, reading, and writing the device.  NOTE: When using this interface, 
 * only ONE device can be open at any given time.  In other words, it is not
 * possible to open a tape and a disk at the same time.
 */
struct boottab {
  char           b_dev[2];        /* Two character device name.          */
  int            (*b_probe)();    /* probe(): "-1" or controller number. */
  int            (*b_boot)();     /* boot(bp): "-1" or start address.    */
  int            (*b_open)();     /* open(iobp): "-"1 or "0".            */
  int            (*b_close)();    /* close(iobp): "-"1 or "0".           */
  int            (*b_strategy)(); /* strategy(iobp, rw): "-1" or "0".    */
  char           *b_desc;         /* Printable string describing device. */
  struct devinfo *b_devinfo;      /* Information to configure device.    */
};

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
struct devinfo {
  unsigned int      d_devbytes;   /* Bytes occupied by device in IO space.  */
  unsigned int      d_dmabytes;   /* Bytes needed by device in DMA memory.  */
  unsigned int      d_localbytes; /* Bytes needed by device for local info. */
  unsigned int      d_stdcount;   /* How many standard addresses.           */
  unsigned long     *d_stdaddrs;  /* The vector of standard addresses.      */
  enum     MAPTYPES d_devtype;    /* What map space device is in.           */
  unsigned int      d_maxiobytes; /* Size to break big I/O's into.          */
};


#include "openprom.h"

#endif _sunromvec_h
