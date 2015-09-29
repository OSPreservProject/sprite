
        .globl  .mul

.mul:
	        mov	%o0, %y
		andncc	%o0, 0xf, %o4
		be	L_0xe8
L_0xc:  	sethi	%hi(0xffff0000), %o5
L_0x10:	        andncc	%o0, 0xff, %o4
L_0x14:	        be,a	L_0x124
L_0x18: 	mulscc	%o4, %o1, %o4
L_0x1c: 	andncc	%o0, 0xfff, %o4
L_0x20: 	be,a	L_0x16c
L_0x24: 	mulscc	%o4, %o1, %o4
L_0x28: 	andcc	%o0, %o5, %o4
L_0x2c: 	be,a	L_0x1c4
L_0x30: 	mulscc	%o4, %o1, %o4
L_0x34: 	andcc	%g0, %g0, %o4
L_0x38: 	mulscc	%o4, %o1, %o4
L_0x3c: 	mulscc	%o4, %o1, %o4
L_0x40: 	mulscc	%o4, %o1, %o4
L_0x44: 	mulscc	%o4, %o1, %o4
L_0x48: 	mulscc	%o4, %o1, %o4
L_0x4c: 	mulscc	%o4, %o1, %o4
L_0x50: 	mulscc	%o4, %o1, %o4
L_0x54: 	mulscc	%o4, %o1, %o4
L_0x58: 	mulscc	%o4, %o1, %o4
L_0x5c: 	mulscc	%o4, %o1, %o4
L_0x60: 	mulscc	%o4, %o1, %o4
L_0x64: 	mulscc	%o4, %o1, %o4
L_0x68: 	mulscc	%o4, %o1, %o4
L_0x6c: 	mulscc	%o4, %o1, %o4
L_0x70:	        mulscc	%o4, %o1, %o4
L_0x74: 	mulscc	%o4, %o1, %o4
L_0x78: 	mulscc	%o4, %o1, %o4
L_0x7c: 	mulscc	%o4, %o1, %o4
L_0x80: 	mulscc	%o4, %o1, %o4
L_0x84: 	mulscc	%o4, %o1, %o4
L_0x88: 	mulscc	%o4, %o1, %o4
L_0x8c: 	mulscc	%o4, %o1, %o4
L_0x90: 	mulscc	%o4, %o1, %o4
L_0x94: 	mulscc	%o4, %o1, %o4
L_0x98: 	mulscc	%o4, %o1, %o4
L_0x9c: 	mulscc	%o4, %o1, %o4
L_0xa0: 	mulscc	%o4, %o1, %o4
L_0xa4: 	mulscc	%o4, %o1, %o4
L_0xa8: 	mulscc	%o4, %o1, %o4
L_0xac: 	mulscc	%o4, %o1, %o4
L_0xb0: 	mulscc	%o4, %o1, %o4
L_0xb4: 	mulscc	%o4, %o1, %o4
L_0xb8: 	mulscc	%o4, %g0, %o4
L_0xbc: 	orcc	%g0, %o0, %g0
L_0xc0: 	rd	%y, %o0
L_0xc4: 	bge	L_0xd0
L_0xc8: 	orcc	%g0, %o0, %g0
L_0xcc: 	sub	%o4, %o1, %o4
L_0xd0: 	bge	L_0xe0
L_0xd4: 	mov	%o4, %o1
L_0xd8: 	jmp	%o7 + 0x8
L_0xdc: 	cmp	%o1, -0x1
L_0xe0: 	jmp	%o7 + 0x8
L_0xe4: 	addcc	%o1, %g0, %g0
L_0xe8: 	mulscc	%o4, %o1, %o4
L_0xec: 	mulscc	%o4, %o1, %o4
L_0xf0: 	mulscc	%o4, %o1, %o4
L_0xf4: 	mulscc	%o4, %o1, %o4
L_0xf8: 	mulscc	%o4, %g0, %o4
L_0xfc: 	rd	%y, %o5
L_0x100:	sll	%o4, 0x4, %o0
L_0x104:	srl	%o5, 0x1c, %o5
L_0x108:	orcc	%o5, %o0, %o0
L_0x10c:	bge	L_0x11c
L_0x110:	sra	%o4, 0x1c, %o1
L_0x114:	jmp	%o7 + 0x8
L_0x118:	cmp	%o1, -0x1
L_0x11c:	jmp	%o7 + 0x8
L_0x120:	addcc	%o1, %g0, %g0
L_0x124:	mulscc	%o4, %o1, %o4
L_0x128:	mulscc	%o4, %o1, %o4
L_0x12c:	mulscc	%o4, %o1, %o4
L_0x130:	mulscc	%o4, %o1, %o4
L_0x134:	mulscc	%o4, %o1, %o4
L_0x138:	mulscc	%o4, %o1, %o4
L_0x13c:	mulscc	%o4, %o1, %o4
L_0x140:	mulscc	%o4, %g0, %o4
L_0x144:	rd	%y, %o5
L_0x148:	sll	%o4, 0x8, %o0
L_0x14c:	srl	%o5, 0x18, %o5
L_0x150:	orcc	%o5, %o0, %o0
L_0x154:	bge	L_0x164
L_0x158:	sra	%o4, 0x18, %o1
L_0x15c:	jmp	%o7 + 0x8
L_0x160:	cmp	%o1, -0x1
L_0x164:	jmp	%o7 + 0x8
L_0x168:	addcc	%o1, %g0, %g0
L_0x16c:	mulscc	%o4, %o1, %o4
L_0x170:	mulscc	%o4, %o1, %o4
L_0x174:	mulscc	%o4, %o1, %o4
L_0x178:	mulscc	%o4, %o1, %o4
L_0x17c:	mulscc	%o4, %o1, %o4
L_0x180:	mulscc	%o4, %o1, %o4
L_0x184:	mulscc	%o4, %o1, %o4
L_0x188:	mulscc	%o4, %o1, %o4
L_0x18c:	mulscc	%o4, %o1, %o4
L_0x190:	mulscc	%o4, %o1, %o4
L_0x194:	mulscc	%o4, %o1, %o4
L_0x198:	mulscc	%o4, %g0, %o4
L_0x19c:	rd	%y, %o5
L_0x1a0:	sll	%o4, 0xc, %o0
L_0x1a4:	srl	%o5, 0x14, %o5
L_0x1a8:	orcc	%o5, %o0, %o0
L_0x1ac:	bge	L_0x1bc
L_0x1b0:	sra	%o4, 0x14, %o1
L_0x1b4:	jmp	%o7 + 0x8
L_0x1b8:	cmp	%o1, -0x1
L_0x1bc:	jmp	%o7 + 0x8
L_0x1c0:	addcc	%o1, %g0, %g0
L_0x1c4:	mulscc	%o4, %o1, %o4
L_0x1c8:	mulscc	%o4, %o1, %o4
L_0x1cc:	mulscc	%o4, %o1, %o4
L_0x1d0:	mulscc	%o4, %o1, %o4
L_0x1d4:	mulscc	%o4, %o1, %o4
L_0x1d8:	mulscc	%o4, %o1, %o4
L_0x1dc:	mulscc	%o4, %o1, %o4
L_0x1e0:	mulscc	%o4, %o1, %o4
L_0x1e4:	mulscc	%o4, %o1, %o4
L_0x1e8:	mulscc	%o4, %o1, %o4
L_0x1ec:	mulscc	%o4, %o1, %o4
L_0x1f0:	mulscc	%o4, %o1, %o4
L_0x1f4:	mulscc	%o4, %o1, %o4
L_0x1f8:	mulscc	%o4, %o1, %o4
L_0x1fc:	mulscc	%o4, %o1, %o4
L_0x200:	mulscc	%o4, %g0, %o4
L_0x204:	rd	%y, %o5
L_0x208:	sll	%o4, 0x10, %o0
L_0x20c:	srl	%o5, 0x10, %o5
L_0x210:	orcc	%o5, %o0, %o0
L_0x214:	bge	L_0x224
L_0x218:	sra	%o4, 0x10, %o1
L_0x21c:	jmp	%o7 + 0x8
L_0x220:	cmp	%o1, -0x1
L_0x224:	jmp	%o7 + 0x8
L_0x228:	addcc	%o1, %g0, %g0

