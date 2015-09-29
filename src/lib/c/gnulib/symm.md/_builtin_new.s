	.file	"_builtin_new.s"
	.data
	.text
	.align	2
	.globl	___builtin_new
___builtin_new:
	jmp	.L16
.L15:
	pushl	8(%ebp)
	call	_malloc
	popl	%ecx
	movl	%eax,-4(%ebp)
	movl	$0,%eax
	cmpl	%eax,-4(%ebp)
	jne	.L18
	call	*___new_handler
.L18:
	movl	-4(%ebp),%eax
	jmp	.L14
/REGAL	0	NOFPA	NODBL
/REGAL	9	NOFPA 	AUTO 	-4(%ebp)	4
.L14:
	leave
	ret
/USES	%eax
.L16:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$4,%esp
	jmp	.L15
/DEF	___builtin_new;
	.data
