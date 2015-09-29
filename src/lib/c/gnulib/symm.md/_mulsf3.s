	.file	"_mulsf3.s"
	.data
	.text
	.align	2
	.globl	___mulsf3
___mulsf3:
	jmp	.L15
.L14:
	flds	12(%ebp)
	flds	8(%ebp)
	fmulp	%st,%st(1)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$12,%esp
	jmp	.L14
/DEF	___mulsf3;
	.data
