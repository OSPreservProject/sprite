	.file	"_floatdidf.s"
	.data
	.text
	.align	2
	.globl	___floatdidf
___floatdidf:
	jmp	.L15
.L14:
	.data
	.align	2
.L16:
#	.double	0Dx40f0 0000 0000 0000
	.quad	0x40f0000000000000
	.align	2
.L17:
#	.double	0Dx40f0000000000000
	.quad	0x40f0000000000000
	.text
	fildl	12(%ebp)
	fmull	.L17
	fmull	.L16
	fstpl	-8(%ebp)
	fildl	8(%ebp)
	testl	$0x80000000,8(%ebp)
	jz	.L18
	fadds	.zzz_uf_adjust
.L18:
	.data
	.align	2
#.zzz_uf_adjust:	.float	0Fx4f800000
.zzz_uf_adjust:	.long	0x4f800000
	.text
	fstpl	-16(%ebp)
	fldl	-8(%ebp)
	faddl	-16(%ebp)
	jmp	.L13
.L13:
	leave
	ret
/USES	%st(0)
.L15:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$16,%esp
	jmp	.L14
/DEF	___floatdidf;
	.data
