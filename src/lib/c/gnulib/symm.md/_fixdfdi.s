	.file	"_fixdfdi.s"
	.data
	.text
	.align	2
	.globl	___fixdfdi
___fixdfdi:
	jmp	.L15
.L14:
	fstcw	-12(%ebp)
	movw	-12(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-10(%ebp)
	fldcw	-10(%ebp)
	fldl	8(%ebp)
	fistpl	-20(%ebp)
	fldcw	-12(%ebp)
	movl	-20(%ebp),%eax
	movl	%eax,-8(%ebp)
	fstcw	-12(%ebp)
	movw	-12(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-10(%ebp)
	fldcw	-10(%ebp)
	fldl	8(%ebp)
	fistpl	-20(%ebp)
	fldcw	-12(%ebp)
	movl	-20(%ebp),%eax
	cmpl	$0,%eax
	jge	.L16
	movl	$-1,%eax
	jmp	.L17
.L16:
	movl	$0,%eax
.L17:
	movl	%eax,-4(%ebp)
	fldl	-8(%ebp)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$20,%esp
	jmp	.L14
/DEF	___fixdfdi;
	.data
