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

int ret_from_fork() {
    return 0;
}

int sys_ni_syscall()
{
    update_stats(current(), RUSER_TO_RSYS);
    update_stats(current(), RSYS_TO_RUSER);
    return -ENOSYS;
}

int sys_getpid()
{
    update_stats(current(), RUSER_TO_RSYS);
    update_stats(current(), RSYS_TO_RUSER);
    return current()->PID;
}

int sys_fork()
{
    update_stats(current(), RUSER_TO_RSYS);

    int PID = -1;
    unsigned int i;

    /* Returns error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) return -EAGAIN;

    /* Needed variables related to child and parent processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    union task_union *parent = (union task_union *)current();
    struct task_struct *pcb_child = &(child->task);
    struct task_struct *pcb_parent = &(parent->task);

    list_del(free_pcb);

    /* Inherits system code+data */
    copy_data(parent, child, sizeof(union task_union));

    /* Allocates new page directory for child process */
    allocate_DIR(pcb_child);
    
    page_table_entry* pagt_child = get_PT(pcb_child);
    page_table_entry* pagt_parent = get_PT(pcb_parent);

    /* Reserve free frames (physical memory) to allocate child's user data */
    int resv_frames[NUM_PAG_DATA];

    for (i = 0; i < NUM_PAG_DATA; i++) {

        /* If there is no enough free frames, those reserved thus far must be freed */
        if ((resv_frames[i] = alloc_frame()) == -1) {
            while (i >= 0) free_frame(resv_frames[i--]);
            update_stats(current(), RSYS_TO_RUSER);
            return -ENOMEM;
        }
    }

    /* Inherits user code. Since it's shared among child and father, only it's needed
     * to update child's page table in order to map the correpond entries to frames
     * which allocates father's user code.
     */
    for (i = PAG_LOG_INIT_CODE_P0; i < PAG_LOG_INIT_DATA_P0; i++) {
        set_ss_pag(pagt_child, i, get_frame(pagt_parent, i));
    }

    /* Inherits user data. Since each process has its own copy allocated in physical
     * memory, it's needed to copy the user data from parent process to the news
     * reserved frames. First the page table entries from child process must be 
     * associated to the new reserved frames. Then the user copy data is performed by
     * modifying the logical adress space of the parent to points to reserved frames,
     * then makes the copy of data, and finally deletes these new entries of parent's
     * page table to deny the access to the child's user data.
     */
    unsigned int stride = PAGE_SIZE * NUM_PAG_DATA;
    for (i = 0; i < NUM_PAG_DATA; i++) {
        /* Associates a logical page from child's page table to physical reserved frame */
        set_ss_pag(pagt_child, PAG_LOG_INIT_DATA_P0+i, resv_frames[i]);

        /* Inherits one page of user data */
        unsigned int logic_addr = (i + PAG_LOG_INIT_DATA_P0) * PAGE_SIZE;
        set_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA, resv_frames[i]);
        copy_data((void *)(logic_addr), (void *)(logic_addr + stride), PAGE_SIZE);
        del_ss_pag(pagt_parent, i + PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA);
    }

    /* Flushes entire TLB */
    set_cr3(get_DIR(pcb_parent));

    /* Updates child's PCB (only the ones that the child process does not inherit) */
    PID = new_pid();
    pcb_child->PID = PID;
    pcb_child->state = ST_READY;
    init_stats(pcb_child);

    /* Prepares the return of child process. It must return 0
     * and its kernel_esp must point to the top of the stack
     */
    unsigned int ebp;
    __asm__ __volatile__(
        "mov %%ebp,%0\n"
        :"=g"(ebp)
    );

    unsigned int stack_stride = (ebp - (unsigned int)parent)/sizeof(unsigned long);

    /* Dummy value for ebp for the child process */
    child->stack[stack_stride-1] = 0;

    child->stack[stack_stride] = (unsigned long)&ret_from_fork;
    child->task.kernel_esp = &child->stack[stack_stride-1];

    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);

    update_stats(current(), RSYS_TO_RUSER);

    return PID;
}

void sys_exit()
{
    update_stats(current(), RUSER_TO_RSYS);

    /* To detroy the process, it's needed to free all resources of the process
     * and schedules the next task to be executed by the CPU
     */
    free_user_pages(current());
    update_current_state_rr(&freequeue);
    sched_next_rr();
}

int sys_get_stats(int pid, struct stats *st)
{
    update_stats(current(), RUSER_TO_RSYS);

    /* Check user parameters */
    if (pid < 0) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }

    /* Checks if st pointer points to a valid user space address */
    if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EFAULT;
    }

    struct task_struct *desired_pcb = NULL;

    /* Checks if the pid corresponds to the current process or the process
     * associated to PID = pid exists and it's alive
     */
    if (pid == current()->PID) desired_pcb = current();
    else {
        int i = 0, found = 0;
        while (i < NR_TASKS && !found) {
            if (task[i].task.PID == pid && task[i].task.state != ST_FREE) {
                found = 1;
                desired_pcb = &task[i];
            }
            i++;
        }
    }

    /* Comment this section for future improvements on ZeOS
    else {
        struct list_head *pt_list;
        list_for_each(pt_list, &readyqueue) {
            struct task_struct *pcb = list_head_to_task_struct(pt_list);
            if ((pcb->PID == pid) && (pcb->state == ST_READY)) {
                desired_pcb = pcb;
                break;
            }
        }
    }
    */

    if (desired_pcb == NULL) {
        update_stats(current(), RSYS_TO_RUSER);
        return -ESRCH;
    }

    if (copy_to_user(&(desired_pcb->statistics), st, sizeof(struct stats)) < 0) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EPERM;
    }

    update_stats(current(), RSYS_TO_RUSER);
    return 0;
}

int sys_write(int fd, char *buffer, int size)
{
    update_stats(current(), RUSER_TO_RSYS);

    /* Check user parameters */
    int err = check_fd(fd, ESCRIPTURA);
    if (err < 0) {
        update_stats(current(), RSYS_TO_RUSER);
        return err;
    }
    if (buffer == NULL) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EFAULT;
    }
    if (size < 0) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }

    /* Checks if buffer pointer points to a valid user space address */
    if (!access_ok(VERIFY_WRITE, buffer, size)) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EFAULT;
    }

    char sys_buffer[size];
    copy_from_user(buffer, sys_buffer, size);

    /* Call the requested service routine (hardware dependent) and return the result */
    err = sys_write_console(sys_buffer, size);
    update_stats(current(), RSYS_TO_RUSER);
    return err;

}

int sys_gettime()
{
    update_stats(current(), RUSER_TO_RSYS);
    update_stats(current(), RSYS_TO_RUSER);
    return zeos_ticks;
}

