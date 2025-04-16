.text
.global add_usermode_gdt_entries
add_usermode_gdt_entries:
    pushq %rbp
    movq %rsp, %rbp
    subq $0xa, %rsp
    
    // Get GDTR
    sgdt -0xa(%rbp)
    
    // Get GDT base
    movq -0x8(%rbp), %rax

    // Add GDT entries 7 and 8 at GDT base
    movq $0x0020fb0000000000, %rdx
    movq %rdx, 0x38(%rax)
    movq $0x0000f30000000000, %rdx
    movq %rdx, 0x40(%rax)
    
    // Increase GDT limit by 2 entries
    addw $0x10, -0xa(%rbp)
    lgdt -0xa(%rbp)

    movq %rbp, %rsp
    popq %rbp
    retq

// void add_tss_gdt_entry(void *tss);
.global add_tss_gdt_entry
add_tss_gdt_entry:
    pushq %rbp
    movq %rsp, %rbp
    subq $0xa, %rsp

    // Get GDTR
    sgdt -0xa(%rbp)

    // Get GDT base
    movq -0x8(%rbp), %rax

    // Add GDT entry 9 (occupies two slots) at GDT base
    movq $0, %rdx
    
    movq %rdi, %rcx
    shr $24, %rcx
    andq $0xFF, %rcx
    addq %rcx, %rdx

    // Granularity = 0
    // AVL = 0
    // Limit bits 19:16 = 0
    shl $8, %rdx
    addq $0x00, %rdx

    // P = 1
    // DPL = 0
    // Type = 9
    shl $8, %rdx
    addq $0x89, %rdx

    // Base address bits 23:16
    shl $8, %rdx
    movq %rdi, %rcx
    shr $16, %rcx
    andq $0xFF, %rcx
    addq %rcx, %rdx
    
    // Base address bits 16:0
    shl $16, %rdx
    movq %rdi, %rcx
    andq $0xFFFF, %rcx
    addq %rcx, %rdx
    
    shl $16, %rdx
    addq $0x68, %rdx

    movq %rdx, 0x48(%rax)

    movq %rdi, %rdx
    shr $32, %rdx
    movq %rdx, 0x50(%rax)
    
    // Increase GDT limit by 2 entries
    addw $0x10, -0xa(%rbp)
    lgdt -0xa(%rbp)
    
    // Load task register
    movw $0x48, %cx
    ltr %cx

    movq %rbp, %rsp
    popq %rbp
    retq
