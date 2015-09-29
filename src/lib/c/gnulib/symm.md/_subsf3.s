	.file	"_subsf3.s"
	.data
	.text
	.align	2
	.globl	___subsf3
___subsf3:
	jmp	.L15
.L14:
	flds	12(%ebp)
	flds	8(%ebp)
	fsubp	%st,%st(1)
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
/DEF	___subsf3;
	.data
