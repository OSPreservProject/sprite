	.file	"_builtin_del.s"
	.data
	.text
	.align	2
	.globl	___builtin_delete
___builtin_delete:
	jmp	.L15
.L14:
	cmpl	$0,8(%ebp)
	je	.L16
	pushl	8(%ebp)
	call	_free
	popl	%ecx
.L16:
/REGAL	0	NOFPA	NODBL
.L13:
	leave
	ret
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	jmp	.L14
/DEF	___builtin_delete;
	.data
	.text
	.align	2
	.globl	___builtin_vec_delete
___builtin_vec_delete:
	jmp	.L21
.L20:
	movl	12(%ebp),%eax
	incl	%eax
	movl	%eax,-8(%ebp)
	movl	8(%ebp),%eax
	movl	%eax,-12(%ebp)
	movl	-8(%ebp),%eax
	imull	16(%ebp),%eax
	addl	%eax,8(%ebp)
	movl	$0,-4(%ebp)
	jmp	.L24
.L25:
	movl	16(%ebp),%eax
	subl	%eax,8(%ebp)
	pushl	28(%ebp)
	pushl	8(%ebp)
	call	*20(%ebp)
	addl	$8,%esp
	incl	-4(%ebp)
.L24:
	movl	-8(%ebp),%eax
	cmpl	%eax,-4(%ebp)
	jl	.L25
.L23:
	cmpl	$0,24(%ebp)
	je	.L26
	pushl	-12(%ebp)
	call	_free
	popl	%ecx
.L26:
/REGAL	0	NOFPA	NODBL
/REGAL	63	NOFPA 	AUTO 	-4(%ebp)	4
/REGAL	57	NOFPA 	PARAM	8(%ebp)	4
/REGAL	36	NOFPA 	AUTO 	-8(%ebp)	4
/REGAL	24	NOFPA 	PARAM	16(%ebp)	4
/REGAL	21	NOFPA 	PARAM	28(%ebp)	4
/REGAL	21	NOFPA 	PARAM	20(%ebp)	4
/REGAL	3	NOFPA 	AUTO 	-12(%ebp)	4
.L19:
	leave
	ret
.L21:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$12,%esp
	jmp	.L20
/DEF	___builtin_vec_delete;
	.data
