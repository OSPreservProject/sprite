gcc_compiled.:
.text
	.align 4
	.proc 1
_foo:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(305419896),%i0
	or %lo(305419896),%i0,%i0
	ret
	restore
	.align 4
	.proc 1
_bar:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov 291,%i0
	ret
	restore
	.align 4
	.proc 1
_baz:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov -16,%i0
	ret
	restore
	.align 4
	.proc 1
_bum:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(_x),%i0
	or %lo(_x),%i0,%i0
	ret
	restore
LC0:
	.ascii "foo = %x, bar = %x, baz = %x, bum = %x, &x = %x\12\0"
	.align 4
.global _main
	.proc 1
_main:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _foo,0
	nop
	call _bar,0
	mov %o0,%l0
	call _baz,0
	mov %o0,%l1
	call _bum,0
	mov %o0,%l2
	mov %o0,%o4
	sethi %hi(LC0),%o0
	or %lo(LC0),%o0,%o0
	mov %l0,%o1
	mov %l1,%o2
	mov %l2,%o3
	sethi %hi(_x),%o5
	call _printf,0
	or %o5,%lo(_x),%o5
	ret
	restore
.global _x
.common _x,8,"bss"
