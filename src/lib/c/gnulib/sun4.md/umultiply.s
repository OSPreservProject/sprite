!
!   umultiply.s
!

        .globl .umul

.umul:          mov	%o0, %y
L_0x4:	        andncc	%o0, 0xf, %o4
L_0x8:          be	L_0xd8
L_0xc:	        sethi	%hi(0xffff0000), %o5
L_0x10:	        andncc	%o0, 0xff, %o4
L_0x14:	        be,a	L_0x114
L_0x18:	        mulscc	%o4, %o1, %o4
L_0x1c:         andncc	%o0, 0xfff, %o4
L_0x20:	        be,a	L_0x15c
L_0x24:         mulscc	%o4, %o1, %o4
L_0x28:	        andcc	%o0, %o5, %o4
L_0x2c:	        be,a	L_0x1b4
L_0x30:	        mulscc	%o4, %o1, %o4
L_0x34: 	andcc	%g0, %g0, %o4
L_0x38:	        mulscc	%o4, %o1, %o4
L_0x3c:	        mulscc	%o4, %o1, %o4
L_0x40: 	mulscc	%o4, %o1, %o4
L_0x44:	        mulscc	%o4, %o1, %o4
L_0x48:	        mulscc	%o4, %o1, %o4
L_0x4c:         mulscc	%o4, %o1, %o4
L_0x50:	        mulscc	%o4, %o1, %o4
L_0x54:	        mulscc	%o4, %o1, %o4
L_0x58: 	mulscc	%o4, %o1, %o4
L_0x5c: 	mulscc	%o4, %o1, %o4
L_0x60: 	mulscc	%o4, %o1, %o4
L_0x64: 	mulscc	%o4, %o1, %o4
L_0x68: 	mulscc	%o4, %o1, %o4
L_0x6c: 	mulscc	%o4, %o1, %o4
L_0x70: 	mulscc	%o4, %o1, %o4
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
L_0xbc: 	orcc	%g0, %o1, %g0
L_0xc0: 	bge	L_0xcc
L_0xc4: 	nop
L_0xc8: 	add	%o4, %o0, %o4
L_0xcc: 	rd	%y, %o0
L_0xd0: 	jmp	%o7 + 0x8
L_0xd4: 	addcc	%o4, %g0, %o1
L_0xd8: 	mulscc	%o4, %o1, %o4
L_0xdc: 	mulscc	%o4, %o1, %o4
L_0xe0: 	mulscc	%o4, %o1, %o4
L_0xe4: 	mulscc	%o4, %o1, %o4
L_0xe8: 	mulscc	%o4, %g0, %o4
L_0xec: 	rd	%y, %o5
L_0xf0: 	orcc	%g0, %o1, %g0
L_0xf4: 	bge	L_0x100
L_0xf8: 	sra	%o4, 0x1c, %o1
L_0xfc: 	add	%o1, %o0, %o1
L_0x100:	sll	%o4, 0x4, %o0
L_0x104:	srl	%o5, 0x1c, %o5
L_0x108:	or	%o5, %o0, %o0
L_0x10c:	jmp	%o7 + 0x8
L_0x110:	orcc	%g0, %o1, %g0
L_0x114:	mulscc	%o4, %o1, %o4
L_0x118:	mulscc	%o4, %o1, %o4
L_0x11c:	mulscc	%o4, %o1, %o4
L_0x120:	mulscc	%o4, %o1, %o4
L_0x124:	mulscc	%o4, %o1, %o4
L_0x128:	mulscc	%o4, %o1, %o4
L_0x12c:	mulscc	%o4, %o1, %o4
L_0x130:	mulscc	%o4, %g0, %o4
L_0x134:	rd	%y, %o5
L_0x138:	orcc	%g0, %o1, %g0
L_0x13c:	bge	L_0x148
L_0x140:	sra	%o4, 0x18, %o1
L_0x144:	add	%o1, %o0, %o1
L_0x148:	sll	%o4, 0x8, %o0
L_0x14c:	srl	%o5, 0x18, %o5
L_0x150:	or	%o5, %o0, %o0
L_0x154:	jmp	%o7 + 0x8
L_0x158:	orcc	%g0, %o1, %g0
L_0x15c:	mulscc	%o4, %o1, %o4
L_0x160:	mulscc	%o4, %o1, %o4
L_0x164:	mulscc	%o4, %o1, %o4
L_0x168:	mulscc	%o4, %o1, %o4
L_0x16c:	mulscc	%o4, %o1, %o4
L_0x170:	mulscc	%o4, %o1, %o4
L_0x174:	mulscc	%o4, %o1, %o4
L_0x178:	mulscc	%o4, %o1, %o4
L_0x17c:	mulscc	%o4, %o1, %o4
L_0x180:	mulscc	%o4, %o1, %o4
L_0x184:	mulscc	%o4, %o1, %o4
L_0x188:	mulscc	%o4, %g0, %o4
L_0x18c:	rd	%y, %o5
L_0x190:	orcc	%g0, %o1, %g0
L_0x194:	bge	L_0x1a0
L_0x198:	sra	%o4, 0x14, %o1
L_0x19c:	add	%o1, %o0, %o1
L_0x1a0:	sll	%o4, 0xc, %o0
L_0x1a4:	srl	%o5, 0x14, %o5
L_0x1a8:	or	%o5, %o0, %o0
L_0x1ac:	jmp	%o7 + 0x8
L_0x1b0:	orcc	%g0, %o1, %g0
L_0x1b4:	mulscc	%o4, %o1, %o4
L_0x1b8:	mulscc	%o4, %o1, %o4
L_0x1bc:	mulscc	%o4, %o1, %o4
L_0x1c0:	mulscc	%o4, %o1, %o4
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
L_0x1f0:	mulscc	%o4, %g0, %o4
L_0x1f4:	rd	%y, %o5
L_0x1f8:	orcc	%g0, %o1, %g0
L_0x1fc:	bge	L_0x208
L_0x200:	sra	%o4, 0x10, %o1
L_0x204:	add	%o1, %o0, %o1
L_0x208:	sll	%o4, 0x10, %o0
L_0x20c:	srl	%o5, 0x10, %o5
L_0x210:	or	%o5, %o0, %o0
L_0x214:	jmp	%o7 + 0x8
L_0x218:	orcc	%g0, %o1, %g0

