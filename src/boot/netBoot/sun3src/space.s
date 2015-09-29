|
|	@(#)space.s 1.1 86/09/27
|	Copyright (c) 1985 by Sun Microsystems, Inc.
|	space.s -- the final frontier.
|
|	This module provides access from 'C' to the movs instruction
|	on the 68010/20.  It could be provided with an asm.sed script but
|	those don't always work and are horrible.
|
|	Better actual idea is to change the 'C' optimizer to do this
|	if you feed it a flag.
|
|	We save and restore the function code control registers since
|	we might be getting called from an interrupt routine, and don't
|	want to either (1) force interrupt routines to save & restore
|	these regs, or (2) clobber the mainline's registers.
|
|	The store routines also clear the associated instruction cache
|	entry, to allow users to store into the instruction stream using
|	Monitor commands (and to allow the breakpoint command to work).
|

#include "../sun3/assym.h"
#include "../h/cache.h"

	.globl	_getsb, _getsw, _getsl
	.globl	_putsb, _putsw, _putsl
	.globl	_cache_dei, _vac_ctxflush
	.globl	_vac_pageflush,_vac_segflush
	.globl	_vac_flush_all


_getsb:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	moveq	#0,d0		| Clear result register
	movsb	a0@,d0		| Pick up a byte
	movc	d1,sfc		| Restore old source function code
	rts

_getsw:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	moveq	#0,d0		| Clear result register
	movsw	a0@,d0		| Pick up a word
	movc	d1,sfc		| Restore old source function code
	rts

_getsl:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movsl	a0@,d0		| Pick up a long
	movc	d1,sfc		| Restore old source function code
	rts

_putsb:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsb	d0,a0@		| Store a byte
	movc	d1,dfc		| Restore old dest function code
	movc	caar,a1		| Save old caar
	movc	a0,caar		| Set up to clear cache entry
	movc	cacr,d1		
	orb	#CACR_CLEAR_ENT,d1
	movc	d1,cacr		| Do it!
	movc	a1,caar		| Restore old caar
	rts

_putsw:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsw	d0,a0@		| Store a word
	movc	d1,dfc		| Restore old dest function code
	movc	caar,a1		| Save old caar
	movc	a0,caar		| Set up to clear cache entry
	movc	cacr,d1		
	orb	#CACR_CLEAR_ENT,d1
	movc	d1,cacr		| Do it!
	movc	a1,caar		| Restore old caar
	rts

_putsl:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsl	d0,a0@		| Store a long
	movc	d1,dfc		| Restore old dest function code
	movc	caar,a1		| Save old caar
	movc	a0,caar		| Set up to clear cache entry
	movc	cacr,d1		
	orb	#CACR_CLEAR_ENT,d1
	movc	d1,cacr		| Do it!
	movc	a1,caar		| Restore old caar
	rts
|********************************************************
| temp place for putting cache subr's
| VAC (virtual Address Cache) Flush All.
| We issue the flush all command VAC_FLUSH_ALL_COUNT times.
| starting at virtual address 0.
| We flush all valid cache data back to memory in all contexts.
|
| Calling function:     vac_flush_all()

_vac_flush_all:
        movc    dfc,a1                  | save dfc
        movl    a1,sp@-
        movc    sfc,a0                  | save sfc
        movl    a0,sp@-
        moveq   #FC_MAP,d0
        movc    d0,dfc
        movc    d0,sfc
        lea     VAC_FLUSH_BASE,a0       | set to flush base address
        moveq   #VAC_FLUSH_ALL_COUNT,d0 | loop this many times
        movb    #VAC_FLUSHALL,d1        | page flush command
10$:
        movsb   d1,a0@                  | flush a set of addresses
        addl    #VAC_FLUSH_INCRMNT,a0   | address of next flush
        dbra    d0,10$
        movsb   ENABLEOFF,d0            | read sys enable reg
        bclr    #ENA_CACHE_BIT,d0
        movsb   d0,ENABLEOFF            | disable the cache
        jsr     _vac_init               | invalidate the cache
        movl    sp@+,a0                 | restore sfc
        movc    a0,sfc
        movl    sp@+,a1
        movc    a1,dfc                  | restore dfc
        rts

|
| VAC (Virtual Address Cache) Flush by context match.
| We have to issue the context flush command VAC_CTXFLUSH_COUNT times.
| Each time we increment the flush address by VAC_FLUSH_INCRMNT.
| Context number being flushed is that stored in the context register
| at the time.
|
| Calling function:     vac_ctxflush()

_vac_ctxflush:
	movc	dfc,d3			| save current dfc
	moveq	#FC_MMU,d0		| samo mmu stuff
	movc	d0,dfc
| now get the ctxt to flush
	movl	sp@(4),d0
	lea	CX_OFF,a0		| ctxt reg offset	
	movsb	a0@,d2			|save current ctxt 
	movsb	d0,a0@			| set it to new ctxt
|
        movl    #VAC_FLUSH_BASE,a0      | get flush command base address
        movl    #VAC_CTXFLUSH_COUNT,d0  | loop this many times
        movb    #VAC_CTXFLUSH,d1        | context flush command
10$:
        movsb   d1,a0@                  | flush a set of addresses
        addl    #VAC_FLUSH_INCRMNT,a0   | next address base
        dbra    d0,10$                  | decrement count
| restore ctxt and dfc
	movc	d3,dfc
	lea	CX_OFF,a0
	movsb	d2,a0@
| ok
        rts

| VAC (virtual Address Cache) Flush by page match.
| We issue the page flush command VAC_PAGEFLUSH_COUNT times.
|
|
|
| Calling function:     vac_pageflush(ctxt,vaddr)  where vaddr is virtual base
|                                             address.

_vac_pageflush:
        movc    dfc,d3                  | save current dfc
        moveq   #FC_MMU,d0              | samo mmu stuff
        movc    d0,dfc
| now get the ctxt to flush
        movl    sp@(4),d0
        lea     CX_OFF,a0               | ctxt reg offset
        movsb   a0@,d2                  |save current ctxt  
        movsb   d0,a0@                  | set it to new ctxt
|
        movl    sp@(8),d0               | get vaddr of this page
        andl    #PAGEADDRBITS,d0        | clear extraneous bits
        orl     #VAC_FLUSH_BASE,d0      | set to page flush base address
        movl    d0,a0
        moveq   #VAC_PAGEFLUSH_COUNT,d0 | loop this many times
        movb    #VAC_PAGEFLUSH,d1       | page flush command
10$:
        movsb   d1,a0@                  | flush a set of addresses
        addl    #VAC_FLUSH_INCRMNT,a0   | address of next page flush
        dbra    d0,10$
| restore ctxt and dfc
        movc    d3,dfc
        lea     CX_OFF,a0
        movsb    d2,a0@
| ok
        rts

| VAC (Virtual Address Cache) flush by segment match.
| We issue the segment fluish command VAC_SEGFLUSH_COUNT times.
| Each time we incremtnt flush address by VAC_FLUSH_INCRMNT.
|
| Calling function:     vac_segflush(ctxt,segno)
| u_int segno;
|

_vac_segflush:
        movc    dfc,d3                  | save current dfc
        moveq   #FC_MMU,d0              | samo mmu stuff
        movc    d0,dfc
| now get the ctxt to flush
        movl    sp@(4),d0
        lea     CX_OFF,a0               | ctxt reg offset
        movsb   a0@,d2                  |save current ctxt
        movsb   d0,a0@                  | set it to new ctxt
|

        movl    sp@(8),d0               | get segment number
        moveq   #SGSHIFT,d1             | segment shift count
        lsll    d1,d0                   | convert segment # to virt addr
        orl     #VAC_FLUSH_BASE,d0      | set to set flush base addr

        movl    d0,a0
        movl    #VAC_SEGFLUSH_COUNT,d0  | loop this many times
        movb    #VAC_SEGFLUSH,d1        | segment flush command
10$:
        movsb   d1,a0@                  | flush a set of addresses
        addl    #VAC_FLUSH_INCRMNT,a0   | next address base
        dbra    d0,10$
| restore ctxt and dfc
        movc    d3,dfc
        lea     CX_OFF,a0
        movsb    d2,a0@
| ok
        rts
 
| Initialize the VAC by invalidating all cache tags.
| We loop thru all cache tag addresses to clear the valid bit of each
| cache block.
|
| Calling function:     vac_init()
|
 
_vac_init:
        movl    #VAC_TAGS_BASE,a0      | (was RWTAGS)set to tag base address
        movl    #VAC_RWTAG_COUNT,d0     | count for whole cache
        clrl    d1
10$:
        movsl   d1,a0@                  | invalid tags for a cache block
        addl    #VAC_RWTAG_INCRMNT,a0   | address of next cache block
        dbra    d0,10$
        rts

|  do the "n" commands
_cache_dei:
        moveq   #FC_MMU,d0
        movc    d0,sfc                  | default space
        movc    d0,dfc
|
        movl    sp@(4),d0                | its either D  E  or I
        cmpb    #0x49,d0		| is it I ?
        jeq     cache_i
        cmpb    #0x44,d0                 | disable?
        jeq     cache_d
        cmpb    #0x45,d0		| is it E nable ?
        jne     cache_o                 |cant figure out
cache_e:
	movsb	ENABLE_REG,d0
	bset	#ENA_CACHE_BIT,d0
        movsb   d0,ENABLE_REG
        bra     cache_o
cache_d:
	movsb   ENABLE_REG,d0
	bclr	#ENA_CACHE_BIT,d0 
        movsb   d0,ENABLE_REG
| all return from here
cache_o:
        rts
|
cache_i:
	jsr	_vac_init		| invalidate the cache
        bra     cache_o
|*************************************************



