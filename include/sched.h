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

/* TODO: Sets properly the default value for quantum (process scheduling) */
#define DEFAULT_QUANTUM 5

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
    int PID;                               /* Process ID */
    page_table_entry * dir_pages_baseAddr; /* Directory base address */
    unsigned int quantum;
    unsigned long *kernel_esp;
    struct list_head list;
};

union task_union {
    struct task_struct task;
    unsigned long stack[KERNEL_STACK_SIZE]; /* system stack, one per process */
};

extern union task_union task[NR_TASKS]; /* Task array */
extern struct task_struct *idle_task;   /* Idle process */
extern unsigned int curr_pid;           /* Current available PID to assign to processes */

/* Structures needed to implement process management */
extern struct list_head freequeue;
extern struct list_head readyqueue;

#define KERNEL_ESP(t) (DWord) &(t)->stack[KERNEL_STACK_SIZE]
#define INITIAL_ESP KERNEL_ESP(&task[1])

/* Macros needed to save and restore context during process switch */
#define SAVE_PARTIAL_CONTEXT \
    __asm__ __volatile__(    \
        "pushl %esi\n"       \
        "pushl %edi\n"       \
        "pushl %ebx\n"       \
            );

#define RESTORE_PARTIAL_CONTEXT \
    __asm__ __volatile__(       \
        "popl %ebx\n"           \
        "popl %edi\n"           \
        "popl %esi\n"           \
            );

/* Headers for the process management */
struct task_struct * current();
struct task_struct *list_head_to_task_struct(struct list_head *l);
void init_freequeue(void);
void init_readyqueue(void);
void init_task1(void);
void init_idle(void);
void init_sched(void);
void task_switch(union task_union *t);
int allocate_DIR(struct task_struct *t);
page_table_entry * get_PT (struct task_struct *t) ;
page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_current_state_rr(struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

#endif  /* __SCHED_H__ */

