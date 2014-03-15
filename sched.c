/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}

struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;

struct task_struct *idle_task;

unsigned int curr_pid = 1;

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

void init_freequeue() {
    INIT_LIST_HEAD(&freequeue);
    
    int i;
    for (i = 0; i < NR_TASKS; i++) {
        list_add_tail(&(task[i].task.list), &freequeue);
    }
}

void init_readyqueue() {
    INIT_LIST_HEAD(&readyqueue);
}

void init_idle (void)
{
    struct list_head *first = list_first(&freequeue);
    idle_task = list_head_to_task_struct(first);
    list_del(first);
    idle_task->PID = 0; /* Its PID always is defiend as 0 */
    allocate_DIR(idle_task);
    idle_task->quantum = DEFAULT_QUANTUM;
    union task_union *p = (union task_union*)idle_task;
    p->stack[KERNEL_STACK_SIZE-1] = cpu_idle;
    p->stack[KERNEL_STACK_SIZE-2] = 0;
}

void init_task1(void)
{
    struct list_head *first = list_first(&freequeue);
    struct task_struct *task = list_head_to_task_struct(first);
    list_del(first);
    task->PID = 1; /* Its PID always is defiend as 1 */
    allocate_DIR(task);
    task->quantum = DEFAULT_QUANTUM;
}


void init_sched() {
    
    /* Initializes required structures to perform the process scheduling */
    init_freequeue();
    init_readyqueue();

}

void task_switch(union task_union*t) {

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

