	.seg	"text"			! [internal]
	.proc	4
	.global	___fixunsdfsi
___fixunsdfsi:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	st	%i0,[%fp+68]
	st	%i1,[%fp+72]
	mov	%i0,%o0
	call	__dtou,2
	mov	%i1,%o1
	mov	%o0,%i0
	ret
	restore	%g0,%g0,%g0
	.seg	"data"			! [internal]
