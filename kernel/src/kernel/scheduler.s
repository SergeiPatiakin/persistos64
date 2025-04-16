.text

// .global iret_from_rsp
// iret_from_rsp:
//	mov %rdi, %rsp
//	iretq

.global set_segment_registers_for_userspace
set_segment_registers_for_userspace:
	mov $0x43, %ax // ring 3 data with bottom 2 bits set for ring 3
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	ret

.set TASK_STRUCT_KERNEL_RSP_OFFSET, 8

// switch_to_task(struct task_struct *new_task)
.global switch_to_task
switch_to_task:
	// Save previous task's state
	// (as required by System V ABI)
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	mov %rdi, %r12
	mov current_task_ts, %rbx
	mov %rsp, TASK_STRUCT_KERNEL_RSP_OFFSET(%rbx)
	mov %rdi, current_task_ts						// current_task_ts = new_task
	mov TASK_STRUCT_KERNEL_RSP_OFFSET(%rdi), %rsp

	call load_cr3_from
	mov %r12, %rdi
	call set_tss_for

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp

	retq

