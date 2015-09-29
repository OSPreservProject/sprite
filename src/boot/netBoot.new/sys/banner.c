
/*
 * @(#)banner.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * banner.c:  Prints banner at power-up or for "kb" command.
 */

#include "../sun3/sunmon.h"
#include "../h/globram.h"
#include "../h/idprom.h"
#include "../sun3/cpu.addrs.h"
#include "../h/video.h"
#include "../h/clock.h"
#include "../h/fbio.h"
#include "../h/eeprom.h"

#define	SUN3_KEYBD	0x03

char monrev[] = ID;

banner()
{
	struct idprom id;
	unsigned megs;
	int	i;

	printf("\nSelftest Passed.\n\n");

	if (EEPROM->ee_diag.eed_showlogo != 0x12) {
  	  if (gp->g_outsink == OUTSCREEN)
               	printf("\t");
#ifdef CARRERA
	    printf ("Sun Workstation, Model %sSun-3/160%s, Sun-%s keyboard\n",
	     	       gp->g_fbtype == FBTYPE_SUN2COLOR? "": "Sun-3/75M or ",
		           gp->g_fbtype == FBTYPE_SUN2COLOR? "C": "M",
		           gp->g_keybid == SUN3_KEYBD? "3": "2");
#endif CARRERA
#ifdef M25
            printf ("Sun Workstation, Model Sun-3/50M, Sun-%s keyboard\n",
                     gp->g_keybid == SUN3_KEYBD? "3": "2");
#endif M25
#ifdef SIRIUS
           printf ("Sun Workstation, Model Sun-3/200 Series, Sun-%s keyboard\n",                    gp->g_keybid == SUN3_KEYBD? "3": "2");
#endif SIRIUS
#ifdef PRISM
            printf ("Sun Workstation, Model Sun-3/110LC or Sun-3/130LC, Sun-%s keyboard\n",
                     gp->g_keybid == SUN3_KEYBD? "3": "2");
#endif PRISM
	} else {
		printf("%s", &EEPROM->ee_diag.eed_banner[0]);
	}
		

	megs = (unsigned)(gp->g_memorysize)/(1024*1024);
	if (EEPROM->ee_diag.eed_showlogo != 0x12 && gp->g_outsink == OUTSCREEN)
		printf("\t");
	printf("ROM %s, %dMB memory installed",  monrev, megs);

	if (IDFORM_1 != idprom(IDFORM_1, &id) ||
           (id.id_machine&IDM_ARCH_MASK) != IDM_ARCH_SUN3) {
		printf("\n\tID PROM INVALID\n\007");
	} else {
		printf(", Serial #%d\n", id.id_serial);
		if (EEPROM->ee_diag.eed_showlogo != 0x12 && gp->g_outsink == OUTSCREEN) 
                	printf("\t");
		printf("Ethernet address %x:%x:%x:%x:%x:%x\n",
			id.id_ether[0], id.id_ether[1], id.id_ether[2],
			id.id_ether[3], id.id_ether[4], id.id_ether[5]);
	}

#ifndef GRUMMAN
	if (EEPROM->ee_diag.eed_showlogo != 0x12 && 
			gp->g_outsink == OUTSCREEN && gp->g_ay >= 3)
		sunlogo(gp->g_ay - 3);		/* produce the sun logo */
	printf("\n");
#endif GRUMMAN
}

/*
 *	Display a help menu for monitor commands
 */

help()
{
        fwritestr ("\f", 1); /* Clear screen */
        printf("Boot PROM Monitor Commands\n\n");
	display_border();
	menu_string("a [digit]", 31, "Open CPU Addr Reg (0-7)");
	menu_string("b [dev([cntrl],[unit],[part])]", 10, "Boot a file");
	menu_string("c [addr]", 32, "Continue program at Addr");
	menu_string("d [digit]", 31, "Open CPU Data Reg (0-7)");
	menu_string("e [addr]", 32, "Open Addr as 16 bit word");
	menu_string("f beg_addr end_addr pattn [size]", 8, "Fill Memory");
	menu_string("g [addr]", 32, "Go to Addr");
	menu_string("h", 39, "Help Menu");
#ifdef  SIRIUS
	menu_string("i [addr]", 32, "Open Cache Data");
	menu_string("j [addr]", 32, "Open Cache Tags");
#endif  SIRIUS
	menu_string("k [number]", 30, "Reset (0)CPU, (1)MMU, (2)System");
	menu_string("l [addr]", 32, "Open Addr as 32 bit long");
	menu_string("m [addr]", 32, "Open Segment Map");
#ifdef  SIRIUS
	menu_string("n [i/e/d]", 31, "Cache Invalidate/Enable/Disable");
#endif  SIRIUS
	menu_string("o [addr]", 32, "Open Addr as 8 bit byte");
	menu_string("p [addr]", 32, "Open Page Map");
	menu_string("q [addr]", 32, "Open EEPROM");
	menu_string("r", 39, "Open CPU Regs (i.e. PC)");
	menu_string("s [digit]", 31, "Set/Query Function Code (0-7)");
	menu_string("t [y/n/c]", 31, "Trace: Yes/No/Continue");
	menu_string("u [arg]", 33, "Select Console Device");
	menu_string("v beg_addr end_addr [size]", 14, "Display Memory");
	menu_string("w [addr] [string]", 23, "Vector");
	menu_string("x", 39, "Extended Diag Tests");
#ifdef  SIRIUS
	menu_string("y [c cxt] [s cxt sg_addr] [p cxt pg_adr]", 0, "Flush Cntxt/Seg/Page");
#endif  SIRIUS
	menu_string("z [addr]", 32, "Set Breakpoint");
	display_border();
	printf("\n");
}

/*
 *	Display the top and bottom borders of help menu
 */

display_border() {
	int	i;

	for (i = 0; i < 76; i++)
		putchar('-');
	printf("\n");
}

/*
 *	Display one line of the help menu as follows
 *
 *	cmdstr = command string
 *	blanknum = number of blanks between command string & |
 *	defstr = definition string of command
 */

menu_string(cmdstr, blanknum, defstr) 
	char *cmdstr;
	int  blanknum;
	char *defstr;
{
  	int  i;

  	printf("%s", cmdstr);		/* Display the command string */

  	for(i=0; i < blanknum; i++) 	/* Display in the blank spaces */
    		putchar(' ');

  	printf("| %s\n", defstr);	/* Display the definition string */
}

