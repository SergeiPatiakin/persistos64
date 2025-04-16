#include <stdint.h>
#include <stdbool.h>
#include "syscall.h"
#include "arch/asm.h"
#include "arch/idt.h"
#include "fs/tar.h"
#include "fs/vfs.h"
#include "fs/exfat.h"
#include "lib/list.h"
#include "kernel/scheduler.h"
#include "kernel/limine-requests.h"
#include "mm/page.h"
#include "mm/userspace.h"
#include "mm/kmem.h"
#include "drivers/tty.h"

#define SYSCALL_WRITE 1
#define SYSCALL_READ 2
#define SYSCALL_EXIT 3
#define SYSCALL_GETPID 4
#define SYSCALL_SCHED_YIELD 5
#define SYSCALL_FORK 6
#define SYSCALL_EXEC 7
#define SYSCALL_BRK 8
#define SYSCALL_WAITPID 9
#define SYSCALL_OPEN 10
#define SYSCALL_CLOSE 11
#define SYSCALL_GETDENTS 12
#define SYSCALL_MKDIR 13
#define SYSCALL_LSEEK 14
#define SYSCALL_FTRUNCATE 15
#define SYSCALL_DUP2 16
#define SYSCALL_GETTASKS 17
#define SYSCALL_KILL 18
#define SYSCALL_SLEEP 19
#define SYSCALL_MOUNT 20

uint64_t handle_syscall(
    uint64_t interrupt_rsp,
    uint64_t syscall_number,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5
) {
    switch(syscall_number) {
        case SYSCALL_WRITE: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            ssize_t write_result = vfs_write(filp, u8p(arg4), arg5); // Unsafe
            return write_result;
        }
        case SYSCALL_READ: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            ssize_t read_result = vfs_read(filp, u8p(arg4), arg5); // Unsafe
            return read_result;
        }
        case SYSCALL_EXIT: {
            current_task_ts->task_state = TS_ZOMBIE;
            current_task_ts->exit_code = arg3;
            task_yield();
            // No return
        }
        case SYSCALL_GETPID: {
            return current_task_ts->pid;
        }
        case SYSCALL_SCHED_YIELD: {
            task_yield();
            return 0;
        }
        case SYSCALL_FORK: {
            struct task_struct *new_process = task_struct_alloc();
            new_process->pid = pid_counter++;
            new_process->task_state = current_task_ts->task_state;
            strcpy(new_process->name, current_task_ts->name);
            new_process->kernel_stack_pages = NULL;
            new_process->pml4_page = NULL;
            new_process->kernel_entry_rsp = 0;
            init_list(&new_process->memory_ranges_lh);
            list_add_tail(&new_process->task_struct_le, &task_struct_lh);
            setup_kernelspace_memory(new_process);
            init_list(&new_process->files_lh);
            
            // Clone memory ranges
            list_for_each(memory_ranges_le, current_task_ts->memory_ranges_lh) {
                struct userspace_memory_range *memory_range = container_of(memory_ranges_le, struct userspace_memory_range, memory_ranges_le);
                struct userspace_memory_range *new_memory_range = userspace_memory_range_alloc();
                new_memory_range->start = memory_range->start;
                new_memory_range->end = memory_range->end;
                new_memory_range->type = memory_range->type;
                list_add_tail(&new_memory_range->memory_ranges_le, &new_process->memory_ranges_lh);
                for (void *page = (void*)(memory_range->start); page < (void*)(memory_range->end); page += PAGE_SIZE) {
                    void *new_kernel_space_address = map_user_page(new_process, page);
                    memcpy(new_kernel_space_address, page, PAGE_SIZE);
                }
            }

            // Clone file descriptors
            list_for_each(files_le, current_task_ts->files_lh) {
                struct file *file = container_of(files_le, struct file, files_le);
                struct file *new_file = file_alloc();
                new_file->fd = file->fd;
                list_add_tail(&new_file->files_le, &new_process->files_lh);
                new_file->inode = file->inode;
                new_file->offset = file->offset;
            }

            uint64_t *kernel_first_entry_rsp = (uint64_t*)(current_task_ts->kernel_entry_rsp);
            uint64_t *kernel_first_entry_rsp_2 = (uint64_t*)(new_process->kernel_entry_rsp);
            // stack for iret
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // user mode data selector
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // user mode rsp
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // user mode rflags. TODO: copy from interrupt_rsp instead
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // user mode code selector (ring 3 code with bottom 2 bits set for ring 3)
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // user mode rip (return address for iret)
            
            // stack for zero_rax_and_iret
            for (uint8_t i = 0; i < IDT_SYSCALL_NUM_SAVED_REGISTERS; i++) {
                // Enter the init program with all registers set to zero
                *--kernel_first_entry_rsp_2 = *((uint64_t*)interrupt_rsp + IDT_SYSCALL_NUM_SAVED_REGISTERS - 1 - i);
            }
            
            // return address for switch_to_task
            *--kernel_first_entry_rsp_2 = (uint64_t)zero_rax_and_iret;
            --kernel_first_entry_rsp; // just_iret

            // stack for switch_to_task
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp; // This is wrong. Should load kernel-mode rbp etc instead. But we probably don't care what they are
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp;
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp;
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp;
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp;
            *--kernel_first_entry_rsp_2 = *--kernel_first_entry_rsp;
            new_process->kernel_rsp = (uint64_t)kernel_first_entry_rsp_2;

            return new_process->pid;
        }
        case SYSCALL_EXEC: {
            void *path_arg = (void*)arg3;
            uint8_t **argv_arg = (void*)arg4;

            void *path_buffer = kpage_alloc(1);
            memset(path_buffer, 0, 4096);
            strcpy(path_buffer, path_arg); // Unsafe

            void *stack_buffer = kpage_alloc(1); // TODO: what if we need more than 1 page?
            memset(stack_buffer, 0, 4096);

            size_t argc = 0;
            while (argv_arg[argc] != NULL) {
                argc++;
            }

            void *stack_end = stack_buffer + 4096;
            void *stack_top = stack_end;

            uint8_t **relocated_arg_pointers = stack_top - (argc + 1) * sizeof(uint8_t*);
            stack_top = relocated_arg_pointers;
            
            for (size_t i = 0; i < argc; i++) {
                size_t arg_strlen = strlen(argv_arg[i]);
                uint8_t *relocated_arg = stack_top - arg_strlen - 1;
                memcpy(relocated_arg, argv_arg[i], arg_strlen + 1);
                stack_top = relocated_arg;
                relocated_arg_pointers[i] = relocated_arg;
            }
            relocated_arg_pointers[argc] = NULL;

            // arg pointer pointer (aligned to 8 bytes)
            uint8_t ***relocated_arg_pointers_pointer = (uint8_t***)((uint64_t)(stack_top - 8) & 0xFFFFFFFFFFFFFFF8);
            *relocated_arg_pointers_pointer = relocated_arg_pointers;
            stack_top = relocated_arg_pointers_pointer;

            void *stack_start = stack_top;
            size_t stack_length = stack_end - stack_start;

            struct vfs_lookup_result lookup_result;
            vfs_resolve(path_buffer, &lookup_result);
            if (lookup_result.status != VFS_RESOLVE_SUCCESS_EXISTS) {
                kpage_free(stack_buffer, 1);
                kpage_free(path_buffer, 1);
                return -1;
            }

            free_userspace_memory(current_task_ts);
            strcpy(current_task_ts->name, path_buffer);
            
            struct loader_result loader_result;
            struct file init_file = {
                .inode = lookup_result.inode,
                .offset = 0,
            };
            load_elf64(&init_file, &loader_result); // Unsafe
            load_cr3_from(current_task_ts);

            // Apply address delta in stack_buffer
            size_t user_address_delta = (void*)loader_result.user_entry_rsp - stack_end;
            for (size_t i = 0; i < argc; i++) {
                relocated_arg_pointers[i] = (void*)relocated_arg_pointers[i] + user_address_delta;
            }
            *relocated_arg_pointers_pointer = (void*)(*relocated_arg_pointers_pointer) + user_address_delta;

            // Copy stack_buffer to user stack
            memcpy((void*)(loader_result.user_entry_rsp - stack_length), stack_start, stack_length);

            *((uint64_t*)interrupt_rsp + IDT_SYSCALL_NUM_SAVED_REGISTERS) = loader_result.user_entry_rip;
            *((uint64_t*)interrupt_rsp + IDT_SYSCALL_NUM_SAVED_REGISTERS + 3) = loader_result.user_entry_rsp - stack_length;

            kpage_free(stack_buffer, 1);
            kpage_free(path_buffer, 1);
            return 0;
        }
        case SYSCALL_BRK: {
            struct userspace_memory_range *heap_range = NULL;
            for (
                struct list_head *memory_ranges_le = current_task_ts->memory_ranges_lh.next;
                memory_ranges_le != &current_task_ts->memory_ranges_lh;
                memory_ranges_le = memory_ranges_le->next
            ) {
                struct userspace_memory_range *range = container_of(memory_ranges_le, struct userspace_memory_range, memory_ranges_le);
                if (range->type == USERSPACE_MEMRANGE_HEAP) {
                    heap_range = range;
                    break;
                }
            }
            if (heap_range == NULL) {
                printk(u8p("No heap range found!\n"));
                return 0;
            }
            if (arg3 == 0) {
                return heap_range->end;
            }
            while (heap_range->end < arg3) {
                map_user_page(current_task_ts, (void*)heap_range->end);
                heap_range->end += PAGE_SIZE;
            }
            return heap_range->end;
        }
        case SYSCALL_WAITPID: {
            for (
                struct list_head *task_struct_le = task_struct_lh.next;
                task_struct_le != &task_struct_lh;
                task_struct_le = task_struct_le->next
            ) {
                struct task_struct *process = container_of(task_struct_le, struct task_struct, task_struct_le);
                if (process->pid == arg3) {
                    while (process->task_state != TS_ZOMBIE) {
                        task_yield();
                    }
                    free_userspace_memory(process);
                    free_kernelspace_memory(process);
                    free_task(process);
                    if (arg4) {
                        *((uint64_t*)arg4) = process->exit_code;
                    }
                    return arg3;
                }
            }
            return -1;
        }
        case SYSCALL_OPEN: {
            struct vfs_lookup_result lookup_result;
            uint8_t *path_arg = (void*)arg3;
            vfs_resolve(path_arg, &lookup_result);
            if (lookup_result.status == VFS_RESOLVE_ERR_NOT_A_DIR) {
                return -1;
            }
            if (lookup_result.status == VFS_RESOLVE_ERR_DOESNT_EXIST) {
                return -2;
            }
            if (lookup_result.status == VFS_RESOLVE_SUCCESS_DOESNT_EXIST) {
                if (arg4 & O_CREAT) {
                    struct inode *create_result = vfs_create(
                        lookup_result.parent_directory,
                        lookup_result.name_start // should be null-terminated because it's at end of path
                    );
                    if (!create_result) {
                        return -4;
                    }
                    struct file *filp = file_alloc();
                    filp->inode = create_result;
                    filp->offset = 0;
                    list_add_tail(&filp->files_le, &current_task_ts->files_lh);
                    struct list_head *previous_filp_le = filp->files_le.prev;
                    if (previous_filp_le == &current_task_ts->files_lh) {
                        // This is the first filp for this process
                        filp->fd = 0;
                    } else {
                        filp->fd = container_of(previous_filp_le, struct file, files_le)->fd + 1;
                    }
                    return filp->fd;
                } else {
                    return -3;
                }
            }
            if (lookup_result.status == VFS_RESOLVE_SUCCESS_EXISTS) {
                struct file *filp = file_alloc();
                filp->inode = lookup_result.inode;
                filp->offset = 0;
                list_add_tail(&filp->files_le, &current_task_ts->files_lh);
                struct list_head *previous_filp_le = filp->files_le.prev;
                if (previous_filp_le == &current_task_ts->files_lh) {
                    // This is the first filp for this process
                    filp->fd = 0;
                } else {
                    filp->fd = container_of(previous_filp_le, struct file, files_le)->fd + 1;
                }
                // Ignore O_TRUNCATE for directories and device files
                if (filp->inode->type == INODE_REGULAR_FILE && (arg4 & O_TRUNCATE)) {
                    ssize_t truncate_result = vfs_ftruncate(filp, 0);
                    if (truncate_result < 0) {
                        return truncate_result;
                    }
                }
                return filp->fd;
            }
            panic("Unknown lookup status\n");
        }
        case SYSCALL_CLOSE: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            list_del(&filp->files_le);
            file_free(filp);
            return 0;
        }
        case SYSCALL_GETDENTS: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            if (filp->inode->type != INODE_DIRECTORY) {
                return -2;
            }
            uint8_t *buf_start = (void*)arg4;
            uint8_t *buf = buf_start;
            uint8_t *end_of_buf = buf + arg5;
            list_for_each(dentry_le, filp->inode->dentry_lh) {
                struct dentry *dentry = container_of(dentry_le, struct dentry, dentry_le);
                uint16_t dentry_name_strlen = strlen(dentry->name);
                uint16_t len_required = sizeof(uint16_t) + dentry_name_strlen + 1;
                if (buf + len_required <= end_of_buf) {
                    *((uint16_t*)buf) = len_required;
                    buf += sizeof(uint16_t);
                    strcpy(buf, dentry->name);
                    buf += (dentry_name_strlen + 1);
                } else {
                    break;
                }
            }
            return buf - buf_start;
        }
        case SYSCALL_MKDIR: {
            struct vfs_lookup_result lookup_result;
            uint8_t *path_arg = (void*)arg3;
            vfs_resolve(path_arg, &lookup_result);
            if (lookup_result.status == VFS_RESOLVE_ERR_NOT_A_DIR) {
                return -1;
            }
            if (lookup_result.status == VFS_RESOLVE_ERR_DOESNT_EXIST) {
                return -2;
            }
            if (lookup_result.status == VFS_RESOLVE_SUCCESS_EXISTS) {
                return -3;
            }
            if (lookup_result.status == VFS_RESOLVE_SUCCESS_DOESNT_EXIST) {
                uint64_t return_value;
                uint8_t *name_buffer = kpage_alloc(1);
                memcpy(name_buffer, lookup_result.name_start, lookup_result.name_length);
                memset(name_buffer + lookup_result.name_length, 0, 1);
                struct inode *inode = vfs_mkdir(
                    lookup_result.parent_directory,
                    name_buffer
                );
                return_value = inode ? 0 : -1;
                kpage_free(name_buffer, 1);
                return return_value;
            }
            return lookup_result.status;
        }
        case SYSCALL_LSEEK: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            // For now, only support SEEK_SET
            if (arg5 != 0) {
                return -1;
            }
            filp->offset = arg4;
            return arg4;
        }
        case SYSCALL_FTRUNCATE: {
            struct file *filp = filp_find(current_task_ts, arg3);
            if (!filp) {
                return -1;
            }
            return vfs_ftruncate(filp, arg4);
        }
        case SYSCALL_DUP2: {
            struct file *old_filp = filp_find(current_task_ts, arg3);
            if (!old_filp) {
                return -1;
            }
            struct file *filp_to_close = filp_find(current_task_ts, arg4);
            if (filp_to_close) {
                list_del(&filp_to_close->files_le);
                file_free(filp_to_close);
            }
            struct file *next_filp = filp_insert_point_find(current_task_ts, arg4);
            struct list_head *next_filp_le = next_filp == NULL ?
                &current_task_ts->files_lh :
                &next_filp->files_le;
            struct file *new_filp = file_alloc();
            new_filp->fd = arg4;
            new_filp->inode = old_filp->inode;
            new_filp->offset = old_filp->offset;
            list_add_tail(&new_filp->files_le, next_filp_le);
            return new_filp->fd;
        }
        case SYSCALL_GETTASKS: {
            uint8_t *buf_start = (void*)arg3;
            uint8_t *buf = buf_start;
            uint8_t *end_of_buf = buf + arg4;
            list_for_each(task_struct_le, task_struct_lh) {
                struct task_struct *ts = container_of(
                    task_struct_le,
                    struct task_struct,
                    task_struct_le
                );
                uint16_t ts_name_strlen = strlen(ts->name);
                uint16_t len_required = sizeof(uint32_t) + sizeof(uint16_t) + ts_name_strlen + 1;
                if (buf + len_required <= end_of_buf) {
                    *((uint32_t*)buf) = ts->pid;
                    buf += sizeof(uint32_t);
                    *((uint16_t*)buf) = ts_name_strlen;
                    buf += sizeof(uint16_t);
                    strcpy(buf, ts->name);
                    buf += (ts_name_strlen + 1);
                } else {
                    break;
                }
            }
            return buf - buf_start;
        }
        case SYSCALL_KILL: {
            uint64_t pid = arg3;
            struct task_struct *task = task_struct_find(pid);
            if (!task) {
                return -1;
            }
            task->task_state = TS_ZOMBIE;
            task->exit_code = -1;
            // In case process is killing itself, make sure it doesn't return from this syscall
            task_yield();
            return 0;
        }
        case SYSCALL_SLEEP: {
            uint64_t start_ticks = timer_ticks;
            uint64_t end_ticks = start_ticks + 1 + (arg3 - 1) * TIMER_TICKS_PER_SECOND / 1000;
            while (timer_ticks < end_ticks) {
                task_yield();
            }
            return 0;
        }
        case SYSCALL_MOUNT: {
            uint8_t *device_path = arg3; // Unsafe
            uint8_t *mountpoint_path = arg4; // Unsafe
            uint8_t *fs_type = arg5; // Unsafe
            
            struct vfs_lookup_result device_lookup_result;
            vfs_resolve(device_path, &device_lookup_result);
            if (device_lookup_result.status != VFS_RESOLVE_SUCCESS_EXISTS) {
                return -1;
            }

            struct vfs_lookup_result mountpoint_lookup_result;
            vfs_resolve(mountpoint_path, &mountpoint_lookup_result);
            if (mountpoint_lookup_result.status != VFS_RESOLVE_SUCCESS_EXISTS) {
                return -2;
            }

            if (mountpoint_lookup_result.inode->type != INODE_DIRECTORY) {
                return -3;
            }

            if (strcmp(fs_type, u8p("exfat")) == 0) {
                return exfat_superblock_ops.mount(device_lookup_result.inode, mountpoint_lookup_result.dentry);
            } else {
                return -4;
            }
        }
        default: {
            printk(u8p("Unrecognized syscall: "));
            printk_uint64(syscall_number);
            printk(u8p("\n"));
            return 0;
        }
    }
}
