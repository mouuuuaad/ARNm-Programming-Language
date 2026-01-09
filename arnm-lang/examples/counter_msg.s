	.text
	.globl Counter_behavior
	.type Counter_behavior, @function
Counter_behavior:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 400, Param count: 0
	subq $400, %rsp
.LCounter_behavior_BB_0:
	jmp .LCounter_behavior_BB_1
.LCounter_behavior_BB_1:
	movq $0, %rdi
	call arnm_receive
	movq %rax, -8(%rbp)
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -24(%rbp)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -32(%rbp)
	movq -32(%rbp), %rax
	movq -16(%rbp), %rbx
	movq %rax, (%rbx)
	call arnm_self
	movq %rax, -40(%rbp)
	movq -40(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -48(%rbp)
	movq -48(%rbp), %rax
	movq %rax, -56(%rbp)
	call arnm_self
	movq %rax, -64(%rbp)
	movq -64(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -72(%rbp)
	movq -72(%rbp), %rax
	movq %rax, -80(%rbp)
	movq -80(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -88(%rbp)
	movq -16(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -96(%rbp)
	movq -88(%rbp), %rax
	addq -96(%rbp), %rax
	movq %rax, -104(%rbp)
	movq -104(%rbp), %rax
	movq -56(%rbp), %rbx
	movq %rax, (%rbx)
	call arnm_self
	movq %rax, -112(%rbp)
	movq -112(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -120(%rbp)
	movq -120(%rbp), %rax
	movq %rax, -128(%rbp)
	movq -128(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -136(%rbp)
	movq -136(%rbp), %rdi
	call arnm_print_int
	jmp .LCounter_behavior_BB_1
	.globl Counter_init
	.type Counter_init, @function
Counter_init:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 320, Param count: 0
	subq $320, %rsp
.LCounter_init_BB_0:
	call arnm_self
	movq %rax, -8(%rbp)
	movq -8(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -16(%rbp)
	movq -16(%rbp), %rax
	movq %rax, -24(%rbp)
	movq $0, %rax
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
	call Counter_behavior
	movq %rbp, %rsp
	popq %rbp
	ret
	.globl _arnm_main
	.type _arnm_main, @function
_arnm_main:
	pushq %rbp
	movq %rsp, %rbp
	# Stack size: 304, Param count: 0
	subq $304, %rsp
.Lmain_BB_0:
	movq $Counter_init, %rdi
	movq $0, %rsi
	movq $8, %rdx
	call arnm_spawn
	movq %rax, -8(%rbp)
	subq $16, %rsp
	movq %rsp, %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %rax
	movq -16(%rbp), %rbx
	movq %rax, (%rbx)
	movq -16(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -24(%rbp)
	movq -24(%rbp), %rdi
	movq $10, %rsi
	movq $0, %rdx
	movq $0, %rcx
	call arnm_send
	movq -16(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -32(%rbp)
	movq -32(%rbp), %rdi
	movq $20, %rsi
	movq $0, %rdx
	movq $0, %rcx
	call arnm_send
	movq -16(%rbp), %rax
	movq (%rax), %rbx
	movq %rbx, -40(%rbp)
	movq -40(%rbp), %rdi
	movq $30, %rsi
	movq $0, %rdx
	movq $0, %rcx
	call arnm_send
	movq %rbp, %rsp
	popq %rbp
	ret
	.section .note.GNU-stack,"",@progbits
