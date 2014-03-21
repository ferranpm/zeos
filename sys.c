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

int sys_fork()
{
    int PID=-1;
    int i;

    if (list_empty(&freequeue)) {
        // ERROR
    }
    struct list_head *child_head = list_first(freequeue);
    union task_union *child_union = (union task_union*)list_head_to_task_struct(child_task);
    struct task_union *current_union = (union task_union*)current();

    copy_data(current_union, child_union, ((current_union + 1) - current_union));

    for (i = 0; i < NUM_PAG_DATA; i++) {
        int dir = allocate_DIR((struct task_struct*)child_union);
        int frame = alloc_frame();
        if (frame == -1) {
            // ERROR
        }
        set_ss_pag(get_PT(child_union), PAG_LOG_INIT_DATA_P0+i, frame);
    }

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

    /* access_ok() must be called to check the user space poinetr buffer */

    char sys_buffer[size];
    copy_from_user(buffer, sys_buffer, size);

    /* Call the requested service routine and return the result */
    return sys_write_console(sys_buffer, size);
}

int sys_gettime()
{
    return zeos_ticks;
}

