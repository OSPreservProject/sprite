	.file	"_lshrsi3.s"
	.data
	.text
	.align	2
	.globl	___lshrsi3
___lshrsi3:
	jmp	.L15
.L14:
	movl	12(%ebp),%ecx
	movl	8(%ebp),%eax
	shrl	%cl,%eax
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
/DEF	___lshrsi3;
	.data
