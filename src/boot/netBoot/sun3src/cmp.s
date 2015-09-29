
|
|	@(#)cmp.s 1.1 86/09/27
|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
|	these routines compare memory blocks
|
|	[b|w|l]cmp(address, address, size)
|		uses loop mode access to compare the specified blocks
|		with the proper size access
|

	.text
	.globl	_bcmp, _wcmp, _lcmp

	.globl	bdb_unpack, wdb_unpack, ldb_unpack
	.globl	bdb_pack, wdb_pack, ldb_pack

cmpstart:
	movl	sp@+, a1
	link	a6, #0
	movl	a1, sp@-
	movl	a6@(16), d0		| get count
	tstl	d0			| if count zero, return now
	beqs	cmpend
	movl	a6@(8), a0		| get first block address
	movl	a6@(12), a1		| get second block address
	rts

cmpend:
	unlk	a6
	rts

_bcmp:
	jsr	cmpstart		| set up cmp registers
	jsr	bdb_unpack		| get d0 setup for dbra
1$:	cmpmb	a0@+, a1@+		| compare
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	jsr	bdb_pack		| repack db count
	jra	cmpend			| do cleanup

_wcmp:
	jsr	cmpstart		| set up cmp registers
	jsr	wdb_unpack		| get d0 setup for dbra
1$:	cmpmw	a0@+, a1@+		| compare
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	jsr	wdb_pack		| repack db count
	jra	cmpend			| do cleanup

_lcmp:
	jsr	cmpstart		| set up cmp registers
	jsr	ldb_unpack		| get d0 setup for dbra
1$:	cmpml	a0@+, a1@+		| compare
	dbne	d0, 1$			| decrement until not equal
	dbne	d1, 1$			| decrement until not equal
	jsr	ldb_pack		| repack db count
	jra	cmpend			| do cleanup
