	.file	"_umodsi3.s"
	.data
	.text
	.align	2
	.globl	___umodsi3
___umodsi3:
	jmp	.L15
.L14:
	movl	8(%ebp),%eax
	xorl	%edx,%edx
	divl	12(%ebp)
	movl	%edx,%eax
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
/DEF	___umodsi3;
	.data
