#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <list.h>

struct sem_t {
    int id;
    unsigned int value;
    int owner_pid;
    struct list_head semqueue;
};

#endif /* __SEMAPHORE_H__ */

