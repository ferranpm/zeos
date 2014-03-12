/*
 * libc.c
 */

#include <libc.h>
#include <types.h>
#include <errno.h>

/* Useful macro to set errno and return expected value from each system call */
#define SET_ERRNO_RETURN                     \
    int mask = ret >> 31;                    \
    errno = (mask & -ret) | (~mask & errno); \
    return (ret | mask);

int errno = 0;

/* TODO: Complete the rest of error messages defined at errno.h */
const char *sys_errlist[] = {
   [0]      =  "Unknown error\n",
   [EBADF]  =  "Bad file number\n",
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

int write(int fd, char *buffer, int size)
{
    int ret;
    __asm__ __volatile__ (
        "int $0x80\n\t"
        : "=a" (ret)
        : "b" (fd), "c" (buffer), "d" (size), "a" (0x04)
    );

    SET_ERRNO_RETURN
}

void perror() {
    write(1, *(sys_errlist + errno), strlen(*(sys_errlist + errno)));
}

int gettime()
{
    int ret;
    __asm__ __volatile__(
        "int $0x80\n"
        : "=a" (ret)
        : "a" (0x04)
    );

    SET_ERRNO_RETURN
}

