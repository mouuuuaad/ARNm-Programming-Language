	.text
	.globl Counter_init
	.type Counter_init, @function
Counter_init:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 368, Param count: 0
	subq $368, %rsp
.LCounter_init_BB_0:
	call arnm_self
	movq %rax, -8(%rbp)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -16(%rbp)
	movq -16(%rbp), %rax
	movq %rax, -24(%rbp)
	movq $111, %rax
	movq -24(%rbp), %rbx
	movq %rax, (%rbx)
	call arnm_self
	movq %rax, -32(%rbp)
	movq -32(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -40(%rbp)
	movq -40(%rbp), %rax
	movq %rax, -48(%rbp)
	movq -48(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -56(%rbp)
	movq -56(%rbp), %rdi
	call arnm_print_int
	call arnm_self
	movq %rax, -64(%rbp)
	movq -64(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -72(%rbp)
	movq -72(%rbp), %rax
	movq %rax, -80(%rbp)
	movq $222, %rax
	movq -80(%rbp), %rbx
	movq %rax, (%rbx)
	call arnm_self
	movq %rax, -88(%rbp)
	movq -88(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -96(%rbp)
	movq -96(%rbp), %rax
	movq %rax, -104(%rbp)
	movq -104(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -112(%rbp)
	movq -112(%rbp), %rdi
	call arnm_print_int
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
	movq $0, %rsi
	movq $8, %rdx
	call arnm_spawn
	movq %rax, -8(%rbp)
	movq %rbp, %rsp
	popq %rbp
	ret
	.section .note.GNU-stack,"",@progbits
