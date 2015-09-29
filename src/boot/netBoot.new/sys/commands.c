

/*
 * @(#)commands.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * commands.c:  Sun ROM monitor command interpreter and execution.
 */
/*CC*************************************************************************
 *CC*
 *CC* 5-Dec-85  Modified the K2 command to reset all of the devices on the
 *CC*		main CPU board that are not reset by the CPU-RESET command
 *CC*  RJHG     But are normally reset by a watchdog reset according to the
 *CC*		Sun/3 architecture manual rev 2.0
 *CC*		Also reset the Intel/Sun control register which is NOT reset
 *CC*		by the CPU-RESET command.  Apperently only the Intel chips
 *CC*		themselves are reset.
 *CC*
 *CC*		NOTE: The K2 command definition is altered to mean a
 *CC*			SOFTWARE reset of all registers/devices on the MAIN
 *CC*			CPU BOARD that are listed as being reset by the
 *CC*			watchdog or power on reset in the Sun-3 Architecture
 *CC*			manual version 2.0
 *CC*
 *CC*************************************************************************/

#include "../sun3/sunmon.h"
#include "../h/globram.h"
#include "../dev/zsreg.h"
#include "../sun3/m68vectors.h"
#include "../h/statreg.h"
#include "../sun3/cpu.addrs.h"
#include "../sun3/cpu.map.h"
#include "../h/buserr.h"
#include "../sun3/cpu.misc.h"
#include "../h/pginit.h"
#include "../h/montrap.h"
#include "../h/led.h"
#include "../h/keyboard.h"
#include "../sun3/memerr.h"
#include "../h/interreg.h"
#include "../h/clock.h"
#include "../dev/saio.h"
#include "../h/enable.h"
#include "../h/eeprom.h"
#include "../diag/diagmenus.h" /* required for struct involving the SCSI */
				 /* logic and reseting it with the K2 cmd  */

#include "../dev/if_obie.h"   /* required for struct involving the Intel */
				 /* ethernet logic and reseting it with the */
				 /* K2 cmd                                  */
#ifdef  SIRIUS
#include        "../h/cache.h"
#endif  SIRIUS


extern unsigned char peekchar(), getone();

char menu_msg[] = "\nOptional Menu Tests\r\nType a character within 10 seconds to enter Menu Tests...";
/*
 * Opcode definitions that the monitor occasionally stuffs into memory
 */
#define	OPC_TRAP1	0x4E41		/* trap 1 */

int trap();		/* Trap handler for trace and bkpt traps */
int bootreset();	/* Call it (no return) to reset and boot */

/*
 * Prints and/or modifies a location.  The address and length (2, 4, or 8
 * hex digits) are arguments.  The result is 1 if the location was modified,
 * 2 if it was not, but a cr was typed, and 0 if anything else was typed.
 */
queryval(adr,len,space)
	register int adr;
	int len, space;
{
	register unsigned char c;
	register unsigned long val;
	register int retval;

	while (' ' == (c = peekchar())) getone();

	if (ishex(c) < 0) {
		/* No value was supplied.  Read current value, print it, then
		   see if s/he wants to modify it. */
		printf(": ");
		switch (len) {			/* Get the value from storage */
		default:val = getsb(adr,space); break;
		case 4: val = getsw(adr,space); break;
		case 8: val = getsl(adr,space); break;
		}
		printhex((int)val,len);

		/* Value is printed.  If there's more on the line, and it's
		   hex, use it; else if there's nonhex, return for continuation,
		   else if there's nothing more on the line, quit. */
		if ('\r' != c) {
			getone();
			while (' ' == (c = peekchar())) getone();
			if (ishex(c) >= 0) goto UseIt;
			putchar ('\n');
			if ('\r' == c)	return 0;	/* Done with command */
			else		return 1;	/* Use rest of line */
		}

		printf("? ");	/* No arg supplied; ask for one */
		getline(1);
		c = peekchar();
		/* CR typed here means "don't store, but continue" */
		if (c == '\r')		return 1;	/* cr typed */
		if (ishex(c) < 0)	return 0;/* non-hex, non-cr */
		val = getnum();
		retval = 1;		/* Continue after storing */
	} else {
		/* Value supplied on command line or with prev value --
		   use it, and quit if end of line, or continue if more */
UseIt:
		val = getnum();
		printf (" -> ");
		printhex ((int)val, len);
		putchar ('\n');
		while (' ' == (c = peekchar())) getone();
		if (c == '\r')	retval = 0;	/* Quit after storing */
		else		retval = 1;	/* Continue after storing */
	}

	switch (len) {		/* Put it back in storage */
		default:	putsb(adr,space,val); break;
		case 4: 	putsw(adr,space,val); break;
		case 8: 	putsl(adr,space,val); break;
	}
/* try sending back retval for now  MJC */
/*
	return 2;
*/
	return(retval);
}

/* Register names */
char reg_names[][3]= {
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
	"A0", "A1", "A2", "A3", "A4", "A5", "A6", 
	"IS", "MS",		/* Two super stack pointers on '020 */
	"US", "SF", "DF", "VB",
	"CA", "CC",		/* Cache controls on 68020 */
	"CX",			/* Context register */
	"SR", "PC",
};

/*
 * Display a register, get new value (if any)
 * If right answer:increment and repeat until last reg
 * Arguments are first reg (&r_d0) and first reg to print.
 */
openreg(rbase, radx)
	long *rbase;
	register long *radx;
{
	register char (*r)[3];

	radx += (getnum()&0xF);	/* only use last hex digit */
	r = reg_names + (radx - rbase);
	for(; r < reg_names+(sizeof reg_names/sizeof *reg_names); r++, radx++) {
		printf(r);	
		if (!queryval ((int)radx, 8, FC_SD)) break;
	}
}

/* Sets up a break point in address space <space> */
dobreak(space)
int space;
{
	register short *bp= gp->g_breakaddr;
	register short *bpa;

	if (bp && (getsw(bp,space) != OPC_TRAP1))
	    bp = 0;

	if (peekchar() == '\r') {
		printhex ( (int)bp, ADDR_LEN);
		printf (" now.\n");
	} else {
		bpa= (short*)getnum();
		if (bp) {
			/* Restore user's old instruction */
			putsw(bp, space, gp->g_breakval);
			/* Restore supervisor's TRAP #1 routine */
			(void) set_evec (EVEC_TRAP1, gp->g_breakvec);
		}
		if (bpa) {
			/* Save new instruction */
			gp->g_breakval = getsw(bpa, space);
			/* Install bkpt atop new instruction */
			putsw(bpa, space, OPC_TRAP1);
			/* Install new TRAP #1 vector in supervisor space */
			gp->g_breakvec = set_evec (EVEC_TRAP1, trap);
			printf ("Break %x installed\n", gp->g_breakval);
		}
		gp->g_breakaddr = bpa;

	};
}
struct pgmapent mainmem_pagemap = {1, PMP_ALL, VPM_MEMORY, 0, 0, 0};

/*
 * This mini-monitor does only a few things:
 *	a <dig>		open A regs
 *	b <filename>	bootload file and start it
 *	c [<address>]	continue [at <address>]
 *	d <dig>		open D regs
 *	e <hex number> 	open address (as word)
 *	g [<address>]	start (call) [at <address>]
 *	k		reset stack [k1 maps, k2 hard reset]
 *	l <hex number> 	open address (as long)
 *	m <hex number>	open segment map
 *	o <hex number>	open address (as byte)
 *	p <hex number>	open page map
 *	r	 	open PC,SR,SS,US
 *	t [y|n|c cmd]	trace yes, no, or continuous
 *	u [specs]	use different console device[s]
 *	z		set simple breakpoint
 */
monitor(monintstack)
	struct monintstack monintstack;
	/* Note: names beginning r_ are references to monintstack */
{
	register char *i;
        struct pgmapent pg;
	register char c;
	register int goadx, adx;
	int (*calladx)();
        struct m25_scsi_control *scsi_ctl; /* pointer to control register   */
					   /* for SCSI interface	    */
					   /* 'm25_scsi_control' is in file */
					   /* diagmenus.h		    */
	char translationsave;
	register int space;
	char justtooktrace = 0;
	int cnt, LEN, j, k, l, adx_mod16;
	int unsigned long val, va;
	int change = 0;
	int modified = 0;

	/*
	 * Default address space for storage-reference commands
	 * depends on whether the interrupted program was in supervisor
	 * or user state.
	 */
	if (r_sr & SR_SUPERVISOR)
		space = FC_SD;		/* supervisor data: FC 5 */
	else
		space = FC_UD;		/* user data: FC 1 */

	if (gp->g_keybid == 3 && r_vector == EVEC_RESET) {
		if (EEPROM->ee_diag.eed_keyclick == EED_KEYCLICK)
			sendtokbd(KBD_CMD_CLICK);     /* turn on Keybd click */
		else
			sendtokbd(KBD_CMD_NOCLICK);   /* turn off Keybd click */
	}

	switch(r_vector) {		/* what caused entry into monitor? */

	case EVEC_TRAPE:	/* "Exit to monitor" trap instruc */
		initgetkey();
		break;

	case EVEC_KCMD:		/* "K1" command */
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
		map_mainmem();  /* map in main memory */
		break;		/* Just reenter monitor */

	case EVEC_BOOTING:
		/* Call boot routine after setting up maps, etc. */
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
BootHere:
		map_mainmem();	/* map in main memory */
		goadx = boot (gp->g_lineptr);
		if (goadx > 0) {
			r_pc = goadx;       /* Boot was OK, goto loaded code */
			goto ContinueTrace;
		}
		break;		/* Then, or else, fall into monitor */

	case EVEC_DOG:
		if (EEPROM->ee_diag.eed_dogaction == EED_DOG_REBOOT)
			k2reset();			/* Do a Power-on-Reset */

		printf("\nWatchdog reset!\n");
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
		break;

	case EVEC_RESET:
		set_leds(~L_RUNNING);		/* Set LED's to normal state */

		printf ("Auto-boot in progress...\n");
		gp->g_lineptr = gp->g_linebuf;	/* Empty argument to boot cmd */
		gp->g_linebuf[0] = 0;
		goto BootHere;		/* Go do the boot */

	case EVEC_ABORT:
		initgetkey();
		printf("\nAbort");
		goto PCPrint;

	case EVEC_MEMERR:
		printf("\nMEMORY ERROR!  Status %x, DVMA-BIT %x, Context %x,",
			MEMORY_ERR_BASE->mr_er, MEMORY_ERR_BASE->mr_dvma,
			MEMORY_ERR_BASE->mr_cxt);
		va = MEMORY_ERR_BASE->mr_vaddr;
		printf("\n  Vaddr: %x", va);
		printf(", Paddr: "); 
                printhex(((getpgmap(va) << 13) + (va & 0x1FFF)), 8); 
                printf(", Type %d", (getpgmap(va) & 0xC000000) >> 26);
		MEMORY_ERR_BASE->mr_dvma = 0;	/* Reset latched interrupt */

		set_enable(get_enable() | ENA_NOTBOOT | ENA_SDVMA);
		*INTERRUPT_BASE &= ~IR_ENA_CLK7;
		*INTERRUPT_BASE |=  IR_ENA_CLK7;
		j = CLOCK_BASE->clk_intrreg;

                printf(" at ");
                printhex( (int)r_pc, ADDR_LEN);
                if (r_format == fvo_format_sixword) {
                        printf(", Instr=");
                        printhex( (int)r_instr_addr, ADDR_LEN);
                	putchar('\n');
		}
		printf("\n");
		if (gp->g_loop & 0x1 != 0) 
			return;
	case EVEC_TRAP1:
		r_pc -= 2;		/* back up to broken word */
	BreakPrint:
		printf("\nBreak ");
		printhex(gp->g_breakval, 4);   /* Print opcode */
		justtooktrace = 1;	/* Allow quick resume with CR */
		goto PCPrint;

	case EVEC_TRACE:
		if (gp->g_breaking) {
		    /* this is single step past	broken instruction */
		    if (getsw(gp->g_breakaddr, space) == gp->g_breakval) {
		    	/* good - not bashed since we set it, so reset it */
			putsw(gp->g_breakaddr, space, OPC_TRAP1);
		    }
		    if (gp->g_breaking == 1) {
			/* Trace was not previously enabled.  Cancel it and
			   resume the breakpointed program. */
			r_sr &= ~SR_TRACE;
			gp->g_breaking = 0;
			/* Fix supervisor trace vec */
			(void) set_evec (EVEC_TRACE, gp->g_breaktrvec);
			return;	/* return to instr after bkpt */
		    }
		    /* Trace was previously enabled.  Take trace trap now. */
		    gp->g_breaking = 0;
		}
		/*
		 * OK, we got a trace trap, but not because we just executed
		 * the instruction at a breakpoint (and are tracing to put
		 * back the breakpoint there).
		 *
		 * If a breakpoint it set at the next instruction, pretend
		 * that's why we got entered.  (Bkpt at loc x takes precedence
		 * over trace at loc x.)
		 */
		if ((long)gp->g_breakaddr == r_pc) 
			goto BreakPrint;
		r_sr |= SR_TRACE;	/* Assume we wanna keep tracing. */
		/* This causes MOVE-to-SR, RTE, etc, to NOT stop the trace. */
		/* Only a mon command will turn the trace bit off, once set */
		justtooktrace = 1;
		printf ("Trace ");
		printhex (*(short *)r_pc, 4);
		goto PCPrint;

	case EVEC_BUSERR:
		if (gp->g_because.be_invalid) 		/* Invalid Page */
			printf ("Invalid Page ");
		else if (gp->g_because.be_proterr)  	/* Protected Page */
			printf ("Protection ");
		else if (gp->g_because.be_timeout)  	/* Timeout Error */
			printf ("Timeout ");
		else if (gp->g_because.be_vmeberr)    	/* VMEbus Error */
			printf ("VMEbus ");
		else if (gp->g_because.be_fpaberr)  	/* FPA Response Error */
                        printf ("FPA Response ");
                else if (gp->g_because.be_fpaenerr) 	/* FPA Enable Error */
                        printf ("FPA Enable ");
		printf("Bus ");		/* Ensure we note it's a bus err */
		goto AccAddPrint;

	case EVEC_ADDERR:
		printf("Address ");
AccAddPrint:
		printf("Error");
		if (r_format == fvo_format_bus20long) printf(", long");
		if (gp->g_bestack.be20_ssw.ssw20_df) {
			printf(":\n  Vaddr: ");
			printhex(gp->g_bestack.be20_data_fault_addr,8);
			printf(", Paddr: ");
			va = gp->g_bestack.be20_data_fault_addr;
                        printhex(((getpgmap(va) << 13) + (va & 0x1FFF)), 8);
			printf(", Type %d", (getpgmap(va) & 0xC000000) >> 26);
			printf(", %s, FC %d, Size %d", 
				gp->g_bestack.be20_ssw.ssw20_rw? "Read":"Write",
				gp->g_bestack.be20_ssw.ssw20_fcode,
				gp->g_bestack.be20_ssw.ssw20_siz?
					gp->g_bestack.be20_ssw.ssw20_siz: 4);
		} else {
			printf(", PC fetch");
		}
PCPrint:
		printf(" at ");
		printhex( (int)r_pc, ADDR_LEN);
		if (r_format == fvo_format_sixword) {
			printf(", Instr=");
			printhex( (int)r_instr_addr, ADDR_LEN);
		}
		putchar('\n');
		break;
		
	case EVEC_MENU_TSTS:
		set_leds(~L_RUNNING);	/* Set LED's to normal state */
		map_mainmem();  /* map in main memory (i.e., set up the page
				 * map for main menu).
				 */
                menutests(space);
		break;				

	case EVEC_BOOT_EXEC:
		set_leds(~L_RUNNING);	/* Set LED's to normal state */
		printf("%s", menu_msg);

		for (j = 0; j < 5; j++) {
			if (mayget() != -1) {
				menutests(space);
				goto boot_cont; /* Then, go to monitor */
			}
			DELAY(1000000);
		}

		gp->g_lineptr = gp->g_linebuf;
        	*gp->g_lineptr++ = 0;
		*gp->g_lineptr++ = 0;
        	gp->g_lineptr = gp->g_linebuf;
		bootreset();		/* Boot EEPROM path */
boot_cont:
		break;			/* by passing command line */

	default: 
	DeFault:
		printf("\nException %x", r_vector);
		if (r_format != fvo_format_short
		 && r_format != fvo_format_sixword)
			printf(" format %x", r_format);
		goto PCPrint;
	}	/* end of switch(r_vector) */

	r_highsr = 0;		/* clear extraneous high word */

/* The following is temporarily deleted cause stack somehow
   gets screwed up FIXME.  mjc 
*/
/*	translationsave = gp->g_translation;
	gp->g_translation = TR_ASCII;
*/

	if (gp->g_tracecmds) {
		/* Auto-execute a command after a trace trap if no keyin */
		if (r_vector != EVEC_TRACE || mayget() >= 0) {
			gp->g_tracecmds = 0;
		} else {
			gp->g_lineptr = gp->g_tracecmds;
			goto TraceCont;
		}
	}

	for (;;) {
		if (gp->g_tracecmds) {
			gp->g_lineptr++;
			if (*gp->g_lineptr) 
				goto TraceCont;
			if (mayget() < 0) 
				goto ContinueTrace;
		}
		/* Else get a line of input */
		/* Don't prompt or input line for menu tests or
		/* exec boot */
		if (r_vector != EVEC_MENU_TSTS && 
		    r_vector != EVEC_BOOT_EXEC) {
			putchar('>');
			getline(1);
		    }
		/* reset r_vector for normal Monitor prompts and input */
		if (r_vector == EVEC_MENU_TSTS ||
		    r_vector == EVEC_BOOT_EXEC)
			r_vector = EVEC_RESET;
		if (justtooktrace && peekchar() == '\r') 
			goto ContinueTrace;

		justtooktrace = 0;

TraceCont:
		while(c= getone()) 		/* Skip control chars */
		    if (c >= '0') 
			break;

		skipblanks();	/* remove any spaces before argument */

		if (c == '\0') 
			continue;

		switch(c&UPCASE) {	/* slight hack, force upper case */

		case 'A':	/* open A register */
			openreg(&r_d0, &r_a0);
			goto setcontexts;	/* Might have modified UC/SC */

		case 'B':	/* Bootload from net, disk, or whatever */
			gp->g_linebuf[gp->g_linesize] = 0;
			/* If first char is not '!', reset system too */
			if ('!' == peekchar()) {
				getone();	/* Skip the - */
				r_pc = boot (gp->g_lineptr);
			} else {
				bootreset();
				/*NOTREACHED*/
			}
			break;

		case 'C':	/* Continue */
			if(ishex(peekchar()) >= 0) 
				r_pc = getnum();
		ContinueTrace:
			if ( (long)gp->g_breakaddr == r_pc) {
			    putsw(gp->g_breakaddr, space, gp->g_breakval);
			    gp->g_breaking = 1 + (r_sr & SR_TRACE);
			    gp->g_breaktrvec = set_evec(EVEC_TRACE, trap);
			    r_sr |= SR_TRACE;
			}
/*  deleted for the same reason mentioned above  FIXME  mjc
			gp->g_translation = translationsave;
*/
			return;

		case 'D':	/* open D register */
			openreg(&r_d0, &r_d0);
			goto setcontexts;	/* Might have modified UC/SC */

		case 'E':	/* open memory (short word) */
			for(goadx = getnum(); ;goadx+=2) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,4,space)) break;
			}
			break;

		case 'F':	/* Fill address space with pattern */
			skipblanks();	/* to next non-blank */
			j = getnum();
                        skipblanks();    /* to next non-blank */ 
			k = getnum(); 
                        skipblanks();    /* to next non-blank */ 
			l = getnum();
                        skipblanks();    /* to next non-blank */ 
			switch (getone() & UPCASE)
			  {

			    case 'W':
				LEN = WORD_LEN;
				j = j & 0xFFFFFFFE; /* make word address */
				k = k & 0xFFFFFFFE;
				break;
 
			    case 'L':
				LEN = LONG_LEN;
				j = j & 0xFFFFFFFC; /* make long address */
				k = k & 0xFFFFFFFC;
				break;

			    case 'B':
				LEN = BYTE_LEN;
				break;

			    default:
				LEN = BYTE_LEN;
				break;
			  }
			for (adx = j; adx <= k; adx += (LEN/2))
			  {
			    switch (LEN)
			     {
				case BYTE_LEN:
				  putsb(adx,space,l); break;
				case WORD_LEN:
				  putsw(adx,space,l); break;
				case LONG_LEN:
				  putsl(adx,space,l); break;
			     }
			  }
			break;

		case 'G':	/* Go */
			if (ishex (peekchar()) >= 0)
				*((long*)&calladx) = getnum();
			else
				*((long*)&calladx) = r_pc;
			gp->g_linebuf[gp->g_linesize] = 0;
                        skipblanks();    /* to next non-blank */ 
			if ((int) calladx < 8) goto v_command;
			(*calladx)(gp->g_lineptr);
			break;

		case 'H':
			help();
			break;
#ifdef  SIRIUS
                case 'I':
                        for (adx = getnum() + CACHEDOFF; ; adx+= CACHEDINCR) {
                            if (adx < CACHEDOFF ||
                                        (adx >= CACHEDOFF
                                         + CACHE_SIZE)) break;
                                printf("CacheData ");
                                printhex(adx,8);
                                if (!queryval(adx,8,FC_MAP)) break;
                        }
                        break;
                 case 'J':
                        for (adx = getnum() + CACHETOFF; ; adx+= CACHETINCR) {
                            if (adx < CACHETOFF ||
                                        (adx >= CACHETOFF
                                         + CACHE_SIZE)) break;
                                printf("CacheTags ");
                                printhex(adx,8);
                                if (!queryval(adx,8,FC_MAP)) break;
                        }
                        break;
 
                case 'N':
                                skipblanks();
                                switch((c=getone()&UPCASE))
                                {
                                        case 'I':
                                              printf("Invalidate");
                                                break;
                                        case 'E':
                                              printf("Enable");
                                                break;
                                        case 'D':
                                              printf("Disable");
                                                break;
 
                                        default: break;
                                }
			printf(" Cache\n"); /* for squeeze  */
                        cache_dei(c);  /* do the action */
                        break;
                case 'Y':  /* flush */
                        skipblanks();
                        c = getone() & UPCASE;
                        /* now, get num-addr */
                        skipblanks();
                        if( (adx=getnum()) < 0) break ; /* error */
                        /* interpret the previous 'c' */
                                switch(c) {
                                        case 'C' :
                                                vac_ctxflush(adx);
                                                printf("Context");
                                                break;
                                        case 'S' :
                                        case 'P' :
                                                if( (goadx=getnum()) < 0) break;                                                if( c == 'S') {
                                                      vac_segflush(adx,goadx);
                                                 printf("Segment");
                                                }
                                                else {
                                                      vac_pageflush(adx,goadx);
                                                 printf("Page");
                                                }
                                                break;
                                        default:
/*                                         printf("Wrong command %c\n",c); */	
                                                break;
                                        }
			printf(" flushed\n");
                        break;  /* end 'Y' */
#endif  SIRIUS


		case 'K':	/* reset monitor */
			switch (getnum()) {

			case 0:		/* K0 - Reset instruction only */
				resetinstr(); 
				*INTERRUPT_BASE |= IR_ENA_INT + IR_ENA_CLK7;
				/* Enable NMI */
#ifndef GRUMMAN
				finit (ax, ay); /* Fix video */
#endif GRUMMAN
				break;

			case 1:		/* K1 - Reset 'most everything */
				softreset();
				/*NOTREACHED*/

			case 2:        /* K2 - Reset just like power-up */
				resetinstr();

#ifndef	M25 /* this register isn't in the m25 (Sun-3/50) so we don't    */
	    /* clear it	if this is a Sun-3/50				*/

				ETHER_BASE->obie_noreset = 0; /* reset the  */
				ETHER_BASE->obie_noloop  = 0; /* Intel/Sun  */
				ETHER_BASE->obie_ca      = 0; /* ethernet   */
				ETHER_BASE->obie_ie      = 0; /* control reg*/

#endif	M25
				  /* reset the keyboard/mouse uart which is */
				  /* not reset by the CPU-RESET instruction */
				  /* in the k2reset code	 	    */
				reset_uart(&KEYBMOUSE_BASE[0].zscc_control,1);
				
				setfc3(sizeof(u_char),ENABLEREG,0);
					/* clear SYSTEM enable reg */

				setfc3(sizeof(u_char),UDMAENABLEOFF,0);
					/* clear USR DVMA enable reg */

				setfc3(sizeof(u_char),DIAGREG,0);
					/* clear USR DVMA enable reg */

				MEMORY_ERR_BASE->mr_er = 0;
				
					/* clear memory error control */
					/* ECC or PARITY memory       */
#ifdef SIRIUS
					/* clear ECC Diagnostic reg */
#endif SIRIUS

#ifdef	M25
                                scsi_ctl = (struct m25_scsi_control *) 
                                           ((u_int)SCSI_BASE + 
                                                   M25_SCSI_CONTROL_OFFSET);
				scsi_ctl->status = 0;
					/* reset the 2 SCSI interface chips */
					/* and the control logic	    */
#endif	M25

				k2reset();
                                /*NOTREACHED*/

			case 0xB:	/* KB - Print power-up banner only */
				banner();
				break;

			default: 
				break;
			}
			break;			

		case 'L':	/* open memory (long word) */
			for(goadx = getnum(); ;goadx+=4) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,8,space)) break;
			}
			break;

		case 'M':	/* open segment map entry */
			for (goadx = getnum() & (ADRSPC_SIZE-1)
					      & ~(PGSPERSEG*BYTESPERPG-1);
					goadx < ADRSPC_SIZE;
					goadx += PGSPERSEG*BYTESPERPG)  {
				printf("SegMap ");
				printhex(goadx, ADDR_LEN);
				if (!queryval((int)SEGMAPADR(goadx),2,FC_MAP))
					break;
			}
			break;

		case 'O':	/* open memory (byte) */
			for(goadx = getnum(); ; ++goadx) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,2,space)) break;
			}
			break;

		case 'P':	/* set page map */
			for (goadx = getnum()&(ADRSPC_SIZE-1)&~(BYTESPERPG-1);
					goadx < ADRSPC_SIZE;
					goadx += BYTESPERPG) {
				printf("PageMap ");
				printhex(goadx, ADDR_LEN);
				{ register segnum_t seggy;
					seggy = getsegmap(goadx);
					printf(" [");
					printhex(seggy,2);
					putchar(']');
				}
				if (!queryval((int)PAGEMAPADR(goadx),8,FC_MAP))
					 break;
			}
			break;
		case 'Q':	/* open EEPROM (byte) */
			modified = 0;
			for (adx = getnum() + (long int )EEPROM_BASE; ; ++adx) {
				if ((adx < (long int )EEPROM_BASE) || 
					(adx >= (long int )EEPROM_BASE
					 + EEPROM_SIZE)) break;
				printf("EEPROM ");
				printhex(adx,EEPROM_LEN);
				change = queryval(adx,BYTE_LEN,space);
				if (!change) 
					break;
				if (change == 2)	/* EEPROM modified */
					modified = 1;
			}
			if (modified)
				eeprom_update(--adx, space);
			break;
		case 'R':	/* open registers */
			openreg(&r_d0, &r_isp);
		setcontexts:
			setcontext(r_context);	/* Modify context reg if set */
			break;

		case 'S':
			if (goadx = getnum()) space = goadx&7;
			else printf ("FC%x space\n", space);
			break;

		case 'T':	/* Trace: ty = on; tcxxx = on w/cmd; else off */
			switch (UPCASE & getone()) {
			case 'C':
				{	/* Turn semis into CR's */
					register unsigned char *foo;
					foo = gp->g_lineptr;
					gp->g_tracecmds = foo;
					while (*foo) {
						if (*foo++ == ';')
							*--foo = '\r';
					}
				}
				goto DoTrace;
			case 'Y':
				gp->g_tracecmds = 0;
			DoTrace:
				printf ("Tracing...\n");
				r_sr |= SR_TRACE;
				gp->g_tracevec = set_evec(EVEC_TRACE, trap);
				goto ContinueTrace;
			default:
				r_sr &= ~SR_TRACE;
				(void) set_evec(EVEC_TRACE, gp->g_tracevec);
				printf("Trace Off\n");
			}
			break;

		case 'U':	/* Use different console I/O */
			usecmd();
			break;
 
		case 'V':	/* View virtual address space */
                        skipblanks();    /* to next non-blank */ 
		   j = getnum() & 0xFFFFFFF0;
                   skipblanks();    /* to next non-blank */ 
		   k = getnum() & 0xFFFFFFF0;
                   skipblanks();    /* to next non-blank */ 
		   switch (getone() & UPCASE)
		     {
			case 'B':	/* View in bytes */
			   LEN = BYTE_LEN;
			   cnt = 16;
			   break;
 
			case 'W':	/* View in words */
			   LEN = WORD_LEN;
			   cnt = 8;
			   break;

			case 'L':	/* View in longwords */
			   LEN = LONG_LEN;
			   cnt = 4;
			   break;

			default:
			   LEN = BYTE_LEN;
			   cnt = 16;
			   break;
		     }
		          for (adx_mod16 = j; adx_mod16 <= k; adx_mod16 += 16)
			    {
			      printhex(adx_mod16,ADDR_LEN);
			      printf(":  ");
			      for (adx = adx_mod16;
				 adx < adx_mod16 + 16; adx += (LEN/2))
	  			{
				 switch (LEN)
				  {
		  		    case BYTE_LEN:	/* display bytes */
		    		     val = getsb(adx,space); break;
		  		    case WORD_LEN:	/* display in words */
		    		     val = getsw(adx,space); break;
		  		    case LONG_LEN:	/* display in longwords */
		    		     val = getsl(adx,space); break;
				  }
				 printhex((int)val,LEN);
	     			 printf(" ");
	  			}
				 printf("    ");
				 for (adx=adx_mod16; adx < adx_mod16 + 16;
					 adx += 1)
	  			  {
	    			    c = getsb(adx, space) & 0x7F;
	    			    if ((c > 0x20) && (c < 0x7F))
					putchar(c);
	    			    else
					putchar(0x2E);
	  			  }
				printf("\n");
				l = mayget();
				if ((l != -1) && (l != ' ')) getchar();
				if (l == ' ') break;
			  }
		break;



		case 'W':	/* Vector */
			*((long*)&calladx) = getnum();
			gp->g_linebuf[gp->g_linesize] = 0;
                        skipblanks();    /* to next non-blank */ 
		v_command:
			(*gp->g_vector_cmd)((long)calladx, gp->g_lineptr);
			break;

		case 'X':	/* Extended Tests */
			if (gp->g_insource == INKEYB)
				gp->g_echoinput = 0x12;
			menureset();
			/* Not reached */

		case 'Z':	/* Zap breakpoint in, 0 removes, none shows */
			dobreak(space);
			break;

		default:
			printf("What?\n");
			break;
		}	/* end of switch(c&UPCASE) */
	}		/* End of loop forever */
}			/* END of procedure monitor() */


/*
 * Subroutine callable via ROM Vector Table which will cause the system
 * to reboot using a specified (or defaulted) argument string, as if it
 * had been typed by the user.
 *
 * Just copy the string to linbuf so it won't get trashed, and call
 * bootreset() which will do all the rest.
 */
boot_me(string)
	char *string;
{

	gp->g_lineptr = gp->g_linebuf;
	while (*gp->g_lineptr++ = *string++) ;
	gp->g_lineptr = gp->g_linebuf;
	bootreset();	/*NOTREACHED*/
}
	
/*
 * Dummy handler for 'v' (vector) routine
 */
void
vector_default(addr, string)
	char *addr;
	char *string;
{
	/* As a demonstration, use it to print strings. */
	if (*string == '%') 
		printf(string, addr);
	else
		printf("Try again.\n");
}

/*
 * Map all of main memory
 */
map_mainmem() {
	register char *i;
        struct pgmapent pg;

	pg = mainmem_pagemap;   /* map in main memory */

        for (i = (char *)0; i < (char *)MAINMEM_MAP_SIZE; pg.pm_page++) {
        	setpgmap(i, pg);
        	i += BYTESPERPG;
        }
}

/*
 * Update EEPROM write count and checksum
 */
eeprom_update(adx, space)
	register int	*adx;
	int	space;
{
	register unsigned char  sum = 0, checksum;
	register char   *sum_addr, *count_addr, *eeprom_addr;
	register char	*start, *end;
	int	count, i;

/*
 *	Decide which EEPROM zone was modified 
 */
	if (adx > (int *)&(EEPROM->ee_diag.eed_nu2) && 
	    adx < (int *)&(EEPROM->ee_resv.eev_wrcnt[0])) {
		start = (char *)&(EEPROM->ee_diag.eed_hwupdate);
		end = (char *)&(EEPROM->ee_resv.eev_wrcnt[0]);
		sum_addr = (char *)&(EEPROM->ee_diag.eed_chksum[0]);
		count_addr = (char *)&(EEPROM->ee_diag.eed_wrcnt[0]);
	} else if (adx > (int *)&(EEPROM->ee_resv.eev_nu2) && 
				 adx < (int *)&(EEPROM->ee_rom.eer_wrcnt[0])) { 
                start = (char *)&(EEPROM->ee_resv.eev_resv[0]);
                end = (char *)&(EEPROM->ee_rom.eer_wrcnt[0]);
		sum_addr = (char *)&(EEPROM->ee_resv.eev_chksum[0]);
		count_addr = (char *)&(EEPROM->ee_resv.eev_wrcnt[0]);
	} else if (adx > (int *)&(EEPROM->ee_rom.eer_nu2) && 
		   adx < (int *)&(EEPROM->ee_soft.ees_wrcnt[0])) {  
                start = (char *)&(EEPROM->ee_rom.eer_resv[0]); 
                end = (char *)&(EEPROM->ee_soft.ees_wrcnt[0]);
		sum_addr = (char *)&(EEPROM->ee_rom.eer_chksum[0]);
		count_addr = (char *)&(EEPROM->ee_rom.eer_wrcnt[0]);
	} else if (adx > (int *)&(EEPROM->ee_soft.ees_nu2) && 
		   adx < (int *)&(EEPROM->ee_soft.ees_resv[243]) + 1) {  
                start = (char *)&(EEPROM->ee_soft.ees_resv[0]); 
                end = (char *)&(EEPROM->ee_soft.ees_resv[240]) + 1;
		sum_addr = (char *)&(EEPROM->ee_soft.ees_chksum[0]);
		count_addr = (char *)&(EEPROM->ee_soft.ees_wrcnt[0]);
	} else {
		return;
	}
/*
 *	Calculate a new checksum for the altered area of the EEPROM
 */
	for (eeprom_addr = start; eeprom_addr < end; eeprom_addr++) 
		sum = sum + getsb(eeprom_addr, space);

	checksum = 256 - sum;	/* sum of bytes + checksum = 8 bit zero */
	putsb(sum_addr, space, checksum);  /* Write the checksum */
/*
 *	Get the current write count & increment by 1
 */
	count = getsb(count_addr++, space) << 8; /* Get the new count */
        DELAY(10000);                            /* Delay for EEPROM recovery */
        count += getsb(count_addr--, space, count) + 1; /* Get the new count */

/*
 *	Write the updated write count in all 4 locations
 */
	for (i = 0; i < 4; i++){
		putsb(count_addr++, space, count >> 8);/* Write the new count */
        	DELAY(10000);                   /* Delay for EEPROM recovery */
		putsb(count_addr++, space, count); /* Write the new count */
		DELAY(10000);                   /* Delay for EEPROM recovery */
	}
}

menutests(space) 
	int	space;
{
	register char	c;
	int		l;

	for (;;) {
		printf("\n\nExtended Test Menu:  (Enter 'q' to return");                        printf(" to Monitor)\n\nCmd -  Test\n\n");
#ifdef M25
                display_opt("ae", "Ethernet");
#else
                display_opt("ie", "Ethernet");
#endif  M25
                display_opt("kb", "Keyboard Input");
                display_opt("me", "Memory");
#ifndef GRUMMAN  
                display_opt("mk", "Mouse/Keyboard Ports");
#endif GRUMMAN
 
#ifndef M25
                display_opt("mt", "TapeMaster Bootpath");
#endif  M25
                display_opt("rs", "Serial Ports");
                display_opt("sd", "SCSI Disk Bootpath");
#ifdef  M25
                display_opt("si", "SCSI Interface");
#endif  M25
                display_opt("st", "SCSI Tape Bootpath");
#ifdef PRISM
                display_opt("vm", "Video (Monochrome)");
                display_opt("vc", "Video (Color)");
                display_opt("ve", "Video Enable Plane");
                display_opt("cm", "Color Map");
#else PRISM
#ifndef GRUMMAN
                display_opt("vi", "Video");
#endif GRUMMAN
#endif PRISM
 
#ifndef M25
/*              display_opt("xd", "Xylogics 751 Disk Bootpath");  */
                display_opt("xt", "Xylogics Tape Bootpath");
                display_opt("xy", "Xylogics 450/451 Disk Bootpath");
#endif  M25
                if ((c = get_cmd()) == 'Q')
                        break;
                l = (int) c;
                c = getone() & UPCASE;
                l = (l<<8) + (int) c;
        
                switch (l) {
 
#ifdef  M25
                case(int)'EA':  /* AMD Ethernet test */
                	amd_ether_test();     /* execute ethernet test */
                	break;
#endif  M25
#ifndef M25
                case (int)'EI': /* Ethernet test */
                	ether_test(); /* execute ethernet test */
                	break;
#endif  M25
                                /*
                                 * The parameter used in call to keybd_test
                                 * is useless.  keybd_test() doesn't declare
                                 * any passed parameters, so it's just ignored.
                                 * keybd_test just checks gp->g_insource to
                                 * see which device (keyboard or port A or B)
                                 * to test.
                                 */
                case (int)'BK': /* Keyboard test */
                	keybd_test(gp->g_insource);
                  	break; 

#ifndef GRUMMAN  
                case (int)'KM': /* Mouse\Keyboard Loopback */
                  	ports_test(1);
                  	break;
#endif GRUMMAN
                case (int)'EM': /* Memory Tests */
                  	memory_test(space, 0);
                  	break;
 
                case (int)'SR': /* Serial Ports test */
                        ports_test(0);
                        break;
 
                case (int)'DS': /* SCSI disk bootpath test */
                        boot_test("sd()");
                        break;
#ifdef  M25
                case (int)'IS': /* SCSI interface test */
                        scsi_test();
                        break;
#endif  M25
                case (int)'TS': /* SCSI tape bootpath test */
                        boot_test("st()");
                        break;
#ifndef M25
                case (int)'TM': /* Tape Master bootpath test */
                        boot_test("mt()");
                        break;
#endif  M25
#ifdef PRISM
                case (int)'MV': /* monochrome video test */
                        memory_test(space, 1);
                        break;
 
                case (int)'EV': /* enable plane test */
                        memory_test(space, 2);
                        break;
        
                case (int)'CV': /* color frame buffer test */
                        memory_test(space, 3);
                        break;
 
                case (int)'MC': /* color map test */
                        memory_test(space, 4);
                        break;
 
#else PRISM
#ifndef GRUMMAN
                case (int)'IV': /* Video test */
                        memory_test(space, 1); /* execute video test */
                        break;
#endif GRUMMAN
#endif PRISM
#ifndef M25
                case (int)'DX': /* Xylogics 751 disk bootpath test */
                        boot_test("xd()");
                        break;

                case (int)'TX': /* Xylogics tape bootpath test */
                        boot_test("xt()");
                        break;

                case (int)'YX': /* Xylogics 450/451 disk bootpath test*/                          	boot_test("xy()");
                        break;
#endif  M25
                default:        /* none of above */
                        printf("\nInvalid command!\n");
                        break;

                }               /* end of switch */
	}                     /* end of for loop */
}

