	.file	"_divdf3.s"
	.data
	.text
	.align	2
	.globl	___divdf3
___divdf3:
	jmp	.L15
.L14:
	fldl	8(%ebp)
	fdivl	16(%ebp)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___divdf3;
	.data
