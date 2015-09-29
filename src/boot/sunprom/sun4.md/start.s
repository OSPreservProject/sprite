!
! The following variable MUST be the first thing in this
! file, as its position marks the beginning of small data
!
	.seg	"data"
	.global	_environ		! first symbol in sdata		
_environ:
	.word	0
!
! This variable is used in the DELAY macro.  5 is the right value for
! 68010's running 10MHz.  3 is the right value for 68020's running 16MHz
! with cache on.  (4x as fast at doing the delay loop.)  Other values
! should be empirically determined as needed.  Srt0.s sets this value
! based on the actual runtime environment encountered.
!
! For a sunrise machine with no cache (16Mhz) the delay should be 4
! if the cache is on the delay can be 1
!
! It's critical that the value be no SMALLER than required, e.g. the
! DELAY macro guarantees a MINIMUM delay, not a maximum.
!
	.seg	"data"
	.global	_cpudelay
_cpudelay:
	.word	5			! Multiplier for DELAY macro.

	.seg	"text"
	.align	4
!
! Startup code for standalone system
!
WINDOWSIZE = (16 * 4)

	.global	_end
	.global	_edata
	.global	_main
	.global	__exit
	.global	_Boot_Transfer
	.global	start
start:
	save	%sp, -WINDOWSIZE, %sp	! get a new window, leave room for args
	call	1f			! get the current pc into o7
	nop				! (where entry is currently located)
1:
	mov	%o7, %o0		! save for later
	sub	%o0, 4, %o0		! point to first byte of prog
	set	start+4,%o6		! address of call instruction - above 
	cmp	%o6, %o7		! are they the same
	be	start
	nop
	set	_edata+4, %o2		! end of program, inclusive, except bss
	set	 start, %o1		! beginning of program
	sub	%o2, %o1, %o2		! size of program
	!
	! check that the following copy won't write on itself
	!
	!add	%o7,%o2,%o5
	!cmp	%o5,%o0	
	!bg	cantreloc
	!nop
	!
	! copy program where it belongs
	!
2:
	ld	[%o0], %o3		! read a word
	add	%o0, 4, %o0			! point to next src word
	st	%o3, [%o1]		! write a word
	subcc	%o2,4, %o2		! check if done
	bge	2b			! if not loop
	add	%o1, 4, %o1		! delay slot, point to next dest word
	
	set	go, %g1		! now that it is relocated, jump to it
	jmp	%g1
	nop
	! program is now relocated
go:
	! we should turn on cache at least by now
	set	_end+4, %o2		! end of bss
	set	_edata, %o0		! beginning of bss
	sub	%o2, %o0, %o2		! size of bss
	! zero the bss
1:
	subcc	%o2,1,%o2			! loop to zero bss
	st	%g0, [%o0]
	bnz	1b
	add	%o0, 4, %o0

	! general startup code
	set	(_environ+0x1000), %g7	! 1st global register (etext + 4K)
	set	0x2000, %g6		! 8k
	add	%g7, %g6, %g6		! 2nd global register (1st global + 8K)
	ld	[%sp + WINDOWSIZE], %o0	! argc
	add	%sp, WINDOWSIZE + 4, %o1! argv
	sll	%o0, 2, %o2		! argc * sizeof (int)
	add	%o2, 4, %o2		! skip 0 at end of arg ptrs
	add	%o1, %o2, %o2		! environ ptr
	call	_main
	st	%o2, [%g7 + -0x1000]	! store 1st word of sdata %ad(_environ)
	nop
#ifdef notdef
	call	_exit			! exit(0)
#endif
	mov	0, %o0			! delay slot
__exit:
	ret				! ret to prom
	restore

_Boot_Transfer:
	jmp	%o0
	restore
