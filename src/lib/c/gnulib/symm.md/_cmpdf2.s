	.file	"_cmpdf2.s"
	.data
	.text
	.align	2
	.globl	___cmpdf2
___cmpdf2:
	jmp	.L15
.L14:
	fldl	8(%ebp)
	fcompl	16(%ebp)
	fstsw	%ax
	sahf
	jbe	.L16
	movl	$1,%eax
	jmp	.L13
.L16:
	fldl	8(%ebp)
	fcompl	16(%ebp)
	fstsw	%ax
	sahf
	jae	.L17
	movl	$-1,%eax
	jmp	.L13
.L17:
	movl	$0,%eax
	jmp	.L13
.L13:
	leave
	ret
/USES	%eax
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___cmpdf2;
	.data
