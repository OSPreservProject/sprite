	.file	"_umulsi3.s"
	.data
	.text
	.align	2
	.globl	___umulsi3
___umulsi3:
	jmp	.L15
.L14:
	movl	8(%ebp),%eax
	imull	12(%ebp),%eax
	jmp	.L13
/REGAL	0	NOFPA	NODBL
.L13:
	leave
	ret
/USES	%eax
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___umulsi3;
	.data
