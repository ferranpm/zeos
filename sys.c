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

    /* creates the child process */

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

