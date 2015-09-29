
/*
 * @(#)reset.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * reset.c
 *
 * Brings the system from its knees (set up by assembler code at power-on)
 * to its feet.
 */

#include "../sun3/sunmon.h"
#include "../sun3/cpu.map.h"
#include "../sun3/cpu.addrs.h"
#include "../sun3/cpu.misc.h"
#include "../h/globram.h"
#include "../dev/zsreg.h"
#include "../h/keyboard.h"
#include "../h/sunromvec.h"
#include "../sun3/m68vectors.h"
#include "../h/pginit.h"
#include "../h/montrap.h"
#include "../h/dpy.h"
#include "../h/video.h"
#include "../h/led.h"
#include "../h/setbus.h"
#include "../dev/saio.h"
#include "../h/clock.h"
#include "../h/interreg.h"
#include "../h/enable.h"
#include "../sun3/memerr.h"
#include "../sun3/structconst.h"
#include "../h/eeprom.h"

int	trap(), bus_error(), addr_error(), nmi();
void 	reset_uart();

int cpudelay =  3;

/*
 * Table of map entries used to map video back on a watchdog.
 * This is because video might have been running in copy mode 'cuz of
 * Unix.  We take it back since we don't turn on copy mode in the
 * video control reg.
 */
struct pginit videoinit[] = {
	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_ALL, VPM_MEMORY, 0, 0, MEMPG_VIDEO}},
	{((char *)VIDEOMEM_BASE)+128*1024, PGINITEND,
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},
};


/*
 * monreset()
 *
 * Entered from assembler code (trap.s) on all resets, just before we
 * call the command interpreter.  We get to set up all of the I/O
 * devices and most of the rest of the software world.  Memory has been
 * mapped to our preferences, and we are out of boot state.
 * Interrupts cannot occur until we enable them,
 * but we can still get bus/address errors, which fetch their vectors from
 * (as yet uninitialized) mappable memory (not prom).
 *
 * For power-on resets, we are in context 0.  Enough working memory is
 * mapped in starting at location 0 to get us going (a few megs).
 * This memory has been initialized to F's.
 *
 * For other resets, FIXME: maps?  Memory is untouched.
 *
 * The argument is an interrupt stack, the same one passed to monitor().
 * Like the one passed to monitor(), we can modify it and this changes
 * external reality (eg, the PC or registers which will be set up for
 * the user program).
 *
 * There are four kinds of resets possible:
 *	EVEC_RESET	Power-on reset (or equivalently, K2 command)
 *	EVEC_KCMD	K1 command from the keyboard
 *	EVEC_BOOTING	B command from the keyboard
 *	EVEC_DOG	Watchdog reset after CPU double bus fault
 *
 * The Dog is the scariest reset, since we want to disturb as little info
 * as possible (for debugging), but also want to bring the system back
 * under control.  The RESET is where we have to be thorough; bad boards
 * will often make it this far and we have to give good diagnosis of
 * what we find.  The other two are similar to each other and relatively easy.
 */

monreset(monintstack)
	struct monintstack monintstack;
{
	register long *et;
	register int time;
	int i, baud = 0;
	bus_buf bbuf;		/* Buffer for bus error recovery */

	set_enable(get_enable() | ENA_NOTBOOT | ENA_SDVMA);

	if (r_vector != EVEC_DOG) {
		r_context = 0;

		for (et = (long *)TRAPVECTOR_BASE, i = NUM_EVECS; i ; i--)
			*et++ = (long) trap;

		et = (long *)TRAPVECTOR_BASE;
		*et++ = (long) gp;		/* 0 = ptr to Globals area */
		*et++ = (long) romp;		/* 4 = ptr to Transfer Vector */

		*et++ = (long) bus_error;	/* 8 = bus error routine */
		*et++ = (long) addr_error;	/* C = address error routine */
		(void) set_evec(EVEC_LEVEL7, nmi);
	}

	if (r_vector == EVEC_RESET || r_vector == EVEC_BOOT_EXEC
		|| r_vector == EVEC_MENU_TSTS) {
		gp->g_inzscc = SERIAL0_BASE; /* Initialize the UART's */
		gp->g_outzscc = SERIAL0_BASE;
	}

	/*
	 * Write all the assorted initialization commands to both
	 * halves of the UART chip.
	 */
	reset_uart(&SERIAL0_BASE[0].zscc_control, 1);
	reset_uart(&SERIAL0_BASE[1].zscc_control, 0);

	/*
	 *	Check EEPROM for SCC Port A baud rate
         */
	if (EEPROM->ee_diag.eed_ttya_def.eet_sel_baud == EET_SELBAUD) {
	    baud = (EEPROM->ee_diag.eed_ttya_def.eet_hi_baud << 8) +
	            EEPROM->ee_diag.eed_ttya_def.eet_lo_baud;
	    SERIAL0_BASE[1].zscc_control = 12;
            DELAY(2);
            SERIAL0_BASE[1].zscc_control = ZSTIMECONST(ZSCC_PCLK, baud);
            DELAY(2);
            SERIAL0_BASE[1].zscc_control = 13;
            DELAY(2);
            SERIAL0_BASE[1].zscc_control = (ZSTIMECONST(ZSCC_PCLK, baud)) >> 8;
            DELAY(2);
	}

	/*
         *      Check EEPROM for SCC Port B baud rate
         */ 
        if (EEPROM->ee_diag.eed_ttyb_def.eet_sel_baud == EET_SELBAUD) {
            baud = (EEPROM->ee_diag.eed_ttyb_def.eet_hi_baud << 8) +
                    EEPROM->ee_diag.eed_ttyb_def.eet_lo_baud;
            SERIAL0_BASE[0].zscc_control = 12;
            DELAY(2);
            SERIAL0_BASE[0].zscc_control = ZSTIMECONST(ZSCC_PCLK, baud);
            DELAY(2);
            SERIAL0_BASE[0].zscc_control = 13;
            DELAY(2);
            SERIAL0_BASE[0].zscc_control = (ZSTIMECONST(ZSCC_PCLK, baud)) >> 8;
            DELAY(2);
        }

	GXBase = (int)VIDEOMEM_BASE;
	gp->g_font = (unsigned short (*)[CHRSHORTS-1])FONT_BASE;

#ifndef GRUMMAN1 /* no video for grumman */
	if ((r_vector != EVEC_RESET) && (r_vector != EVEC_BOOT_EXEC)
		&& (r_vector != EVEC_MENU_TSTS)) {
		setupmap(videoinit);	/* Initialize video mem map */
		finit (ax, ay);
	}
#endif GRUMMAN1

#ifndef	SIRIUS
	MEMORY_ERR_BASE->mr_er = PER_INTENA | PER_CHECK; /* turn on parity */
#else
	MEMORY_ERR_BASE->mr_er = EER_INTENA ;  /* turn off ecc error rep */
#endif	SIRIUS
	if (r_vector != EVEC_DOG) {
		reset_alloc();  
		CLOCK_BASE->clk_cmd = CLK_CMD_NORMAL;
	}

	/*
	 * This hardware is sufficiently delicate that we need to follow
	 * this order:
	 *	Reset interrupt-catching flops
	 *	Clear and disable TOD chip interrupts
	 *	Enable interrupt-catching flop(s)
	 *	Enable TOD chip interrupts
	 */
	gp->g_nmiclock = 0;	/* initialize nmi counter */
	*INTERRUPT_BASE &= ~(IR_ENA_CLK7 | IR_ENA_CLK5);  /* Unhang flops */
	CLOCK_BASE->clk_intrreg = 0;		/* Disable TOD ints */
	i = CLOCK_BASE->clk_intrreg;		/* Maybe clr pending int */
	*INTERRUPT_BASE |=  IR_ENA_CLK7;	/* Prime the flops */
	CLOCK_BASE->clk_intrreg = CLK_INT_HSEC;	/* Now allow TOD ints */

	/*
	 * These are used by the NMI routine.
	 */
	gp->g_debounce = ZSRR0_BREAK;	/* For remembering BREAK state */
	gp->g_init_bounce = 0x0;        /* Initialize BREAK state */

#ifndef GRUMMAN1 /* no keyboard/mouse for grumman */

	if (r_vector == EVEC_DOG || r_vector == EVEC_RESET || 
	    r_vector == EVEC_BOOT_EXEC || r_vector ==EVEC_MENU_TSTS ||
	    r_vector == EVEC_KCMD || r_vector == EVEC_BOOTING)
	      {
		/* Setup mouse SCC channel - MSP 10/5/85 */
		reset_uart(&KEYBMOUSE_BASE[0].zscc_control, 0);	

		gp->g_keybzscc = &KEYBMOUSE_BASE[1];
		initbuf (gp->g_keybuf, KEYBUFSIZE); /* Set up buffer pointers */

		reset_uart(&gp->g_keybzscc->zscc_control, 0);
		gp->g_keybzscc->zscc_control = 12;
		DELAY(2);
		gp->g_keybzscc->zscc_control = ZSTIMECONST(ZSCC_PCLK, 1200);
		DELAY(2);
		gp->g_keybzscc->zscc_control = 13;
		DELAY(2);
		gp->g_keybzscc->zscc_control = (ZSTIMECONST(ZSCC_PCLK,1200))>>8;
		DELAY(2);
	} 

	if (r_vector == EVEC_RESET || r_vector == EVEC_BOOT_EXEC ||
           r_vector == EVEC_MENU_TSTS) {
		gp->g_insource = INKEYB;        /* set pointer to keyboard */
		gp->g_outsink = OUTSCREEN;	/* set pointer to video */
	}
#endif GRUMMAN1

#ifdef GRUMMAN1 /* Grumman has only ports A & B so we force it to A */
	if (r_vector == EVEC_RESET || r_vector == EVEC_BOOT_EXEC ||
           r_vector == EVEC_MENU_TSTS) {
		gp->g_insource = INUARTA;        /* set pointer to keyboard */
		gp->g_outsink =  INUARTA;	/* set pointer to video */
	}
#endif GRUMMAN1

	/*
         * Now that we need to take NMI's, set up the NMI vector and
         * enable interrupts.  We need to set up the vector in case this
         * is a watchdog; Unix now steals it from us, and the keyboard
         * is unusable after a Dog.
         */
        (void) set_evec (EVEC_LEVEL7, nmi);
        *INTERRUPT_BASE |= IR_ENA_INT;  /* Enable any interrupts */

#ifndef GRUMMAN1 /* no choice for basic io just use port a */

	if (r_vector == EVEC_RESET || r_vector == EVEC_BOOT_EXEC ||
		r_vector == EVEC_MENU_TSTS) {
		gp->g_keystate = STARTUP;	/* initialize state machine */
                gp->g_keybid = KB_UNKNOWN;	/* set flag to unknown keyb */

		for (i = 100; i >= 0; i--) {
			if (sendtokbd(KBD_CMD_RESET)) {
				time = 1000 + gp->g_nmiclock;  /* wait 100 ms */
				do {
					if (gp->g_keybid != KB_UNKNOWN) 
						break;
				} while (time > gp->g_nmiclock);
				break;
			}
		}
	
           gp->g_outsink = OUTSCREEN;   /* Copy to video */
           finit (0, 0); 
           fwritestr ("\f", 1); /* Clear screen */
  
                if (r_vector != EVEC_MENU_TSTS) {
			if(EEPROM->ee_diag.eed_console == EED_CONS_TTYA) {
				banner();
                                printf ("EEPROM: Using RS232 A port.\n");
		    		gp->g_outsink = gp->g_insource = INUARTA;
				banner_test();
			}
			else if (EEPROM->ee_diag.eed_console == EED_CONS_TTYB) {
				banner();
                                printf ("EEPROM: Using RS232 B port.\n");
		    		gp->g_outsink = gp->g_insource = INUARTB;
				banner_test();
		  	}
		  	else if (gp->g_keybid == KB_UNKNOWN) {
				gp->g_insource = INUARTA;  /* serial input */
				banner_test();
				printf("No keyboard found: Using RS232 Port A as input!\n");
		  	}
		  	else {
		    		gp->g_insource = INKEYB;
                        	banner_test();
		  	}
		}
		else {
			if (gp->g_echoinput == 0x12) {
				gp->g_insource = INKEYB;
			} else {
				gp->g_insource = gp->g_sccflag;
                                gp->g_outsink = gp->g_sccflag;
			}
        	}
		initgetkey();
	}
#endif GRUMMAN1

#ifdef GRUMMAN1	/* do this init outside the if block if this is grumman */
		initgetkey();
#endif GRUMMAN1

	if (r_vector == EVEC_DOG) {
		initgetkey();
                return;
	}

	r_usp = gp->g_memorysize;	/* reset User Stack pointer */

	{
		extern  void  vector_default();
		gp->g_vector_cmd = vector_default;
	}

	gp->g_breaking = 0;	/* no break in progress */
	gp->g_breakaddr = 0;	/* initialize break address */
	return;
}

/*
 *	This routine will only display the banner in diagnostic mode, but
 *	it will run a memory test in normal mode.  The memory test has
 *	been added after the banner because of its length of time when
 *	the video screen looks dead.
 */
banner_test() {
	int	beg_addr, limit, size;

	banner();				/* Display the banner */

	/* If Non-Diagnostic mode, run this memrory test */

#ifndef M25
	if ((get_enable() & 0x01) == 0) {
	   if ((EEPROM->ee_diag.eed_memtest << 20) < gp->g_memoryavail) {
		limit = EEPROM->ee_diag.eed_memtest << 20;
		size = EEPROM->ee_diag.eed_memtest;
	   } else {
		limit = gp->g_memoryavail;
		size = (gp->g_memoryavail + 0x4000) >> 20;
	   }

	   printf("\nTesting %d Megabytes of Memory ... ", size);

	   for (beg_addr = 0; beg_addr < limit; beg_addr += MAINMEM_MAP_SIZE) {
		if ((beg_addr + MAINMEM_MAP_SIZE) >= limit) {     /* limit? */
		   test_mem(beg_addr, limit);
	 	   break;
		} else {
	   	   test_mem(beg_addr, beg_addr + MAINMEM_MAP_SIZE); 
		}
		open_window(beg_addr + MAINMEM_MAP_SIZE);
	   }
	   printf("Completed.\n\n");
	   close_window();
	}
#endif M25
}

/*
 *	This routine is intended to test memory from the address
 *	passed into it to the limit passed into it.  This will
 *      test at most 8 Megabytes at a time.
 */
test_mem(addr, limit) 
	int	addr, limit;
{
        int     pass, end_addr, errflag, feedback = 0, pattern = 0x5A972C5A;  
	int	start_addr;
        char    *ind = "-\\|/";
 
	for (pass = 0; pass < 6; pass++) {
            if (pass == 2)
                pattern = 0x5A5A972C;	/* Set the pattern for 2nd pass */
            if (pass == 4)
                pattern = 0x2C5A5A97;	/* Set the pattern for 3rd pass */

            /*  Test 1 Megabyte unless it will hit the global variables */
	    start_addr = addr;
	    for (; start_addr < limit; start_addr += 0x100000) {
                printf("%c\b", ind[feedback++ % 4]);		/* show life */
 	    	if ((end_addr = start_addr + 0x100000) > limit)	/* limit? */
                   end_addr = limit;

            	if ((pass % 2) == 0)
                   errflag = mod3write(start_addr, end_addr, pattern); /* WRITE */
            	else
                   errflag = mod3read(start_addr, end_addr, pattern);  /* READ  */

		/* Check for data compare error */

	    	if (errflag == 0) {
                   printf("\n\nMemory Error at 0x%x:", gp->g_mod3addr);
                   printf(" Exp = 0x%x, Obs = 0x%x, Xor = 0x%x!\n", 
			   gp->g_mod3exp, gp->g_mod3obs, 
			   gp->g_mod3exp ^ gp->g_mod3obs);
                   exit_to_mon();			/* Enter Monitor */
		}
            } 
        }
}

/*
 *	This routine opens a window of pages to map 8 Mb of physical memory 
 *	for the memory test.
 */
open_window(paddr)
	int	paddr;
{
	int	virt_addr, pmeg = MAINMEM_MAP_SIZE/(PGSPERSEG * BYTESPERPG);
	unsigned long	pgentry = (0xc0000000 + (paddr >> 13));

	/* Map 8 Megabytes of segments linearly */

	for (virt_addr = paddr; virt_addr < paddr + MAINMEM_MAP_SIZE; 
	     virt_addr += PGSPERSEG * BYTESPERPG)     
                   setsegmap(virt_addr, pmeg++);

	/* Map 8 Megabytes of pages to the appropriate physical memory */
	
	for (virt_addr = paddr; virt_addr < paddr + MAINMEM_MAP_SIZE; 
             virt_addr += BYTESPERPG)     
                   setpgmap(virt_addr, pgentry++);
}

/*
 *      This routine remaps the segments between memory and the monitor
 *	back to invalid.
 */
close_window()
{
	int	virt_addr;

	for (virt_addr = MAINMEM_MAP_SIZE; virt_addr < 16*MAINMEM_MAP_SIZE;
             virt_addr += PGSPERSEG * BYTESPERPG)
                   setsegmap(virt_addr, SEG_INVALID);
}

