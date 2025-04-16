.text

.global handle_exception_0
.type handle_exception_0, @function
handle_exception_0:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $0, %rdi
    mov $0, %rsi // No error code
    mov %rsp, %rdx
    mov $0, %rcx
    call cpu_exception_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

.global handle_exception_8
.type handle_exception_8, @function
handle_exception_8:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $8, %rdi
    mov 0x48(%rsp), %rsi
    mov %rsp, %rdx
    mov $0, %rcx
    call cpu_exception_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    addq $8, %rsp // Pop error code
    iretq

.global handle_exception_13
.type handle_exception_13, @function
handle_exception_13:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $13, %rdi
    mov 0x48(%rsp), %rsi
    mov %rsp, %rdx
    mov $0, %rcx
    call cpu_exception_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    addq $8, %rsp // Pop error code
    iretq

.global handle_exception_14
.type handle_exception_14, @function
handle_exception_14:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $14, %rdi
    mov 0x48(%rsp), %rsi
    mov %rsp, %rdx
    mov %cr2, %rcx
    call cpu_exception_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    addq $8, %rsp // Pop error code
    iretq

.global handle_interrupt_32
.type handle_interrupt_32, @function
handle_interrupt_32:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $32, %rdi
    mov $0, %rsi
    call hw_interrupt_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq


.global handle_interrupt_33
.type handle_interrupt_33, @function
handle_interrupt_33:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $33, %rdi
    mov $0, %rsi
    call hw_interrupt_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

.global handle_interrupt_41
.type handle_interrupt_41, @function
handle_interrupt_41:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $41, %rdi
    mov $0, %rsi
    call hw_interrupt_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

.global handle_interrupt_42
.type handle_interrupt_42, @function
handle_interrupt_42:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $42, %rdi
    mov $0, %rsi
    call hw_interrupt_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq

.global handle_interrupt_43
.type handle_interrupt_43, @function
handle_interrupt_43:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    mov $43, %rdi
    mov $0, %rsi
    call hw_interrupt_handler
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax
    iretq


.global handle_interrupt_128
.global zero_rax_and_iret
.type handle_interrupt_128, @function
handle_interrupt_128:
    // no need to preserve rax
    // todo: push all general purpose register
    // otherwise it's tricky to get at the preserved registers in C code
    push %rcx
    push %rdx
    push %rbx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    mov $128, %rdi // interrupt number
    mov %rsp, %rsi // interrupt rsp (arg1)
    mov 0x48(%rsp), %rdx // syscall number (arg2). Pre-interrupt value of %rdi
    mov 0x50(%rsp), %rcx // arg3. Pre-interrupt value of %rsi
    mov 0x60(%rsp), %r8  // arg4. Pre-interrupt value of %rdx
    mov 0x68(%rsp), %r9  // arg5
    call hw_interrupt_handler
    jmp skip_zero_rax
zero_rax_and_iret:
    mov $0, %rax
skip_zero_rax:
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rbx
    pop %rdx
    pop %rcx
    // no need to restore rax
    iretq
