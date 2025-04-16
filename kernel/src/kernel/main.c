#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "arch/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "drivers/font.h"
#include "drivers/pci.h"
#include "drivers/nvme.h"
#include "drivers/tty.h"
#include "drivers/zero.h"
#include "fs/elf.h"
#include "fs/tar.h"
#include "fs/ramfs.h"
#include "fs/sysfs.h"
#include "fs/exfat.h"
#include "fs/vfs.h"
#include "kernel/limine-requests.h"
#include "kernel/scheduler.h"
#include "lib/cstd.h"
#include "lib/limine.h"
#include "lib/list.h"
#include "lib/spinlock.h"
#include "mm/kmem.h"
#include "mm/slab.h"
#include "mm/userspace.h"

struct task_struct dummy_task_struct;

void kt_hw_init_main() {
    for (int i = 0; i < num_nvme_devices; i++) {
        nvme_probe_2(&nvme_devices[i]);
    }
    while (true) {
        task_yield();
    }
}

void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        halt_forever();
    }

    terminal_init_1();
    idt_init();
    kmem_init();
    scheduler_init_1();
    vfs_init();
    ramfs_init();
    sysfs_init();
    exfat_init();
    terminal_init_2();
    zero_init();
    pci_probe();
    userspace_init();

    if (module_request.response->module_count != 1) {
        printk(u8p("Found 0x"));
        printk_uint8(module_request.response->module_count);
        printk(u8p(" limine modules, expected 1\n"));
        halt_forever();
    };

    extract_tar_files(module_request.response->modules[0]->address);

    // asm volatile ("int $0x80" : : "a" (7)); // raise a software interrupt

    // printk("eflags_if: ");
    // printk(are_interrupts_enabled() ? "true\n" : "false\n");

    // unsigned long flags;
    // spin_lock_irqsave(NULL, flags);

    // printk("eflags_if: ");
    // printk(are_interrupts_enabled() ? "true\n" : "false\n");

    // spin_lock_irqrestore(NULL, flags);

    // printk("eflags_if: ");
    // printk(are_interrupts_enabled() ? "true\n" : "false\n");

    add_usermode_gdt_entries();
    add_tss_gdt_entry(&tss);
    memset(&tss, 0, sizeof(tss64_t));
    tss.iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)

    struct task_struct *init_process = task_struct_alloc();
    init_process->pid = pid_counter++;
    init_process->task_state = TS_RUNNING;
    init_process->exit_code = 0;
    strcpy(init_process->name, u8p("init"));
    init_process->kernel_stack_pages = NULL;
    init_process->pml4_page = NULL;
    init_process->kernel_entry_rsp = 0;
    init_list(&init_process->memory_ranges_lh);
    list_add_tail(&init_process->task_struct_le, &task_struct_lh);
    init_list(&init_process->files_lh);

    setup_kernelspace_memory(init_process);
    load_cr3_from(init_process);
    current_task_ts = init_process;

    struct vfs_lookup_result init_lookup_result;
    vfs_resolve(u8p("bin/init"), &init_lookup_result);
    if (init_lookup_result.status != VFS_RESOLVE_SUCCESS_EXISTS) {
        panic(u8p("init binary not found"));
    }
    struct file init_file = {
        .inode = init_lookup_result.inode,
        .offset = 0,
    };
    struct loader_result init_load_result;
    load_elf64(&init_file, &init_load_result);
    uint64_t *init_first_entry_rsp = (uint64_t*)(init_process->kernel_entry_rsp);
    // userspace stack
    *((void**)(init_load_result.user_entry_rsp - 16)) = (void*)(init_load_result.user_entry_rsp - 8);
    *((void**)(init_load_result.user_entry_rsp - 8)) = NULL;

    // stack for iret
    *--init_first_entry_rsp = 0x43; // user mode data selector
    *--init_first_entry_rsp = init_load_result.user_entry_rsp - 16; // user mode rsp
    *--init_first_entry_rsp = 0x202; // user mode rflags
    *--init_first_entry_rsp = 0x3b; // user mode code selector (ring 3 code with bottom 2 bits set for ring 3)
    *--init_first_entry_rsp = init_load_result.user_entry_rip; // user mode rip
    
    // stack for zero_rax_and_iret
    for (uint8_t i = 0; i < IDT_SYSCALL_NUM_SAVED_REGISTERS; i++) {
        // Enter the init program with all registers set to zero
        *--init_first_entry_rsp = 0;
    }

    // return address for switch_to_task
    *--init_first_entry_rsp = (uint64_t)zero_rax_and_iret;

    // stack for switch_to_task
    *--init_first_entry_rsp = 0;
    *--init_first_entry_rsp = 0;
    *--init_first_entry_rsp = 0;
    *--init_first_entry_rsp = 0;
    *--init_first_entry_rsp = 0;
    *--init_first_entry_rsp = 0;
    init_process->kernel_rsp = (uint64_t)init_first_entry_rsp;

    struct task_struct *kt_hw_init = task_struct_alloc();
    kt_hw_init->pid = pid_counter++;
    kt_hw_init->task_state = TS_RUNNING;
    kt_hw_init->exit_code = 0;
    strcpy(kt_hw_init->name, u8p("kt-hw-init"));
    kt_hw_init->kernel_stack_pages = NULL;
    kt_hw_init->pml4_page = NULL;
    kt_hw_init->kernel_entry_rsp = 0;
    init_list(&kt_hw_init->memory_ranges_lh);
    list_add_tail(&kt_hw_init->task_struct_le, &task_struct_lh);
    init_list(&kt_hw_init->files_lh);
    setup_kernelspace_memory(kt_hw_init);
    
    uint64_t *kt_hw_init_first_entry_rsp = (uint64_t*)(kt_hw_init->kernel_entry_rsp);

    // return address for switch_to_task
    *--kt_hw_init_first_entry_rsp = (uint64_t)kt_hw_init_main;

    // stack for switch_to_task
    *--kt_hw_init_first_entry_rsp = 0;
    *--kt_hw_init_first_entry_rsp = 0;
    *--kt_hw_init_first_entry_rsp = 0;
    *--kt_hw_init_first_entry_rsp = 0;
    *--kt_hw_init_first_entry_rsp = 0;
    *--kt_hw_init_first_entry_rsp = 0;
    kt_hw_init->kernel_rsp = (uint64_t)kt_hw_init_first_entry_rsp;
    
    
    current_task_ts = &dummy_task_struct;
    set_segment_registers_for_userspace();

    switch_to_task(init_process); // Never returns
    halt_forever();
}
