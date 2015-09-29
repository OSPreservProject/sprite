gcc_compiled.:
.data
	.align 4
_G_inTclFPMath:
	.word 0
	.align 4
_G_gotTclFPMathErr:
	.word 0
.text
LC0:
	.ascii "domain\0"
.data
	.align 8
LC1:
	.double 0r99e999
.text
LC2:
	.ascii "overflow\0"
.data
	.align 8
LC3:
	.double 0r-99e999
.text
LC4:
	.ascii "underflow\0"
LC5:
	.ascii "floating point \0"
LC6:
	.ascii " error\0"
	.align 4
	.proc 1
_ReturnIEEEMathError:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	st %g0,[%fp-16]
	st %g0,[%fp-12]
	mov %i0,%o0
	st %i2,[%fp-4]
	st %i1,[%fp-8]
	ldd [%fp-8],%f4
	fcmped %f4,%f4
	nop
	fbe L2
	nop
	sethi %hi(LC0),%o2
	b L3
	or %o2,%lo(LC0),%o2
L2:
	sethi %hi(LC1),%g1
	ldd [%g1+%lo(LC1)],%f2
	fcmped %f4,%f2
	nop
	fbule L4
	nop
	sethi %hi(LC2),%o2
	b L3
	or %o2,%lo(LC2),%o2
L4:
	sethi %hi(LC3),%g1
	ldd [%g1+%lo(LC3)],%f2
	fcmped %f4,%f2
	nop
	fbuge L3
	nop
	sethi %hi(LC4),%o2
	or %lo(LC4),%o2,%o2
L3:
	sethi %hi(LC5),%o1
	or %lo(LC5),%o1,%o1
	sethi %hi(LC6),%o3
	or %lo(LC6),%o3,%o3
	call _Tcl_AppendResult,0
	mov 0,%o4
	mov 1,%i0
	ret
	restore
	.align 4
.global _Tcl_MathError
	.proc 1
_Tcl_MathError:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o1
	sethi %hi(_G_inTclFPMath),%g1
	ld	[%g1+%lo(_G_inTclFPMath)],%o0
	tst %o0
	be L8
	nop
	mov 1,%o2
	sethi %hi(_G_gotTclFPMathErr),%g1
	st	%o2,[%g1+%lo(_G_gotTclFPMathErr)]
	sethi %hi(_G_errorType),%g1
	st	%o1,[%g1+%lo(_G_errorType)]
	b L7
	mov 1,%i0
L8:
	mov 0,%i0
L7:
	ret
	restore
LC7:
	.ascii " expr\0"
.data
	.align 8
LC8:
	.double 0r-99e999
	.align 8
LC9:
	.double 0r99e999
.text
	.align 4
	.proc 1
_Tcl_UnaryFloatFunction:
	!#PROLOGUE# 0
	save %sp,-120,%sp
	!#PROLOGUE# 1
	st %g0,[%fp-16]
	st %g0,[%fp-12]
	mov %i0,%l0
	mov %i2,%o2
	mov %i3,%l1
	cmp %i1,2
	be L11
	nop
	mov %l0,%o0
	sethi %hi(_tclXWrongArgs),%g1
	ld	[%g1+%lo(_tclXWrongArgs)],%o1
	ld [%o2],%o2
	sethi %hi(LC7),%o3
	or %lo(LC7),%o3,%o3
	call _Tcl_AppendResult,0
	mov 0,%o4
	b L10
	mov 1,%i0
L11:
	mov %l0,%o0
	ld [%o2+4],%o1
	call _Tcl_ExprDouble,0
	add %fp,-24,%o2
	tst %o0
	be L12
	nop
	b L10
	mov 1,%i0
L12:
	mov 1,%o5
	sethi %hi(_G_inTclFPMath),%g1
	st	%o5,[%g1+%lo(_G_inTclFPMath)]
	ldd [%fp-24],%o0
	call %l1,0
	nop
	fmovs %f0,%f4
	fmovs %f1,%f5
	sethi %hi(_G_inTclFPMath),%g1
	st	%g0,[%g1+%lo(_G_inTclFPMath)]
	fcmped %f4,%f4
	nop
	fbne L14
	nop
	sethi %hi(LC8),%g1
	ldd [%g1+%lo(LC8)],%f2
	fcmped %f4,%f2
	nop
	fbl L14
	nop
	sethi %hi(LC9),%g1
	ldd [%g1+%lo(LC9)],%f2
	fcmped %f4,%f2
	nop
	fbule L13
	nop
L14:
	mov %l0,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _ReturnIEEEMathError,0
	nop
	b L10
	mov %o0,%i0
L13:
	mov %l0,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _Tcl_ReturnDouble,0
	nop
	mov 0,%i0
L10:
	ret
	restore
	.align 4
.global _Tcl_AcosCmd
	.proc 1
_Tcl_AcosCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_acos),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_acos),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_AsinCmd
	.proc 1
_Tcl_AsinCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_asin),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_asin),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_AtanCmd
	.proc 1
_Tcl_AtanCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_atan),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_atan),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_CosCmd
	.proc 1
_Tcl_CosCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_cos),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_cos),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_SinCmd
	.proc 1
_Tcl_SinCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_sin),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_sin),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_TanCmd
	.proc 1
_Tcl_TanCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_tan),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_tan),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_CoshCmd
	.proc 1
_Tcl_CoshCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_cosh),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_cosh),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_SinhCmd
	.proc 1
_Tcl_SinhCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_sinh),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_sinh),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_TanhCmd
	.proc 1
_Tcl_TanhCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_tanh),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_tanh),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_ExpCmd
	.proc 1
_Tcl_ExpCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_exp),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_exp),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_LogCmd
	.proc 1
_Tcl_LogCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_log),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_log),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_Log10Cmd
	.proc 1
_Tcl_Log10Cmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_log10),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_log10),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_SqrtCmd
	.proc 1
_Tcl_SqrtCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_sqrt),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_sqrt),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_FabsCmd
	.proc 1
_Tcl_FabsCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_fabs),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_fabs),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_FloorCmd
	.proc 1
_Tcl_FloorCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_floor),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_floor),%o3
	mov %o0,%i0
	ret
	restore
	.align 4
.global _Tcl_CeilCmd
	.proc 1
_Tcl_CeilCmd:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i1,%o0
	mov %i2,%o1
	mov %i3,%o2
	sethi %hi(_ceil),%o3
	call _Tcl_UnaryFloatFunction,0
	or %o3,%lo(_ceil),%o3
	mov %o0,%i0
	ret
	restore
LC10:
	.ascii " expr divisor\0"
.data
	.align 8
LC11:
	.double 0r-99e999
	.align 8
LC12:
	.double 0r99e999
.text
	.align 4
.global _Tcl_FmodCmd
	.proc 1
_Tcl_FmodCmd:
	!#PROLOGUE# 0
	save %sp,-128,%sp
	!#PROLOGUE# 1
	st %g0,[%fp-16]
	st %g0,[%fp-12]
	mov %i1,%l1
	mov %i3,%l0
	cmp %i2,3
	be L32
	nop
	mov %l1,%o0
	sethi %hi(_tclXWrongArgs),%g1
	ld	[%g1+%lo(_tclXWrongArgs)],%o1
	ld [%l0],%o2
	sethi %hi(LC10),%o3
	or %lo(LC10),%o3,%o3
	call _Tcl_AppendResult,0
	mov 0,%o4
L38:
	b L31
	mov 1,%i0
L32:
	mov %l1,%o0
	ld [%l0+4],%o1
	call _Tcl_ExprDouble,0
	add %fp,-24,%o2
	tst %o0
	bne L38
	nop
	mov %l1,%o0
	ld [%l0+8],%o1
	call _Tcl_ExprDouble,0
	add %fp,-32,%o2
	tst %o0
	bne L38
	nop
	mov 1,%o5
	sethi %hi(_G_inTclFPMath),%g1
	st	%o5,[%g1+%lo(_G_inTclFPMath)]
	ldd [%fp-24],%o0
	ldd [%fp-32],%o2
	call _fmod,0
	nop
	fmovs %f0,%f4
	fmovs %f1,%f5
	sethi %hi(_G_inTclFPMath),%g1
	st	%g0,[%g1+%lo(_G_inTclFPMath)]
	fcmped %f4,%f4
	nop
	fbne L36
	nop
	sethi %hi(LC11),%g1
	ldd [%g1+%lo(LC11)],%f2
	fcmped %f4,%f2
	nop
	fbl L36
	nop
	sethi %hi(LC12),%g1
	ldd [%g1+%lo(LC12)],%f2
	fcmped %f4,%f2
	nop
	fbule L35
	nop
L36:
	mov %l1,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _ReturnIEEEMathError,0
	nop
	b L31
	mov %o0,%i0
L35:
	mov %l1,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _Tcl_ReturnDouble,0
	nop
	mov 0,%i0
L31:
	ret
	restore
LC13:
	.ascii " expr exp\0"
.data
	.align 8
LC14:
	.double 0r-99e999
	.align 8
LC15:
	.double 0r99e999
.text
	.align 4
.global _Tcl_PowCmd
	.proc 1
_Tcl_PowCmd:
	!#PROLOGUE# 0
	save %sp,-128,%sp
	!#PROLOGUE# 1
	st %g0,[%fp-16]
	st %g0,[%fp-12]
	mov %i1,%l1
	mov %i3,%l0
	cmp %i2,3
	be L40
	nop
	mov %l1,%o0
	sethi %hi(_tclXWrongArgs),%g1
	ld	[%g1+%lo(_tclXWrongArgs)],%o1
	ld [%l0],%o2
	sethi %hi(LC13),%o3
	or %lo(LC13),%o3,%o3
	call _Tcl_AppendResult,0
	mov 0,%o4
L46:
	b L39
	mov 1,%i0
L40:
	mov %l1,%o0
	ld [%l0+4],%o1
	call _Tcl_ExprDouble,0
	add %fp,-24,%o2
	tst %o0
	bne L46
	nop
	mov %l1,%o0
	ld [%l0+8],%o1
	call _Tcl_ExprDouble,0
	add %fp,-32,%o2
	tst %o0
	bne L46
	nop
	mov 1,%o5
	sethi %hi(_G_inTclFPMath),%g1
	st	%o5,[%g1+%lo(_G_inTclFPMath)]
	ldd [%fp-24],%o0
	ldd [%fp-32],%o2
	call _pow,0
	nop
	fmovs %f0,%f4
	fmovs %f1,%f5
	sethi %hi(_G_inTclFPMath),%g1
	st	%g0,[%g1+%lo(_G_inTclFPMath)]
	fcmped %f4,%f4
	nop
	fbne L44
	nop
	sethi %hi(LC14),%g1
	ldd [%g1+%lo(LC14)],%f2
	fcmped %f4,%f2
	nop
	fbl L44
	nop
	sethi %hi(LC15),%g1
	ldd [%g1+%lo(LC15)],%f2
	fcmped %f4,%f2
	nop
	fbule L43
	nop
L44:
	mov %l1,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _ReturnIEEEMathError,0
	nop
	b L39
	mov %o0,%i0
L43:
	mov %l1,%o0
	std %f4,[%fp-8]
	ld [%fp-4],%o2
	ld [%fp-8],%o1
	call _Tcl_ReturnDouble,0
	nop
	mov 0,%i0
L39:
	ret
	restore

.reserve _G_errorType,8,"bss"
