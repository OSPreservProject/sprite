	.file	"_negdf2.s"
	.data
	.text
	.align	2
	.globl	___negdf2
___negdf2:
	jmp	.L15
.L14:
	fldl	8(%ebp)
	fchs
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___negdf2;
	.data
