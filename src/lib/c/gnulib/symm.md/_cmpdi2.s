	.file	"_cmpdi2.s"
	.data
	.text
	.align	2
	.globl	___cmpdi2
___cmpdi2:
	jmp	.L15
.L14:
	movl	20(%ebp),%eax
	cmpl	%eax,12(%ebp)
	jge	.L16
	movl	$0,%eax
	jmp	.L13
.L16:
	movl	20(%ebp),%eax
	cmpl	%eax,12(%ebp)
	jle	.L17
	movl	$2,%eax
	jmp	.L13
.L17:
	movl	16(%ebp),%eax
	cmpl	%eax,8(%ebp)
	jnb	.L18
	movl	$0,%eax
	jmp	.L13
.L18:
	movl	16(%ebp),%eax
	cmpl	%eax,8(%ebp)
	jna	.L19
	movl	$2,%eax
	jmp	.L13
.L19:
	movl	$1,%eax
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
/DEF	___cmpdi2;
	.data
