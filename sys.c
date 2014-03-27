/*
 * sys.c - Syscalls implementation
 */

#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern unsigned int zeos_ticks;

int new_pid()
{
    return next_free_pid++;
}

int check_fd(int fd, int permissions)
{
    if (fd != 1) return -EBADF;
    if (permissions != ESCRIPTURA) return -EACCES;
    return 0;
}

int sys_ni_syscall()
{
    return -ENOSYS;
}

int sys_getpid()
{
    return current()->PID;
}

int ret_from_fork() {
    printk("RET FROM FORK\n");
    return 0;
}

int sys_fork()
{
    int PID=-1;
    unsigned int i;

    /* Return error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) return -EAGAIN;

    /* Needed variables related to child and father processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    union task_union *parent = (union task_union *)current();
    struct task_struct *pcb_child = &(child->task);
    struct task_struct *pcb_parent = &(parent->task);

    list_del(free_pcb);

    /* Inherits system code+data from father to child */
    /* TODO: How can we check whether the following copy performs successfully? */
    copy_data(parent, child, sizeof(union task_union));

    /* Reserve free frames (physical memory) to allocate child's user data */

    allocate_DIR(pcb_child);
    page_table_entry* pagt_child = get_PT(pcb_child);
    page_table_entry* pagt_parent = get_PT(pcb_parent);

    /* Array of free frames that will be reserved for child's user data allocation */
    int resv_frames[NUM_PAG_DATA];

    for (i = 0; i < NUM_PAG_DATA; i++) {

        /* If there is no enough free frames, those reserved thus far must be freed */
        if ((resv_frames[i] = alloc_frame()) == -1) {
            while (i >= 0) free_frame(resv_frames[i--]);
            return -ENOMEM;
        }
    }

    /* Inherit user code. Since it's shared among child and father, only it's needed
     * to update child's table page in order to map the correpond entries to frames
     * which allocates father's user code.
     */
    for (i = PAG_LOG_INIT_CODE_P0; i < PAG_LOG_INIT_DATA_P0; i++) {
       set_ss_pag(pagt_child, i, get_frame(pagt_parent, i));
    }

    /* Associates logical pages from child to physical reserved frame */
    for (i = 0; i < NUM_PAG_DATA; i++) {
        set_ss_pag(pagt_child, PAG_LOG_INIT_DATA_P0+i, resv_frames[i]);
    }

    /* Inherits user data */
    unsigned int stride = PAGE_SIZE * NUM_PAG_DATA;
    for (i = 0; i < NUM_PAG_DATA; i++) {
        unsigned int logic_addr = (i + PAG_LOG_INIT_DATA_P0) * PAGE_SIZE;
        set_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA, resv_frames[i]);
        copy_data((void *)(logic_addr), (void *)(logic_addr + stride), PAGE_SIZE);
        del_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA);
    }

    /* Flushes entire TLB */
    /* TODO: Why is necessary to write this value on cr3 (get_DIR(pcb_parent)) instead of 0? */
    set_cr3(get_DIR(pcb_parent));

    /* Updates child's PCB */
    PID = new_pid();
    pcb_child->PID = PID;
    pcb_child->quantum = DEFAULT_QUANTUM;

    /* Stores 0 into kernel stack position of eax in order to allow the child
     * process to return 0. eax is at tenth position from the base of kernel
     * (see documentation provided by entry.S file to know the kernel stack
     * status when switches to privilege level 0
     */
    unsigned int ebp;
    __asm__ __volatile__ (
            "movl %%ebp, %0"
            : /* no output */
            : "g" (ebp)
            );
    unsigned int stack_stride = (ebp - (unsigned int)parent)/sizeof(int);
    child->stack[stack_stride+2] = 0;
    child->stack[stack_stride+1] = (unsigned long)&ret_from_fork;

    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);

    task_switch(child);
    return PID;
}

void sys_exit()
{
}

int sys_write(int fd, char *buffer, int size)
{
    /* Check user parameters */
    int err = check_fd(fd, ESCRIPTURA);
    if (err < 0) return err;
    if (buffer == NULL) return -EFAULT;
    if (size < 0) return -EINVAL;

    /* access_ok() must be called to check the user space pointer buffer */

    char sys_buffer[size];
    copy_from_user(buffer, sys_buffer, size);

    /* Call the requested service routine and return the result */
    return sys_write_console(sys_buffer, size);
}

int sys_gettime()
{
    return zeos_ticks;
}

