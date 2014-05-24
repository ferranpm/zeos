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

#include <keyboard.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define BUFFER_SIZE 256

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
    if (list_empty(&freequeue)) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EAGAIN;
    }

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
            list_add_tail(&(pcb_child->list), &freequeue);
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

    int NUM_PAG_HEAP = (pcb_parent->heap_break - pcb_parent->heap_start)/PAGE_SIZE;
    int resv_heap[NUM_PAG_HEAP];
    for (i = 0; i < NUM_PAG_HEAP; i++) {
        /* Associates a logical page from child's page table to physical reserved frame */
        set_ss_pag(pagt_child, pcb_child->heap_start+i, resv_heap[i]);

        /* Inherits one page of user data */
        unsigned int logic_addr = pcb_child->heap_start + (i * PAGE_SIZE);
        set_ss_pag(pagt_parent, i + pcb_child->heap_start + NUM_PAG_HEAP, resv_heap[i]);
        copy_data((void *)(logic_addr), (void *)(logic_addr + stride), PAGE_SIZE);
        del_ss_pag(pagt_parent, i + pcb_child->heap_start + NUM_PAG_HEAP);
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

    /* If current process is idle, immediately removes from the CPU */
    if (current()->PID == 0) sched_next_rr();

    update_stats(current(), RSYS_TO_RUSER);

    return PID;
}

/* TODO: Debug it further */
int sys_clone(void (*function) (void), void *stack)
{
    update_stats(current(), RUSER_TO_RSYS);

    int PID = -1;

    /* Checks user parameters */
    /* TODO: How we have to check the size of the user parameters for access_ok()? */
    if (!access_ok(VERIFY_READ, function, sizeof(function)) || !access_ok(VERIFY_WRITE, stack, sizeof(stack))) { 
        update_stats(current(), RSYS_TO_RUSER);
        return -EFAULT;
    }

    /* Returns error if there isn't any available task in the free queue */
    if (list_empty(&freequeue)) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EAGAIN;
    }

    /* Needed variables related to child and parent processes */
    struct list_head *free_pcb = list_first(&freequeue);
    union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
    union task_union *parent = (union task_union *)current();
    struct task_struct *pcb_child = &(child->task);

    list_del(free_pcb);

    /* Inherits system code+data */
    copy_data(parent, child, sizeof(union task_union));

    /* Updates references for child's page directory, inherited by parent */
    update_DIR_refs(pcb_child);
    
    /* Updates child's PCB (only the ones that the child process does not inherit) */
    PID = new_pid();
    pcb_child->PID = PID;
    pcb_child->state = ST_READY;

    /* TODO: Should stats be inherited too? */
    /* init_stats(pcb_child); */

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
    /* TODO: Is it necessary with sys_clone? */
    child->stack[stack_stride-1] = 0;

    /* TODO: Is it necessary with sys_clone? */
    child->stack[stack_stride] = (unsigned long)&ret_from_fork;
    
    child->task.kernel_esp = &child->stack[stack_stride-1];

    /* Modifies ebp with the address of the new stack */
    child->stack[stack_stride+7] = (unsigned long)stack;
    
    /* Modifies eip with the address of the new code (function) to execute */
    child->stack[stack_stride+13] = (unsigned long)function;
    
    /* Modifies esp with the address of the new stack */
    child->stack[stack_stride+16] = (unsigned long)stack;

    /* Adds child process to ready queue and returns its PID from parent */
    list_add_tail(&(pcb_child->list), &readyqueue);

    /* If current process is idle, immediately removes from the CPU */
    /* TODO: Is it necessary with sys_clone? */
    if (current()->PID == 0) sched_next_rr();

    update_stats(current(), RSYS_TO_RUSER);

    return PID;
}

void sys_exit()
{
    update_stats(current(), RUSER_TO_RSYS);

    /* To detroy the process, it's needed to free all resources of the process
     * and schedules the next task to be executed by the CPU
     */
    
    /* TODO: Modifies the compare statement to "== 0" */
    if (--dir_pages_refs[POS_TO_DIR_PAGES_REFS(get_DIR(current()))] <= 0) {
        free_user_pages(current());
    }

    /* TODO: Must we find if this process has a semaphore and, if it's true, destroys it? */
    int i;
    for (i = 0; i < NR_SEMS; i++) {
        if (sems[i].owner_pid == current()->PID) {
            
            /* TODO: What happens if sem_destroy returns error? */
            sys_sem_destroy(i);
        }
    }

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

    /* TODO: We can leave the current version or implements this will increase the qualityt of ZeOS implementation? */
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

int sys_sem_init(int n_sem, unsigned int value)
{
    update_stats(current(), RUSER_TO_RSYS);
   
    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }

    if (sems[n_sem].owner_pid != -1) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EBUSY;
    }
        
    sems[n_sem].owner_pid = current()->PID;
    sems[n_sem].count = value;
    INIT_LIST_HEAD(&(sems[n_sem].semqueue));

    update_stats(current(), RSYS_TO_RUSER);
    return 0;
}

int sys_sem_wait(int n_sem)
{
    update_stats(current(), RUSER_TO_RSYS);
   
    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }

    if (sems[n_sem].count > 0) --sems[n_sem].count;

    else {
        struct list_head *semqueue = &(sems[n_sem].semqueue);
        struct list_head *curr_task = &(current()->list);
        list_del(curr_task);
        current()->state = ST_BLOCKED;
        list_add_tail(curr_task, semqueue);
        update_stats(current(), RSYS_TO_BLOCKED);
        sched_next_rr();
    }

    /* Assures that the semaphore was destroyed while the process is blocked */
    if (sems[n_sem].owner_pid == -1) {
        update_stats(current(), RSYS_TO_RUSER);

        /* TODO. If we change the return value, all tests are also correct. Which would be the proper value to return in this case? */
        return -EPERM;
    }

    update_stats(current(), RSYS_TO_RUSER);
    return 0;
}

int sys_sem_signal(int n_sem)
{
    update_stats(current(), RUSER_TO_RSYS);
   
    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }
    
    struct list_head *semqueue = &(sems[n_sem].semqueue);

    /* TODO: Is (sems[n_sem].count == 0) an equivalent checking? */
    if (list_empty(semqueue)) ++sems[n_sem].count;
    else {
        struct list_head *elem = list_first(semqueue);
        struct task_struct *unblocked = list_head_to_task_struct(elem);
        list_del(elem);
        unblocked->state = ST_READY;
        list_add_tail(elem, &readyqueue);

        /* TODO: Is this transition correct, or should be BLOCKED_TO_READY directly? */
        update_stats(current(), BLOCKED_TO_RSYS);
    }

    update_stats(current(), RSYS_TO_RUSER);
    return 0;
}

int sys_sem_destroy(int n_sem)
{
    update_stats(current(), RUSER_TO_RSYS);
   
    /* Check user parameters */
    if (n_sem < 0 || n_sem >= NR_SEMS || sems[n_sem].owner_pid == -1) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EINVAL;
    }

    if (sems[n_sem].owner_pid != current()->PID) {
        update_stats(current(), RSYS_TO_RUSER);
        return -EPERM;
    }

    sems[n_sem].owner_pid = -1;
    struct list_head *semqueue = &(sems[n_sem].semqueue);
    while(!list_empty(semqueue)) {
        struct list_head *elem = list_first(semqueue);
        struct task_struct *unblocked = list_head_to_task_struct(elem);
        list_del(elem);
        unblocked->state = ST_READY;
        list_add_tail(elem, &readyqueue);

        /* TODO: Is this transition correct, or should be BLOCKED_TO_READY directly? */
        update_stats(unblocked, BLOCKED_TO_RSYS);
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

    /* Performs only one write if user size buffer is less than system default size buffer */
    if (size < BUFFER_SIZE) {

        /* Checks if buffer pointer points to a valid user space address */
        if (!access_ok(VERIFY_WRITE, buffer, size)) {
            update_stats(current(), RSYS_TO_RUSER);
            return -EFAULT;
        }
        
        char sys_buffer[size];
        copy_from_user(buffer, sys_buffer, size);

        /* Call the requested service routine (hardware dependent) and returns the result */
        sys_write_console(sys_buffer, size);
        update_stats(current(), RSYS_TO_RUSER);
        return size;
    }
    
    int i;
    for (i = 0; i < size; i += BUFFER_SIZE) {

        /* Checks if buffer pointer points to a valid user space address */
        if (!access_ok(VERIFY_WRITE, buffer + i, BUFFER_SIZE)) {
            update_stats(current(), RSYS_TO_RUSER);
            return -EFAULT;
        }

        char sys_buffer[BUFFER_SIZE];
        copy_from_user(buffer + i, sys_buffer, BUFFER_SIZE);

        /* Call the requested service routine (hardware dependent) */
        sys_write_console(sys_buffer, BUFFER_SIZE);
    }

    /* Computes the remainder bytes if there are */
    if (i > size) {
        int remainder = i - size;

        /* Checks if buffer pointer points to a valid user space address */
        if (!access_ok(VERIFY_WRITE, buffer - BUFFER_SIZE, remainder)) {
            update_stats(current(), RSYS_TO_RUSER);
            return -EFAULT;
        }
        
        char sys_buffer[remainder];
        copy_from_user(buffer + size - BUFFER_SIZE, sys_buffer, remainder);

        /* Call the requested service routine (hardware dependent) */
        sys_write_console(sys_buffer, remainder);
    }
    
    update_stats(current(), RSYS_TO_RUSER);
    return size;
}

int sys_gettime()
{
    update_stats(current(), RUSER_TO_RSYS);
    update_stats(current(), RSYS_TO_RUSER);
    return zeos_ticks;
}

int sys_read(int fd, char *buff, int count) {
    update_stats(current(), RUSER_TO_RSYS);
    // TODO: CHECK PARAMS
    /* if ((ret = kb_check_fd(fd, ESCRIPTURA) != 0)) return ret; */
    /* if ((ret = access_ok(VERIFY_WRITE, buff, count)) == 0) return ret; */
    if (count < 0) return -1;
    update_stats(current(), RSYS_TO_RUSER);
    return sys_read_keyboard(buff, count);
}

void *sys_sbrk(int increment) {
    update_stats(current(), RUSER_TO_RSYS);
    struct task_struct *current_task = current();
    void *ret = current_task->heap_break;
    int i;

    if (increment <= 0) return current_task->heap_break;

    page_table_entry* current_page = get_PT(current_task);

    /* Reserve free frames (physical memory) to allocate child's user data */
    int NUM_PAG_HEAP = increment/PAGE_SIZE + 1;
    int resv_frames[NUM_PAG_HEAP];

    for (i = 0; i < NUM_PAG_HEAP; i++) {

        /* If there is no enough free frames, those reserved thus far must be freed */
        if ((resv_frames[i] = alloc_frame()) == -1) {
            while (i >= 0) free_frame(resv_frames[i--]);
            return -ENOMEM;
        }
    }

    printc_xy(1,1,'a');
    for (i = 0; i < NUM_PAG_HEAP; i++) {
        printc_xy(1,1,'z');
        unsigned int logic_addr = (i + (unsigned long)current_task->heap_break) * PAGE_SIZE;
        set_ss_pag(current_page, (void *)logic_addr, resv_frames[i]);
    }
    /* for (i = current_task->heap_break; i < current_task->heap_break + increment; i++) { */
    /*     set_ss_pag(current_page, i, resv_frames[i-(unsigned long)current_task->heap_start]); */
    /*     #<{(| int x = i - current_task->heap_break; |)}># */
    /*     #<{(| set_ss_pag(current_page, i, get_frame(current_page, x)); |)}># */
    /* } */
    printc_xy(1,1,'b');

    current_task->heap_break += increment;

    update_stats(current(), RSYS_TO_RUSER);
    return ret;
}
