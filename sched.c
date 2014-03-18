/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

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
unsigned int curr_pid = 2;

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

    idle_task->quantum = DEFAULT_QUANTUM;

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
    struct task_struct *pcb_ini_task = list_head_to_task_struct(first);
    list_del(first);

    /* Its PID always is defiend as 1 */
    pcb_ini_task->PID = 1;

    pcb_ini_task->quantum = DEFAULT_QUANTUM;

    allocate_DIR(pcb_ini_task);
    set_user_pages(pcb_ini_task);
    set_cr3(get_DIR(pcb_ini_task));
}

void init_sched()
{
    /* Initializes required structures to perform the process scheduling */
    init_freequeue();
    init_readyqueue();
}

/* TODO: Debugg it further */
void inner_task_switch(union task_union *t)
{
    struct task_struct *curr_task_pcb = current();
    tss.esp0 = (unsigned long)&(t->stack[KERNEL_STACK_SIZE]);

    /* TODO: Is it necessary to check anything before sets cr3 register? */
    set_cr3(get_DIR(curr_task_pcb));

    __asm__ __volatile__(
        "mov %%ebp,%0\n"
        "mov %1,%%esp\n"
        "pop %%ebp\n"
        "ret\n"
        : /* no output */
        : "r" (curr_task_pcb->kernel_esp), "r" (t->task.kernel_esp)
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

