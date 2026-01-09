	.text
	.globl Counter_init
	.type Counter_init, @function
Counter_init:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 304, Param count: 1
	subq $304, %rsp
	movq %rdi, -8(%rbp)
.LCounter_init_BB_0:
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %rax
	movq -16(%rbp), %rbx
	movq %rax, (%rbx)
	call arnm_self
	movq %rax, -24(%rbp)
	movq -24(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -32(%rbp)
	movq -32(%rbp), %rax
	movq %rax, -40(%rbp)
	movq -16(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -48(%rbp)
	movq -48(%rbp), %rax
	movq -40(%rbp), %rbx
	movq %rax, (%rbx)
	movq %rbp, %rsp
	popq %rbp
	ret
	.globl Counter_get
	.type Counter_get, @function
Counter_get:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 288, Param count: 0
	subq $288, %rsp
.LCounter_get_BB_0:
	call arnm_self
	movq %rax, -8(%rbp)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -16(%rbp)
	movq -16(%rbp), %rax
	movq %rax, -24(%rbp)
	movq -24(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -32(%rbp)
	movq -32(%rbp), %rax
	movq %rbp, %rsp
	popq %rbp
	ret
	.globl _arnm_main
	.type _arnm_main, @function
_arnm_main:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 272, Param count: 0
	subq $272, %rsp
.Lmain_BB_0:
	movq $Counter_init, %rdi
	movq $100, %rsi
	movq $8, %rdx
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
