	.file	"_cmpsf2.s"
	.data
	.text
	.align	2
	.globl	___cmpsf2
___cmpsf2:
	jmp	.L15
.L14:
	flds	12(%ebp)
	flds	8(%ebp)
	fcompp
	fstsw	%ax
	sahf
	jbe	.L16
	movl	$1,%eax
	jmp	.L13
.L16:
	flds	12(%ebp)
	flds	8(%ebp)
	fcompp
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
	subl	$8,%esp
	jmp	.L14
/DEF	___cmpsf2;
	.data
