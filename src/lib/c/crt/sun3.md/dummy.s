	.text
	.globl Vaddd, Vsubd, Vfloord, _ffloat_
Vfloord:jmp	Ffloord:L
Vaddd:	jmp	Faddd:L
Vsubd:	jmp	Fsubd:L
_ffloat_:	rts
