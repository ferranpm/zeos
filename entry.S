/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>

/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

/* macro needed to save the remainning hardware context */
#define SAVE_ALL             \
    pushl %gs;               \
    pushl %fs;               \
    pushl %es;               \
    pushl %ds;               \
    pushl %eax;              \
    pushl %ebp;              \
    pushl %edi;              \
    pushl %esi;              \
    pushl %edx;              \
    pushl %ecx;              \
    pushl %ebx;              \
    movl $__KERNEL_DS, %edx; \
    movl %edx, %ds;          \
    movl %edx, %es

/* macro needed to restore the remainning hardware context */
#define RESTORE_ALL         \
    popl %ebx;              \
    popl %ecx;              \
    popl %edx;              \
    popl %esi;              \
    popl %edi;              \
    popl %ebp;              \
    popl %eax;              \
    popl %ds;               \
    popl %es;               \
    popl %fs;               \
    popl %gs

/* End Of Interrupti (EOI): macro needed to solve hardware interrupts */
#define EOI      \
movb $0x20, %al; \
outb %al, $0x20

/* size of system call table */
MAX_SYSCALL = 40

/*
 * system call handler, one handler to solve all system calls
 * located at system call table
 */

ENTRY(system_call_handler)
     SAVE_ALL                           /* Save the current context */
     cmpl $0, %eax                      /* Is syscall number negative? */
     jl err                             /* If it is, jump to return an error */
     cmpl $MAX_SYSCALL, %eax            /* Is syscall greater than MAX_SYSCALL? */
     jg err                             /* If it is, jump to return an error */
     call *sys_call_table(, %eax, 0x04) /* Call the corresponding service routine */
     jmp fin                            /* Finish */
err:
     call sys_ni_syscall
fin:
     movl %eax, 0x18(%esp)              /* Change the eax value in the stack */
     RESTORE_ALL                        /* Restore the context */
     iret

/* keyboard interrupt handler */
ENTRY(keyboard_handler)
    SAVE_ALL
    EOI
    call keyboard_routine
    RESTORE_ALL
    iret

/* clock interrupt handler */
ENTRY(clock_handler)
    SAVE_ALL
    EOI
    call clock_routine
    RESTORE_ALL
    iret

