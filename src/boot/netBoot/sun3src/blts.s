
|
|	@(#)blts.s 1.1 86/09/27
|	Copyright (c) 1986 by Sun Microsystems, Inc.

	.globl	_bltshort, _setshort

argmsk = 0x0700	| pick up a2, a1, a0 from stacked args
args = 8	| Args at offset 8 from sp
|lo = a0	| First arg in a0 -- low end address
|hi = a1	| 2nd arg in a1 -- high end address
|destlo = a2	| 3rd arg in a2 -- destination low address
|fillval = a2	| 3rd arg in a2 -- fill value for setshort.

|	bltshort(a0, a1, a2);

_bltshort:
	movl	a2,sp@-
	moveml	sp@(args),#argmsk	| args to regs
	movl	a1,d0	| move how many?
	subl	a0,d0
	jeq	done	| none!
	cmpl	a2,a1
	jle	forward
	cmpl	a2,a0
	jlt	backward
	jeq	done

|	forward-moving loop:
forward:
	asrl	#1,d0	| (shorts not bytes)
	subql	#8,d0	| 8 more to do?
	jle	fsmall
fbig:
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	subql	#8,d0	| 8 more to do?
	jge	fbig
fsmall:
	negw	d0	| (8 - cnt&7)
	aslw	#1,d0	| (to words)
	jmp	pc@(2,d0:w)
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
	movw	a0@+,a2@+
done:
	movl	sp@+,a2		| restore saved reg
	rts

|	backward-moving loop:
backward:
	cmpl	a0,a2
	jeq	done
	addl	d0,a2	| (bump destination to end)
	asrl	#1,d0	| (shorts not bytes)
	subql	#8,d0	| 8 more to do?
	jle	bsmall
bbig:
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	subql	#8,d0	| 8 more to do?
	jge	bbig
bsmall:
	negw	d0	| (8 - cnt&7)
	aslw	#1,d0	| (to words)
	jmp	pc@(2,d0:w)
	movw	a1@-,a2@-	|d0 = 0
	movw	a1@-,a2@-	|d0 = 1<<1
	movw	a1@-,a2@-	|d0 = 2<<1, etc
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	movw	a1@-,a2@-
	jra	done

|	setshort(lo, hi, fillval);

_setshort:
	movl	a2,sp@-
	moveml	sp@(args),#argmsk	| args to regs
	movl	a1,d0	| move how many?
	subl	a0,d0
	jeq	sdone	| none!
	asrl	#1,d0	| (shorts not bytes)
	subql	#8,d0	| 8 more to do?
	jle	ssmall
sbig:
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	subql	#8,d0	| 8 more to do?
	jge	sbig
ssmall:
	negw	d0	| (8 - cnt&7)
	aslw	#1,d0	| (to words)
	jmp	pc@(2,d0:w)
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
	movw	a2,a0@+
sdone:
	movl	sp@+,a2		| restore saved reg(s)
	rts
