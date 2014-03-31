/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;

struct task_struct *idle_task;

/* Values 0 and 1 are reserved for idle and initial process respectively */
int next_free_pid = 2;

int curr_quantum = 0;

int get_quantum(struct task_struct *t)
{
    return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
    t->quantum = new_quantum;
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t)
{
	int pos;
	pos = ((int)t-(int)task)/sizeof(union task_union);
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];
	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

    printk("CPU IDLE");
	while(1)
	{
	;
	}
}

void init_freequeue()
{
    INIT_LIST_HEAD(&freequeue);

    int i;
    for (i = 0; i < NR_TASKS; i++) {
        list_add_tail(&(task[i].task.list), &freequeue);
    }
}

void init_readyqueue()
{
    INIT_LIST_HEAD(&readyqueue);
}

/* TODO: Debugg it further */
void init_idle (void)
{
    struct list_head *first = list_first(&freequeue);
    idle_task = list_head_to_task_struct(first);
    list_del(first);

    /* Its PID always is defiend as 0 */
    idle_task->PID = 0;

    set_quantum(idle_task, DEFAULT_QUANTUM);
    idle_task->state = ST_READY;
    init_stats(idle_task);

    /* TODO: Why is this call necessary? */
    allocate_DIR(idle_task);

    union task_union *idle_task_stack = (union task_union *)idle_task;
    idle_task_stack->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
    idle_task_stack->stack[KERNEL_STACK_SIZE-2] = 0;

    idle_task->kernel_esp = (unsigned long *)&(idle_task_stack->stack[KERNEL_STACK_SIZE-2]);
}

/* TODO: Debugg it further */
void init_task1(void)
{
    struct list_head *first = list_first(&freequeue);
    struct task_struct *pcb_init_task = list_head_to_task_struct(first);
    list_del(first);

    /* Its PID always is defiend as 1 */
    pcb_init_task->PID = 1;
    
    set_quantum(pcb_init_task, DEFAULT_QUANTUM);
    pcb_init_task->state = ST_READY;
    init_stats(pcb_init_task);

    allocate_DIR(pcb_init_task);
    set_user_pages(pcb_init_task);
    set_cr3(get_DIR(pcb_init_task));

    /* Must be added at readyqueue since this will become the first process
     * to be executed by the CPU
     */
    list_add_tail(&(pcb_init_task->list), &readyqueue);
}

void init_sched()
{
    /* Initializes required structures to perform the process scheduling */
    init_freequeue();
    init_readyqueue();

    curr_quantum = DEFAULT_QUANTUM;
}

/* TODO: Debugg it further */
void inner_task_switch(union task_union *t)
{
    struct task_struct *curr_task_pcb = current();

    tss.esp0 = KERNEL_ESP(t);

    /* TODO: Is it necessary to check anything before sets cr3 register? */
    /* set_cr3(get_DIR(curr_task_pcb)); */
    set_cr3(get_DIR(t));

    __asm__ __volatile__ (
        "mov %%ebp,%0\n"
        "movl %1, %%esp\n"
        "popl %%ebp\n"
        "ret\n"
        : "=g" (curr_task_pcb->kernel_esp)
        :"r" (t->task.kernel_esp)
    );
}

/* TODO: Debugg it further */
void task_switch(union task_union *t)
{
    /* Saves the registers esi, edi and ebx manually */
    SAVE_PARTIAL_CONTEXT

    inner_task_switch(t);

    /* Restores the previously saved registers */
    RESTORE_PARTIAL_CONTEXT
}

struct task_struct* current()
{
    int ret_value;

    __asm__ __volatile__(
  	    "movl %%esp, %0"
	    : "=g" (ret_value)
    );

    return (struct task_struct*)(ret_value&0xfffff000);
}

void update_sched_data_rr()
{
    --curr_quantum;
}

int needs_sched_rr()
{
    return (curr_quantum == 0);
}

void update_current_state_rr(struct list_head *dst_queue)
{
    /* Updates the state of current process */
    struct task_struct *curr_task = current();
    if (dst_queue == &freequeue) curr_task->state = ST_ZOMBIE;
    else if (dst_queue == &readyqueue) curr_task->state = ST_READY;
    else curr_task->state = ST_BLOCKED;

    /* Removes current process from its current queue and put it to dst_queue
     * (only if the current process is not the idle process)
     */
    if (curr_task != idle_task) {
        list_del(&(curr_task->list));
        list_add_tail(&(curr_task->list), dst_queue);
    }
}

void sched_next_rr()
{
    struct task_struct *pcb_next_task;
    
    /* If there is not any process in the readyqueue, executes idle process */
    if (list_empty(&readyqueue)) pcb_next_task = idle_task;
    else {
        struct list_head *list_next_task = list_first(&readyqueue);
        pcb_next_task = list_head_to_task_struct(list_next_task);
        pcb_next_task->state = ST_RUN;
    }

    /* Restores system quantum to the current quantum of the next process */
    curr_quantum = get_quantum(pcb_next_task);

    /* Performs a task switch only if the next process is not the current */
    if (pcb_next_task != current()) {
        update_stats(current(), RSYS_TO_READY);
        update_stats(pcb_next_task, READY_TO_RSYS);
        task_switch((union task_union *)pcb_next_task);
    }
}

void init_stats(struct task_struct *pcb)
{
    pcb->statistics.user_ticks = 0;
    pcb->statistics.system_ticks = 0;
    pcb->statistics.blocked_ticks = 0;
    pcb->statistics.ready_ticks = 0;
    pcb->statistics.elapsed_total_ticks = 0;
    pcb->statistics.total_trans = 0;
    pcb->statistics.remaining_ticks = 0;
}

void update_stats(struct task_struct *pcb, enum transition_t trans)
{
    switch (trans) {
    case RUSER_TO_RSYS :
        update_stats_ruser_to_rsys(pcb);
        break;

    case RSYS_TO_RUSER :
        update_stats_rsys_to_ruser(pcb);
        break;

    case RSYS_TO_READY :
        update_stats_rsys_to_ready(pcb);
        break;

    case READY_TO_RSYS :
        update_stats_ready_to_rsys(pcb);
        break;
    }
}

void update_stats_ruser_to_rsys(struct task_struct *pcb)
{
    pcb->statistics.user_ticks += (get_ticks() - pcb->statistics.elapsed_total_ticks);
    pcb->statistics.elapsed_total_ticks = get_ticks();
}

void update_stats_rsys_to_ruser(struct task_struct *pcb)
{
    pcb->statistics.system_ticks += (get_ticks() - pcb->statistics.elapsed_total_ticks);
    pcb->statistics.elapsed_total_ticks = get_ticks();
}

void update_stats_rsys_to_ready(struct task_struct *pcb)
{
    pcb->statistics.system_ticks += (get_ticks() - pcb->statistics.elapsed_total_ticks);
    pcb->statistics.elapsed_total_ticks = get_ticks();
}

void update_stats_ready_to_rsys(struct task_struct *pcb)
{
    pcb->statistics.ready_ticks += (get_ticks() - pcb->statistics.elapsed_total_ticks);
    pcb->statistics.elapsed_total_ticks = get_ticks();
    ++pcb->statistics.total_trans;
}

