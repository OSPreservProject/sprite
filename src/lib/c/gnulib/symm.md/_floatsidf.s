	.file	"_floatsidf.s"
	.data
	.text
	.align	2
	.globl	___floatsidf
___floatsidf:
	jmp	.L15
.L14:
	fildl	8(%ebp)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___floatsidf;
	.data
