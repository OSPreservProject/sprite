	.file	"_lshlsi3.s"
	.data
	.text
	.align	2
	.globl	___lshlsi3
___lshlsi3:
	jmp	.L15
.L14:
	movl	12(%ebp),%ecx
	movl	8(%ebp),%eax
	shll	%cl,%eax
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
/DEF	___lshlsi3;
	.data
