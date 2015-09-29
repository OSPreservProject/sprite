	.file	"_fixunsdfsi.s"
	.data
	.text
	.align	2
	.globl	___fixunsdfsi
___fixunsdfsi:
	jmp	.L15
.L14:
	fstcw	-4(%ebp)
	movw	-4(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-2(%ebp)
	fldcw	-2(%ebp)
	fldl	8(%ebp)
	fistpl	-12(%ebp)
	fldcw	-4(%ebp)
	movl	-12(%ebp),%eax
	jmp	.L13
.L13:
	leave
	ret
/USES	%eax
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$12,%esp
	jmp	.L14
/DEF	___fixunsdfsi;
	.data
