
|
|	@(#)patt.s 1.1 86/09/27
|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
|	these routines fill and check memory blocks with constant
|	patterns.
|
|	[o_][b|w|l]{p,}fill(address, size, pattern)
|		fills the block at address for size bytes with pattern.
|		fill/check with no prefix for backward compatibility.
|		o_& is dummy routine for fill, for consistentcy.
|	[o_][b|w|l]{p,}check(address, size, pattern)
|		checks to see that the pattern is in the block given.
|		o_& prefix causes check to return obs_value if check
|		fails, but does NOT use loop mode to do so.  Regular
|		version does give loop mode, but no observed value.
|
|	.data
|	.asciz	"@(#)patt.s 1.4 10/12/84 Copyright Sun Micro"
|	.even
	.text
	.globl	_bfill, _wfill, _lfill
	.globl	_bpfill, _wpfill, _lpfill
	.globl	_bcheck, _wcheck, _lcheck
	.globl	_bpcheck, _wpcheck, _lpcheck

	.globl	_o_bcheck, _o_wcheck, _o_lcheck
	.globl	_o_bpcheck, _o_wpcheck, _o_lpcheck

	.globl	bdb_unpack, wdb_unpack, ldb_unpack
	.globl	bdb_pack, wdb_pack, ldb_pack
	.globl	_obs_value, _exp_value

fillstart:
	movl	sp@+, a1		| save return (off stack tho)
	link	a6, #0
	movl	a6@(12), d0		| get count
	tstl	d0			| if count zero, return now
	beqs	fillend2
	movl	d7, sp@-		| save d7
	movl	d6, sp@-		| only used in o_, but WTH
	clrl	d6			| clear obs_value register
	movl	a6@(16), d7		| get pattern into d7
	movl	a6@(8), a0		| get buffer address
	jmp	a1@			| use rts address (unpack now)

fillend:
	movl	d6, _obs_value		| only valid for o_&check
	movl	d7, _exp_value		| valid for any check.
	movl	sp@+, d6
	movl	sp@+, d7
fillend2:
	unlk	a6
	rts

_bfill:
_bpfill:
	jsr	fillstart		| set up fill registers
	jsr	bdb_unpack		| get d0 setup for dbra
1$:	movb	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	fillend			| do cleanup

_wfill:
_wpfill:
	jsr	fillstart		| set up fill registers
	jsr	wdb_unpack		| get d0 setup for dbra
1$:	movw	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	fillend			| do cleanup

_lfill:
_lpfill:
	jsr	fillstart		| set up fill registers
	jsr	ldb_unpack		| get d0 setup for dbra
1$:	movl	d7, a0@+		| stuff into memory
	dbra	d0, 1$			| decrement until not equal
	dbra	d1, 1$			| decrement until not equal
	jra	fillend			| do cleanup
|
|	checks come in two flavors, regular (fast) and o_& (slow)
|	the o_ version returns a real obs_value at the end, and does NOT
|	use loop mode.  The regular is faster with loop mode, but
|	cannot return the real observed value.
|
_bcheck:
_bpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear out expected bits we dont use
1$:	cmpb	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	fillend			| then finish up

_wcheck:
_wpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear out expected bits we dont use
1$:	cmpw	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	fillend			| then finish up

_lcheck:
_lpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	cmpl	a0@+, d7		| check value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	fillend			| then finish up
|
| return observed pattern versions
|
_o_bcheck:
_o_bpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	bdb_unpack
	andl	#0xff, d7		| clear out expected bits we dont use
1$:	movb	a0@+, d6		| check value
	cmpb	d7, d6			| with expected value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	bdb_pack		| construct return
	jra	fillend			| then finish up

_o_wcheck:
_o_wpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	wdb_unpack
	andl	#0xffff, d7		| clear out expected bits we dont use
1$:	movw	a0@+, d6		| check value
	cmpw	d7, d6			| with expected value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	wdb_pack		| construct return
	jra	fillend			| then finish up

_o_lcheck:
_o_lpcheck:
	jsr	fillstart		| get prams (if needed)
	jsr	ldb_unpack
1$:	movl	a0@+, d6		| check value
	cmpl	d7, d6			| with expected value
	dbne	d0, 1$
	dbne	d1, 1$			| loop till value mismatch or done
	jsr	ldb_pack		| construct return
	jra	fillend			| then finish up
