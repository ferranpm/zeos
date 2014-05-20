/*
 * libc.c
 */

#include <libc.h>
#include <types.h>
#include <errno.h>

#include <keyboard.h>

/* Useful macro to set errno and return expected value from each system call */
#define SET_ERRNO_RETURN                     \
    int mask = ret >> 31;                    \
    errno = (mask & -ret) | (~mask & errno); \
    return (ret | mask);

int errno = 0;

const char *sys_errlist[] = {
   [0]      =  "Unknown error\n",
   [EPERM]  =  "Operation not permitted\n",
   [ESRCH]  =  "No such process\n",
   [EBADF]  =  "Bad file number\n",
   [EAGAIN] =  "Try again\n",
   [ENOMEM] =  "Out of memory\n",
   [EACCES] =  "Permission denied\n",
   [EFAULT] =  "Bad addresss\n",
   [EINVAL] =  "Invalid argument\n",
};

void itoa(int a, char *b)
{
    int i, i1;
    char c;

    if (a == 0) {
        b[0] = '0';
        b[1] = 0;
        return;
    }

    i=0;
    while (a > 0) {
        b[i] = (a % 10) + '0';
        a = a / 10;
        i++;
    }

    for (i1 = 0; i1 < i/2; i1++) {
        c=b[i1];
        b[i1]=b[i-i1-1];
        b[i-i1-1]=c;
    }

    b[i]=0;
}

int strlen(char *a)
{
    int i = 0;

    while (a[i] != 0) i++;

    return i;
}

void perror() {
    write(1, *(sys_errlist + errno), strlen(*(sys_errlist + errno)));
}

/* Wrapper for the system call sys_write(int fd, char *buffer, int size).
 * It has got the entry 4 (0x04) in the system call table
 */
int write(int fd, char *buffer, int size)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (fd), "c" (buffer), "d" (size), "a" (0x04)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_gettime().
 * It has got the entry 10 (0x0a) in the system call table
 */
int gettime()
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "a" (0x0a)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_getpid().
 * It has got the entry 20 (0x14) in the system call table
 */
int getpid()
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "a" (0x14)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_fork().
 * It has got the entry 2 (0x02) in the system call table
 */
int fork()
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "a" (0x02)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_clone().
 * It has got the entry 19 (0x13) in the system call table
 */
int clone(void (*function) (void), void *stack)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (function), "c" (stack), "a" (0x13)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_exit().
 * It has got the entry 1 (0x01) in the system call table.
 * Does not return any value
 */
void exit()
{
    __asm__ __volatile__(
        "int $0x80\n"
        : /* No output */
        : "a" (0x01)
    );
}

/* Wrapper for the system call sys_get_stats(int pid, struct stats *st).
 * It has got the entry 35 (0x23) in the system call table
 */
int get_stats(int pid, struct stats *st)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (pid), "c" (st), "a" (0x23)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_sem_init(int n_sem, unsigned int value).
 * It has got the entry 21 (0x15) in the system call table
 */
int sem_init(int n_sem, unsigned int value)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (n_sem), "c" (value), "a" (0x15)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_sem_wait(int n_sem).
 * It has got the entry 22 (0x16) in the system call table
 */
int sem_wait(int n_sem)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (n_sem), "a" (0x16)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_sem_signal(int n_sem).
 * It has got the entry 23 (0x17) in the system call table
 */
int sem_signal(int n_sem)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (n_sem), "a" (0x17)
    );

    SET_ERRNO_RETURN
}

/* Wrapper for the system call sys_sem_destroy(int n_sem).
 * It has got the entry 24 (0x18) in the system call table
 */
int sem_destroy(int n_sem)
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (n_sem), "a" (0x18)
    );

    SET_ERRNO_RETURN
}

int read(int fd, char *buff, int count) {

    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "b" (fd), "c" (buff), "d" (count), "a" (30)
    );
    /* int i; */
    /* char a = keyboard_buffer_pop(); */
    /* write(1, &a, 1); */
    /* #<{(| for (i = 0; i < count; i++) { |)}># */
    /* #<{(|     buff[i] = keyboard_buffer_pop(); |)}># */
    /* #<{(| } |)}># */
}
