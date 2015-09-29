	.file	"_adddf3.s"
	.data
	.text
	.align	2
	.globl	___adddf3
___adddf3:
	jmp	.L15
.L14:
	fldl	8(%ebp)
	faddl	16(%ebp)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___adddf3;
	.data
