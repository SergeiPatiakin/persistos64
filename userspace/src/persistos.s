.text

.global _start
_start:
    call libc_init
    
    // jmp _start_call_main
    // Calculate argc
    movq $0, %rdi
    movq (%rsp), %rsi
_start_try_another_arg:
    cmpq $0, (%rsi)
    jz _start_no_more_args
    addq $1, %rdi
    addq $8, %rsi
    jmp _start_try_another_arg
_start_no_more_args:
    movq (%rsp), %rsi
_start_call_main:
    call main
    movq %rax, %rdi
    call exit

.global write
write:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $1, %rdi
    int $0x80
    retq

.global read
read:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $2, %rdi
    int $0x80
    retq

.global exit
exit:
    movq %rdi, %rsi
    movq $3, %rdi
    int $0x80
    retq

.global getpid
getpid:
    movq $4, %rdi
    int $0x80
    retq

.global sched_yield
sched_yield:
    movq $5, %rdi
    int $0x80
    retq

.global fork
fork:
    movq $6, %rdi
    int $0x80
    retq

.global exec
exec:
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $7, %rdi
    int $0x80
    retq

.global brk
brk:
    movq %rdi, %rsi
    movq $8, %rdi
    int $0x80
    retq

.global waitpid
waitpid:
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $9, %rdi
    int $0x80
    retq

.global open
open:
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $10, %rdi
    int $0x80
    retq

.global close
close:
    movq %rdi, %rsi
    movq $11, %rdi
    int $0x80
    retq

.global getdents
getdents:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $12, %rdi
    int $0x80
    retq

.global mkdir
mkdir:
    movq %rdi, %rsi
    movq $13, %rdi
    int $0x80
    retq

.global lseek
lseek:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $14, %rdi
    int $0x80
    retq

.global ftruncate
ftruncate:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $15, %rdi
    int $0x80
    retq

.global dup2
dup2:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $16, %rdi
    int $0x80
    retq

.global gettasks
gettasks:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $17, %rdi
    int $0x80
    retq

.global kill
kill:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $18, %rdi
    int $0x80
    retq

.global sleep
sleep:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $19, %rdi
    int $0x80
    retq

.global mount
mount:
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq $20, %rdi
    int $0x80
    retq
