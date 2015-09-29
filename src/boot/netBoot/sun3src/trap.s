
|
|	@(#)trap.s 1.6 86/05/24	
|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
|	trap.s -- entered by hardware for all traps
|
|	Biggest function is handling resets.
|
|	If it is a power-up reset, run the diagnostics, map things a little,
|	then jump to C code to deal with setting up the world and booting.
|
|	If it's not a power-up reset, and the Reset vectoring information
|	in page zero is still good, vector to user code to deal with the
|	reset.
|
|	The code vectored to at reset is entered with the following environment:
|		Supervisor state.  Interrupt level 7.
|		Enable register = ENA_NOTBOOT|ENA_PAR_GEN;
|			(note that this disables interrupts and DVMA.)
|			FIXME, on Sun-3 parity controls are elsewhere.
|		Both contexts = 0.
|		Segment map entry 0 = 0th pmeg.
|		Page map entry for g_resetaddr = g_resetmap.  (Supercedes
|			the above page map entries, if there's a conflict.)
|		No segment map entries other than #0 are modified from their
|			state before the reset.
|		Page map entry #1 (0x800-0x1000) is mapped to physical memory
|			page 1.
|		No page map entries outside segment 0 are modified from their
|			state before the reset.
|		Page map entries in segment 0 cannot be depended on.
|			(Note that g_resetaddr should point into segment
|			 zero, if you want to guarantee that you know what
|			 pmeg it is being mapped into.)
|		SSP = pointing to stacked registers and fake exception stack.
|			Base of this stack is at 0x1000.  Note that the
|			pointed-to memory may not be mapped, if its map entry
|			was overridden by the g_resetmap entry.  The actual
|			data is in physical memory page 1.
|		No other registers guaranteed.
|		g_resetaddrcomp has been invalidated, and the NMI timer has
|			been zapped, so a second Reset will not reinvoke the
|			handler unless you fix them.  You must fix the NMI
|			timer if you like NMI's.
|		Memory is not guaranteed to be initialized (i.e. may have
|			any data, and may contain parity errors), except
|			for the stack area between SSP and 0x1000 -- and
|			that might be invalid, if that chunk of physical
|			memory is bad.
|		Other than physical page 1, the proms have not remapped
|			or otherwise messed with things.
|		Code is entered at the address from g_resetaddr.
|
|	This code has to be written very carefully, as it it easy to make
|	assumptions that aren't true.  If the code fails (eg, resets again)
|	before it makes g_resetaddrcomp valid again, the PROMs will avoid
|	invoking RAM code, but will just bootstrap the system from scratch
|	as if they had detected a power-up reset.
|
|*CC***************************************************************************	
|*CC*	
|*CC*	4-Dec-85 RJHG	
|*CC*	
|*CC*		Changed dfc to sfc in 'get_enable' for correct operation.
|*CC*	
|*CC*		NOTE: New info on the _k2reset routine (see _k2reset in here)	
|*CC*	
|*CC***************************************************************************	
|

#include "../sun3/assym.h"
#include "../h/led.h"

#define	DUMBSTACK	0x800
#define delay_2_sec	0x53aaaa
#define IR_ENA_CLK5	0x20

#ifdef	SIRIUS
#define ECC_CTRL_BASE   0x0FE16000
#define	ECC_SYNDROME	ECC_CTRL_BASE+4
#define	SYN_MASK        0x7ffffe 
#define	EER_INTR        0x80	| ECC memory int pending 
#define EER_UE          0x02    | r/o - UE, uncorrectable error 
#define EER_CE          0x01    | r/o - CE, correctable (single bit) error 
#endif	SIRIUS


	.globl  _hardreset, _reset_common, _bootreset, _softreset, _set_evec
	.globl	_exit_to_mon, _trap, _addr_error, _bus_error, _nmi
	.globl  _abortent, _sendtokbd, _get_enable, _peek, _pokec
	.globl	_set_enable, _set_leds, _resetinstr, _setbus, _unsetbus
	.globl	_getidprom, _k2reset, _menureset

|*CC*****************************************************************************
|*CC* 	_k2reset:
|*CC*
|*CC* 	This routine sets a flag to tell _hardreset that this is from the
|*CC*	monitor by setting a flag value in d7.
|*CC*	it does NOT do the resetting functions of a K2 command.
|*CC*
|*CC*****************************************************************************

_k2reset:
	movl	#0x77770000, d7

| 
| Boot PROM reset starting point.
|
| 	This routine does the final details in preparing for a
|	hard reset.  it does NOT do the functions of a K2 command
|
_hardreset:
	movw	#CACR_CLEAR+CACR_ENABLE,a7	| Cache clear and enable bits
	movc	a7,cacr

	movw	#FC_MAP,a7		| SP is already clobberred, use it
	movc	a7,dfc			| Destination function code
	movc	a7,sfc			| Set source function code, too.

	subl    a7,a7                   | Clear A7
        movsb   a7,CONTEXTOFF           | Clear context register
        movsb   a7,SMAPOFF              | Clear segment map loc 0
        movsb   a7,ENABLEOFF            | Enter Boot state

	movl    d7, a7
#ifdef M25
	movsb   SMAPOFF+0xfe00000, d7   | Check for Watchdog reset
        cmpb    #0xfe, d7
        jne     _selftest               | Else, force a Power on Reset

        movsb   SMAPOFF+0xfe20000, d7   | Check for Watchdog reset
        cmpb    #0xfd, d7
        jne     _selftest               | Else, force a Power on Reset

        movsb   SMAPOFF+0xfef0000, d7   | Check for Watchdog reset
        cmpb    #0xf7, d7
        jne     _selftest               | Else, force a Power on Reset
#else M25
        movsb   BUSERROFF, d7
        btst    #0, d7			| Watchdog Reset ?
        jeq     _selftest		| If not, run Power-on diagnostics
#endif M25
 
	movl	a7, d7
	andl	#0xFFFF0000, d7         | This checks if Watchdog was in diags
	cmpl	#0x77770000, d7
	jeq	_selftest

        moveq   #~L_USERDOG,d7          | Entering watchdog code
        movsb   d7,LEDOFF
        movl    a7, d7

| FIXME, for software resets, we need to clear interrupt reg, parity, etc.
| FIXME, (everything that used to be in enable reg!)

	lea	INITSP,sp		| Reset stack ptr to correct value
	moveml	#0xFFFF,sp@-		| Save all regs for user's handler
	movl	usp,a0			| Save USP
	movl	a0,sp@(15*4)		| on top of useless SSP...

	movl	#PME_PROM,d0		| Get our own map entry
	movsl	d0,PMAPOFF		| Map ourself in.
	jmp	dogreset:l		| Jump to our real location

dogreset:
        movl    #PME_MEM_0,d0           | Pg mapper for 0x000-7FF
        movsl   d0,PMAPOFF          	| Map in low memory

        moveml  sp@+,#0xFFFF            | Restore all registers.
        movw    #EVEC_DOG,INITSP-6      | Fake FVO

|
| The following is common to power-up, boot, soft, and watchdog resets.
|
_reset_common:
	movw	#FC_MAP,a7		| Set map function code
	movc	a7,dfc
	subw	a7,a7			| Boot state, shut off rest.
	movsb	a7,ENABLEOFF

	movl	#TRAPVECTOR_BASE,a7	| Set up our vector base (not at 0,
	movc	a7,vbr			| which is where CPU resets put it).

	movl	#_exit_to_mon,INITSP-4	| return address if user rts'es to us
	lea	INITSP-6,sp		| Reset stack ptr below stored stuff
	pea 	USERCODE		| fake PC = User code start addr
	movw	sr,sp@-			| Save current SR (oughta be 2700)
	subw	#mis_sr,sp		| SP was pointing at SR, back it
					| up so sp@(mis_sr) points there.
	moveml	#0xFFFF,sp@(mis_d0)	| Store all registers, including SSP
	movl	sp,d7			| Set "reset or trap" indicator.
	jra	_resettrap		| Pretend we took a trap.
|
| SCC Menu: fake stack as if we'd taken "Diagnostic Menus reset" interrupt
|
_menureset:
	movw    #EVEC_MENU_TSTS,d7	| Fake FVO
	jra     bootsoft
|
| Bootstrap reset: fake stack as if we'd taken "Bootstrap reset" interrupt
|
_bootreset:
	movw	#EVEC_BOOTING,d7	| Fake FVO
	jra	bootsoft
|
| Soft resets (K1): fake stack as if we'd taken a "K command reset" interrupt
|
_softreset:
	movw	#EVEC_KCMD,d7		| Fake FVO
bootsoft:
	jsr     _resetinstr             | Zap out Mainbus devices
	clrb	INTERRUPT_BASE		| Shut off all interrupts.
|
| Reset the world's maps.
|
| This is done by a C routine that runs in boot state on a temp stack,
| so we have to secure a stack area first.
| 
	movl	g_memoryworking,d6	| Save size of working RAM

	movw	#FC_MAP,d0		| SP is already clobberred, use it
	movc	d0,dfc			| Destination function code
	movc	d0,sfc			| Set source function code, too.

	clrl	d0			| Boot state, shut off rest.
	movsb	d0,ENABLEOFF
	movsb	d0,CONTEXTOFF		| Clear context register
	movsb	d0,SMAPOFF		| Clear segment map loc 0
	movl	#PME_MEM_0,d0		
	movsl	d0,PMAPOFF		| Set page map loc 0
	
	lea	DUMBSTACK,a7		| Set stack pointer.

	moveq	#~L_SETUP_MAP,d0
	movsb	d0,LEDOFF

	movl	d6,sp@-			| Size of working memory...
	jbsr	_mapmem			| Go map/zap it.
	addql	#4,sp
	movl	d0,g_memoryavail	| Available memory in the system
	movb	#0,g_loop		| initialize loop option

	movw	d7,INITSP-6		| Fake fvo
	jra	_reset_common		| Go build rest of stack
|
| exit_to_mon is used if the user program returns to us via "rts" (eg, from
| a "boot" command).  We just pretend she did a TRAP #1.
| Note that the trap vector might not be set, so we fake it.
|
_exit_to_mon:
	pea	_exit_to_mon		| For the next rts...
	movw	#EVEC_TRAPE,sp@-	| Fake FVO
	pea	_exit_to_mon		| Push fake PC that brings us back
	movw	#0x2700,sp@-		| Push fake SR
|
|	Entry point for all traps except reset, address, and bus error.
|
_trap:
	subw	#mis_sr,sp		| SP was pointing at SR, back it
					| up so sp@(mis_sr) points there.
	moveml	#0xFFFF,sp@(mis_d0)	| Store all registers, including SSP
	subl	d7,d7			| Clear "reset or trap" indicator.
_resettrap:
	addl	#mis_sr,sp@(mis_a7)	| Make visible SSP == trap frame
	movc	usp,a0			| Drag usp out of the hardware
	movl	a0,sp@(mis_usp)		| Put it in the memory image
	movc	sfc,a0
	movc	dfc,a1
	movc	vbr,a2
	movc	caar,a3
	movc	cacr,a4
	moveml	#0x1F00,sp@(mis_sfc)	| Store sfc, dfc, vbr, caar, cacr

	moveq	#FC_MAP,d0
	movc	d0,sfc
	movsb	CONTEXTOFF,d0
	andb	#CONTEXTMASK,d0		| Throw away undefined bits
	movl	d0,sp@(mis_context)	| Save user context reg

	orw	#0x0700,sr		| Run at interrupt level 7 now.
	tstl	d7			| For resets, call monreset() first.
	jeq	tomon
	jbsr	_monreset		| The args are modifiable if desired.
tomon:
	jbsr	_monitor		| Call the interactive monitor

	movl	sp@(mis_usp),a0		| Get usp out of memory image
	movc	a0,usp			| Cram it into the hardware.
	moveq	#FC_MAP,d0
	movc	d0,dfc
	movl	sp@(mis_context),d0
	movsb	d0,CONTEXTOFF
	movl	sp@(mis_vbase),d0
	movc	d0,vbr
	movl	sp@(mis_sfc),d0
	movc	d0,sfc
	movl	sp@(mis_dfc),d0
	movc	d0,dfc
	movl	sp@(mis_caar),d0
	movc	d0,caar
	movl	sp@(mis_cacr),d0
	movc	d0,cacr
	movl	sp@(mis_a7),a0		| Grab new SSP value (prolly same)
	lea	sp@(mis_sr),a1		| Get the old SSP value
	cmpl	a0,a1			| Are they the same?
	jeq	samestack

	clrw	a0@(6)			| Fake format/vo word
	movl	sp@(mis_pc),a0@(2)	| Stack new PC&SR below new SSP
	movw	sp@(mis_sr),a0@
	moveml	sp@(mis_d0),#0xFFFF	| Restore all regs including new SSP
	rte				| Poof, resume from error.

samestack:
	moveml	sp@(mis_d0),#0x7FFF	| Restore all but SSP
	addw	#mis_sr,sp		| Restore SSP to where it was after trap
	rte				| Byebye...
|
|
|	Entry point for bus and address errors.
|
|	Save specialized info in dedicated locations for debugging things.
|	Then truncate stack to usual size, so user can resume anywhere
|	instead of being able to resume in mid-cycle (sorry...).
|
|	They could resume in mid-cycle by simply stacking this info
|	(that we save in low memory) and rts'ing.  But that's not the
|	default.
|
_addr_error:
_bus_error:
	moveml	#0xFFFF,g_beregs	| Store all regs
	bfextu	sp@(6){#0:#4},d0	| Get format of stack
|	movb	pc@(format_size,d0),d0	| Get size of stack
	 lea	format_size,a0	|FIXME assembler doesn't work
	 movb	a0@(0,d0),d0
	subql	#1,d0			| Set up for dbra.
	lea	g_bestack,a0		| Get set to move all the data
	movl	sp,a1
BEloop:
	movb	a1@+,a0@+		| Move a byte of error info
	dbra	d0,BEloop		| Baby's very first loopmode loop
	
	movc	sfc,a0			| Save it a moment
	moveq	#FC_MAP,d0		| Get the map/CPU layer function
	movc	d0,sfc
	movsb	BUSERROFF,d0		| Get contents of bus error reg
	movb	d0,g_because		| Save for higher level code
	movc	a0,sfc			| Restore previous value

| Truncate stack frame back to manageable proportions
	bfextu	sp@(6){#0:#4},d0	| Get format of stack
|	movb	pc@(format_size,d0),d0	| Get size of stack
	 lea	format_size,a0	|FIXME assembler doesn't work
	 movb	a0@(0,d0),d0
	subql	#sizeofintstack,d0
	movl	sp@,   sp@(0,d0:w)	| Move stack frame out there.
	movl	sp@(4),sp@(4,d0:w)
	addw	d0,sp			| Toss extended info
	andw	#FVO_OFFSET,sp@(i_fvo)	| Clear "long stack frame" format
	movl	g_beregs   ,d0		| Restore d0, a0, and a1
	movl	g_beregs+32,a0		| (Since we clobberred them.)
	movl	g_beregs+36,a1
	jra	_trap			| Enter "normal" trap code

format_size:
	.byte	8, 8, 12, 0, 0, 0, 0, 0	| short, throwaway, sixword
	.byte	58			| 68010 berr
	.byte	20, 28, 0x5C		| Coproc, short berr, long berr on 20
	.byte	0, 0, 0, 0		| Unassigned
|
|
|	Entry point for non-maskable interrupts, used to scan keyboard.
|
_nmi:
	movl	d0,sp@-			| Save a scratch register
	movb	MEMORY_ERR_BASE+MR_ER,d0 | Get parity error indicator
#ifndef	SIRIUS
	andb	#PER_INTR,d0		| See if it's the cause
#else	SIRIUS
	btst	#7,d0			| bit num for ECC int
#endif	SIRIUS
	jeq	not_parity		| Branch if some other interrupt
|
| Memory (parity or ECC) error!
| if its an uncorrectible error then go on to monitor
|  else print the correctible error address
|  reset the ?ecc? int?
|  and go on to exit
#ifdef	SIRIUS
	btst	#0,d0		| is ti a correctible error?
	beq	10$		| its a UE so continue on to monitor
| CE begins here
        movl    a0,sp@-         | save addr reg
        movl    d2,sp@-         |for brd cnt 
        clrl    d2 
        lea     ECC_SYNDROME,a0 | to get the phys addr
5$:
        movl    a0@,d0          | get phys address
        movl    #0,a0@          | clear out current CE report
        btst    #0,d0           | is this the board?
        bne     7$              | yes, run with it
        addql   #1,d2           | bump brd number cnt 
        addl    #0x40,a0        | bump it by nxt adrr amount
        cmpl    #ECC_SYNDROME+(4*0x40),a0 |at end + 1 ?
        bne     5$
| this hack-- cant find right board so use last one MJC hack
7$:
        andl    #SYN_MASK,d0    | mask in A<24..3>
        lsll    #2,d0           | and adjust it  
| now call printf
        movl    d0,sp@-         | push address
|have to add board number here!!! MJC
        movl    d2,sp@-         | push brd num 
        pea     ecc_txt         | and the text format str
        jbsr    _printf
        addl   #12,sp           | adjust sp back
        movl    sp@+,d2         | restore d2 
        movl    sp@+,a0         | restore a0 
|       movl    d1,sp@-         | just set up to exit
        movb    CLOCK_BASE+clk_intrreg,d0 | Is the clock chip interrupting?
	bne	ce_and_clk	| yes, go handle it
	movl    d1,sp@-         | just set up to exit
        jra     NotSerial      | need to check for cll int MJC   NotSerial
#endif	SIRIUS			| debug for now
|
|
10$:	movw	#EVEC_MEMERR,sp@(4+i_fvo) | Indicate that this is memory err
trapout:
	movl	sp@+,d0			| Restore saved register
	jra	_trap			| And just trap out as usual.

not_parity:
	movb	CLOCK_BASE+clk_intrreg,d0 | Is the clock chip interrupting?
	beqs	trapout			| (No, report as "trap 7C".)
| branch here from CE above if clk int 
ce_and_clk:
	andb	#~IR_ENA_CLK7,INTERRUPT_BASE	| Shut it off,
	orb	#IR_ENA_CLK7,INTERRUPT_BASE	| then back on.
	movb  CLOCK_BASE+clk_intrreg,d0 | Clear the interrupt
	movl	d1,sp@-			| Save yet another register
|
| Timer interrupt: Counting the milliseconds for 1/2 second LED pulse
|
	movw	g_nmiclock+2,d0		| Get the clock value
	addl	#1000/NMIFREQ,g_nmiclock| Count off x ms more.
	movw	g_nmiclock+2,d1		| See if the "1/2 second" bit changed
	eorw	d1,d0
	btst	#9,d0			| (2**9 == 512, close enough)
	jeq	Trykey

	movc	dfc,d1			| Save destination fn code
	moveq	#FC_MAP,d0		| Set it up to access LEDs
	movc	d0,dfc
	movb	g_leds,d0		| Grab shadow copy of LEDs
	eorb	#L_HEARTBEAT,d0		| Invert heartbeat LED
	movb	d0,g_leds		| Put it back in RAM shadow loc
	movsb	d0,LEDOFF		| Put it into real LEDs too
	movc	d1,dfc			| Restore function code register
|
| Keyboard/Serial Port scanning code.
|
Trykey:
	movl	a0,d1			| Save a0 temporarily
	movl	g_keybzscc,a0		| Point to keyboard
	tstb	a0@(zscc_control)	| Read to flush previous pointer.

	moveq   #0x7f, d0 
1$:     dbra    d0, 1$

	moveq	#ZSRR0_RX_READY,d0	| Bit mask for RX ready bit
	andb	a0@(zscc_control),d0	| Now read it for real.
	jeq	s2_notnew		| If no char around, skip.

	moveq   #0x7f, d0 
2$:     dbra    d0, 2$

	moveq	#0,d0
	movb	a0@(zscc_data),d0	| Get the keycode
	movl	d1,a0			| Restore A-register temp

	moveml	#0x00C0,sp@-		| Save a0, a1.  d0&d1 already saved.
	movl	d0,sp@-			| Push it as argument to keypress().
	jbsr	_keypress
	addql	#4,sp
	moveml	sp@+,#0x0300		| Restore saved registers
	tstl	d0			| See if keypress() wants us to abort.
	jeq	Trykey
	jra	abort			| Yep, abort to monitor.

s2_notnew:
	movl	d1,a0			| Restore a0 from shenanigans
	tstb	g_insource		| Is input source a serial line?
| FIXME, g_insource should vector.
	jeq	NotSerial		| No, skip over this stuff.
	movl	a0,d1			| Save away a0
	movl	g_inzscc,a0		| Grab uart address
| FIXME, remove next 4 lines when g_inzscc is right.
	cmpb	#INUARTA,g_insource	| See if A-side or B-side
	jne	Brkt1			| (B-side; a0 is OK.)
	addw	#sizeofzscc,a0		| A-side is second in the chip.
Brkt1:
	tstb	a0@(zscc_control)	| Read to flush previous pointer.
	moveq   #0x7f, d0 
1$:     dbra    d0, 1$
	movb	#ZSWR0_RESET_STATUS,a0@(zscc_control)	| Avoid latch action
	moveq   #0x7f, d0 
2$:     dbra    d0, 2$
	movb	#ZSRR0_BREAK,d0		| Who's afraid of the big bad sign?
	andb	a0@(zscc_control),d0	| Now read it for real.
	movl	d1,a0			| Restore a0
	cmpb	g_debounce,d0		| Is its state same as last time?
	jeq	NotSerial		| Yes, don't worry about it.
	movb	d0,g_debounce		| No -- remember its state,
	tstb	g_init_bounce		| First time ?
	jne	abort			| Not first time, Abort
	movb	#1, g_init_bounce	| Remember not first time
| FIXME, there will be a null in the read fifo, but we will ignore it
| anyway, so probably no need to worry.  Check this.

NotSerial:
	movl	sp@+,d1			| Restore other register
	movl	sp@+,d0			| Restore saved register
	rte				| Return from NMI

|
|	Somebody wants to abort whatever is going on, and return to
|	Rom Monitor control of things.  Be obliging.
|
abort:
	movl	sp@+,d1			| Restore other register
	movl	sp@+,d0			| Restore saved register
_abortent:
	movw	#EVEC_ABORT,sp@(i_fvo)	| Indicate that this is an abort.
	jra	_trap			| And just trap out as usual.
	
|
| Send a command byte to the keyboard if possible.
| Return 1 if done, 0 if not done (Uart transmit queue full).
|
_sendtokbd:
	movl	g_keybzscc,a0
	tstb	a0@(zscc_control)	| Read to flush previous pointer.

	moveq   #0x7f, d0 
1$:     dbra    d0, 1$

	moveq   #ZSRR0_TX_READY,d0      | Bit mask for TX ready bit
	andb	a0@(zscc_control),d0	| Now read it for real.
	jeq	sendret			| If can't xmit, just return the 0.

	moveq	#0x7f, d0
2$:	dbra	d0, 2$

	movb	sp@(7),a0@(zscc_data)	| Write the byte to the kbd uart
	moveq	#1,d0			| Indicate success
sendret:rts				| Return to caller.

|
| Gets the current value of the enable register.
|
_get_enable:
	moveq	#0,d0
	movc	sfc,a0			| Save SOURCE function code
	moveq	#FC_MAP,d1		| Set up for map access
	movc	d1,sfc
	movsb	ENABLEOFF,d0		| Snarf it into d0 as result
	movc	a0,sfc			| Restore SFC and exit.
	rts

|
| Sets the enable register to the specified value.
|
_set_enable:
	movw	sp@(6),d0		| Grab desired value from stack
	movc	dfc,a0			| Save dest function code
	moveq	#FC_MAP,d1		| Set up for map access
	movc	d1,dfc
	movsb	d0,ENABLEOFF		| Stuff it into enable reg
	movc	a0,dfc			| Restore dfc and exit.
	rts

|
| Set the LEDs.
|
_set_leds:
	movc	dfc,a0			| Save dest function code
	moveq	#FC_MAP,d1		| Set up for map access
	movc	d1,dfc
	movb	sp@(7),d0		| Get desired LED contents
	movb	d0,g_leds		| Save in global RAM for readback
	movsb	d0,LEDOFF		| Stuff into LED reg
	movc	a0,dfc			| Restore dfc and exit.
	rts
|
| Issue a reset instruction.
|
_resetinstr:
	reset				| That's all.
|following added to make sure int's are cleared from tod
| taken from locore.s
#ifdef  SIRIUS
        movl    #delay_2_sec,d0
del_2sec:
        subql   #1,d0
        bne     del_2sec
 
        lea     CLOCK_BASE+clk_intrreg,a4
        tstb    a4@                     | read CLKADDR->clk_intrreg to clear
        lea     INTERRUPT_BASE,a5
        andb    #~IR_ENA_CLK5,a5@       | clear interrupt request
        orb     #IR_ENA_CLK5,a5@        | and re-enable
        tstb    a4@                     | clear interrupt register again,
#endif  SIRIUS
	rts
|
|	This runs the "normal" code, but sneaks back to the "buserror" code
|	if a bus error occurs.  Note that on the "bus error" return, all
|	register variables are trashed (restored to their values on
|	entry, even though future code may have modified them).
|
_setbus:
	movl	sp@(4),a0		| Get bus buffer area
	movl	sp@,a0@			| Save return address of setbus().
	moveml	a2-a7/d2-d7,a0@(12)	| Save volatile regs
	movc	vbr,a1			| Find trap vectors
	movl	a1@(EVEC_BUSERR),a0@(4)	| Save old bus error vector.
	movl	g_busbuf,a0@(8)		| Save old busbuf address
	movl	a0,g_busbuf		| Install new busbuf address
	movl	#buserr,a1@(EVEC_BUSERR) | Set up bus error vector
	moveq	#0,d0			| Result is zero
	rts

| Entered when a bus error occurs
buserr:
	movl	g_busbuf,a0		| Get bus buffer pointer
	moveml	a0@(12),a2-a7/d2-d7	| Restore regs
	movl	a0@,sp@			| Restore return PC of setbus()
	moveq	#1,d0			| Result is 1, indicating bus error
	jra	unsetout		| Unset it and get out
_unsetbus:
	movl	sp@(4),a0		| Get bus buffer area
unsetout:
	movc	vbr,a1			| Find vectors
	movl	a0@(4),a1@(EVEC_BUSERR)	| Restore old bus error vector
	movl	a0@(8),g_busbuf		| Restore old busbuf address
	rts

|
| peek(addr)
|
| Temporarily re-routes Bus Errors, and then tries to
| read a byte from the specified address.  If a Bus Error occurs,
| we return -1. 
|
_peek:
	movl    a7@(4),a0       	| Get address to probe
        movc    vbr,a1                  | Find trap vectors
        movl    a1@(EVEC_BUSERR),d1	| Save old bus error vector.
        movl    #BEhand,a1@(EVEC_BUSERR) | Set up bus error vector
	movl	sp,a1			| Save stack pointer
        moveq   #0,d0                   | Result is zero
	movb    a0@,d0          	| Read a byte.
BEexit:
	movl	a1,sp			| Restore stack pointer
	movc	vbr,a1			| Find trap vectors
        movl    d1,a1@(EVEC_BUSERR)     | Restore old bus error vector
        rts

BEhand:					| Entered when a bus error occurs
        moveq   #-1,d0                  | Result is 1, indicating bus error
        bra	BEexit			| Exit with flag set

|
| pokec(a,c)
|  
| This routine is the same, but uses a store instead of a read, due to
| stupid I/O devices which do not respond to reads.
|
| if (pokec (charpointer, bytevalue)) itfailed;
|
_pokec:
	movl	a7@(4),a0	| Get address to probe
	movc    vbr,a1          | Find trap vectors
        movl    a1@(EVEC_BUSERR),d1     | Save old bus error vector.
        movl    #BEhand,a1@(EVEC_BUSERR) | Set up bus error vector
        movl    sp,a1            | Save stack pointer
	movb	a7@(11),a0@	| Write a byte
	moveq	#0,d0		| It worked; return 0 as result.
	bra	BEexit
|
|	set_evec.s -- Set an execption vector and return the old one
|
|	int (*)()
|	set_evec(offset, func)
|		int offset;		/* Offset to vector, eg 8 for berr */
|		int (*func)();		/* Function to call for it */
|
_set_evec:
	movc	vbr,a0			| Get current vector base
	addl	sp@(4),a0		| Add desired exception vector offset
	movl	a0@,d0			| Return old value.
	movl	sp@(8),a0@		| Set new value.
	rts
|
| getidprom(addr, size)
|
| Read back <size> bytes of the ID prom and store them at <addr>.
| Typical use:  getidprom(&idprom_struct, sizeof(idprom_struct));
|
_getidprom:
	movl	sp@(4),a0	| address to move ID prom bytes to
	movl	sp@(8),d1	| How many bytes to move
	movl	d2,sp@-		| save a reg
	movc	sfc,d0		| save source func code
	movl	#FC_MAP,d2
	movc	d2,sfc		| set space 3
	lea	IDPROMOFF,a1	| select id prom
	jra	2$		| Enter loop at bottom as usual for dbra
1$:	movsb	a1@+,d2		| get a byte
	movb	d2,a0@+		| save it
2$:	dbra	d1,1$		| and loop
	movc	d0,sfc		| restore sfc
	movl	sp@+,d2		| restore d2
	rts
|**********************
        .data
        .even
ecc_txt:
        .ascii  "CE memory error at %x+%x\12\0"


