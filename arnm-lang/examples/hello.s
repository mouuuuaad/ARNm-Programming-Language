	.text
	.globl _arnm_main
	.type _arnm_main, @function
_arnm_main:
	pushq %rbp
	movq %rsp, %rbp
	subq $288, %rsp
.Lmain_BB_0:
	movq $42, %rdi
	call arnm_print_int
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -8(%rbp)
	movq $10, %rax
	movq -8(%rbp), %rbx
	movq %rax, (%rbx)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -16(%rbp)
	movq -16(%rbp), %rax
	addq $5, %rax
	movq %rax, -24(%rbp)
	movq -24(%rbp), %rdi
	call arnm_print_int
	movq %rbp, %rsp
	popq %rbp
	ret
	.section .note.GNU-stack,"",@progbits
