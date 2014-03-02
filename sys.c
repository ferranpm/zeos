/*
 * sys.c - Syscalls implementation
 */

#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;

int check_fd(int fd, int permissions)
{
    if (fd != 1) return -9; /* EBADF */
    if (permissions != ESCRIPTURA) return -13; /* EACCES */
    return 0;
}

int sys_ni_syscall()
{
    return -38; /*ENOSYS*/
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
    /* TODO: Decide final error values for the final release */
    if (check_fd(fd, ESCRIPTURA) < 0) return -3;
    if (buffer == NULL) return -5;
    if (size < 0) return -7;
    
    /* TODO: Is it needed to call access_ok() to check if user space poinetr 
     * buffer is valid?
     */
    char sys_buffer[size];
    copy_from_user(buffer, sys_buffer, size);

    /* Call the requested service routine and return the result */
    return sys_write_console(sys_buffer, size);
}

int sys_gettime()
{
    return zeos_ticks;
}

