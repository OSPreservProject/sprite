	.file	"_negsf2.s"
	.data
	.text
	.align	2
	.globl	___negsf2
___negsf2:
	jmp	.L15
.L14:
	flds	8(%ebp)
	fchs
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$4,%esp
	jmp	.L14
/DEF	___negsf2;
	.data
