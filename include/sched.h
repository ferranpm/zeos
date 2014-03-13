/*
 * sched.h - Structures and macros used in process management
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define NR_TASKS 10
#define KERNEL_STACK_SIZE 1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
    int PID;                               /* Process ID */
    page_table_entry * dir_pages_baseAddr; /* Directory base address */
    unsigned int quantum;
    struct list_head list;
};

union task_union {
    struct task_struct task;
    unsigned long stack[KERNEL_STACK_SIZE]; /* system stack, one per process */
};

extern union task_union task[NR_TASKS]; /* Task array */
extern struct task_struct *idle_task;
extern unsigned int pid;

/* Queues needed to implement process management */
/* TODO: Where is better to declare extern variables, on .h or .c files? */
extern struct list_head freequeue;
extern struct list_head readyqueue;

#define KERNEL_ESP(t) (DWord) &(t)->stack[KERNEL_STACK_SIZE]
#define INITIAL_ESP KERNEL_ESP(&task[1])


/* Initializes required data for the initial process */
struct task_struct * current();
struct task_struct *list_head_to_task_struct(struct list_head *l);
void init_task1(void);
void init_idle(void);
void init_sched(void);
void task_switch(union task_union*t);
int allocate_DIR(struct task_struct *t);
page_table_entry * get_PT (struct task_struct *t) ;
page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_current_state_rr(struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

#endif  /* __SCHED_H__ */
