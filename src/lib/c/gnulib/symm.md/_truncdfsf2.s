	.file	"_truncdfsf2.s"
	.data
	.text
	.align	2
	.globl	___truncdfsf2
___truncdfsf2:
	jmp	.L15
.L14:
	fldl	8(%ebp)
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
/DEF	___truncdfsf2;
	.data
