/*
 * devTMR.h --
 *
 *	Declarations of sun3/4 timer device.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _devTMR
#define _devTMR

/*
 * Definitions of the Am9513 interval timer chip
 *
 * The Am9513 counter's 5 stages may be concatenated to form an 80 bit
 * counter.  We read The contents of these five 16-bit counter registers 
 * into their respective hold registers with the "save counters" instruction.
 */

typedef struct DevTimerVal {
  unsigned char cmd;
  unsigned char datahi;
  unsigned char datalo;
} DevTimerVal;

typedef struct DevTimerTest {
  unsigned int proc;
  unsigned int inLine;
} DevTimerTest;

#define IOC_DEV_TIMER	(3 << 16)

# define IOC_DEV_TIMER_CMDWR   (IOC_DEV_TIMER | 1)
# define IOC_DEV_TIMER_CMDRD   (IOC_DEV_TIMER | 2)
# define IOC_DEV_TIMER_DATAWR  (IOC_DEV_TIMER | 3)
# define IOC_DEV_TIMER_DATARD  (IOC_DEV_TIMER | 4)
# define IOC_DEV_TIMER_INIT    (IOC_DEV_TIMER | 5)
# define IOC_DEV_TIMER_DEBUG   (IOC_DEV_TIMER | 6)
# define IOC_DEV_TIMER_TEST    (IOC_DEV_TIMER | 7)
# define IOC_DEV_TIMER_SET     (IOC_DEV_TIMER | 8)

# define DEV_TIMER_DISARM  0xdf    /* Disarm all counters */
# define DEV_TIMER_RESET   0xFF    /* Master Reset */
# define DEV_TIMER_SAVE    0xBF    /* Save all 5 counters into the hold registers */
# define DEV_TIMER_LDDPTR  0x00    /* OR'D with register name */
# define DEV_TIMER_LDALL   0x5F    /* Load all timers.  Clears state. */
# define DEV_TIMER_ARM     0x3F    /* ARM all timers. */

/*
 * Counter mode bit
 */

# define DEV_TIMER_F1  0x1B      /* Count from input clock, no gating */
# define DEV_TIMER_F2  0x1C      /* Count from input clock/16, no gating */
# define DEV_TIMER_TC  0x00      /* Previous stage's output, no gating */
# define DEV_TIMER_CNT 0x28      /* Wrap around up counter.  Outputs disabled */
/*
 * Select the register to write.  These are writen to the load
 * data pointer register instruction.
 */

# define DEV_TIMER_HOLDCYC 0x19    /* Read hold registers 1-5 */
# define DEV_TIMER_ELEMCYC 0x01    /* Write all counter registers 1-5 */
# define DEV_TIMER_MMODE   0x17    /* Write master mode register */

# define Dev_TimerReadRegInline(x) { \
		register unsigned char *y; \
		register volatile DevTimerChip *tmrp; \
		tmrp = dev_TimerAddr; \
		DISABLE_INTR(); \
		DEV_TIMER_CMD_PORT;\
		tmrp->d_reg = DEV_TIMER_SAVE; MACH_DELAY(0); \
                tmrp->d_reg = DEV_TIMER_LDDPTR|DEV_TIMER_HOLDCYC;\
	         DEV_TIMER_DATA_PORT; \
		 y = (unsigned char *) (x) + sizeof(unsigned)-1; \
		 *y = tmrp->d_reg; --y; \
		 *y = tmrp->d_reg; --y; \
		 *y = tmrp->d_reg; --y; \
		 *y = tmrp->d_reg;  \
		 ENABLE_INTR(); }


typedef struct DevTimerChip {
	unsigned char cmdPort;       /* write only register pointer */
	unsigned char 	:8;
	unsigned char d_reg;         /* Data register */
	unsigned char   :8;
} DevTimerChip;

extern volatile DevTimerChip *dev_TimerAddr;
/*
 * Ready timer boards for a command write or status read
 */
# define DEV_TIMER_CMD_PORT  dev_TimerAddr->cmdPort=0
/*
 * Ready timer boards for a data read or write 
 */
# define DEV_TIMER_DATA_PORT    devTimerJunkVar=dev_TimerAddr->cmdPort

#ifdef KERNEL
extern int devTimerJunkVar;    /* unused variable */

extern void Dev_TimerReadReg();
extern ClientData Dev_TimerProbe();
extern ReturnStatus Dev_TimerOpen();
extern ReturnStatus Dev_TimerRead();
extern ReturnStatus Dev_TimerIOControl();
#endif

#endif /* _devTMR */

