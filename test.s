	.file	"test.c"
	.text
	.globl	do_test
	.type	do_test, @function
do_test:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$8, 8(%rsp)
	movl	$7, (%rsp)
	movl	$6, %r9d
	movl	$5, %r8d
	movl	$4, %ecx
	movl	$3, %edx
	movl	$2, %esi
	movl	$1, %edi
	call	test
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	do_test, .-do_test
	.ident	"GCC: (Debian 4.8.2-16) 4.8.2"
	.section	.note.GNU-stack,"",@progbits
