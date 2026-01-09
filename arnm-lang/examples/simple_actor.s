	.text
	.globl Counter_init
	.type Counter_init, @function
Counter_init:
	pushq %rbp
	movq %rsp, %rbp
	subq $272, %rsp
.LCounter_init_BB_0:
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -8(%rbp)
	movq $0, %rax
	movq -8(%rbp), %rbx
	movq %rax, (%rbx)
	movq %rbp, %rsp
	popq %rbp
	ret
	.globl Counter_behavior
	.type Counter_behavior, @function
Counter_behavior:
	pushq %rbp
	movq %rsp, %rbp
	subq $272, %rsp
.LCounter_behavior_BB_0:
	movq $0, %rdi
	call arnm_receive
	movq %rax, -8(%rbp)
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %rax
	movq -16(%rbp), %rbx
	movq %rax, (%rbx)
	movq %rbp, %rsp
	popq %rbp
	ret
	.globl _arnm_main
	.type _arnm_main, @function
_arnm_main:
	pushq %rbp
	movq %rsp, %rbp
	subq $272, %rsp
.Lmain_BB_0:
	movq $Counter_init, %rdi
	movq $0, %rsi
	call arnm_spawn
	movq %rax, -8(%rbp)
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %rax
	movq -16(%rbp), %rbx
	movq %rax, (%rbx)
	movq %rbp, %rsp
	popq %rbp
	ret
	.section .note.GNU-stack,"",@progbits
