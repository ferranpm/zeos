/*
 * libc.c 
 */

#include <libc.h>
#include <types.h>
#include <errno.h>

/* 
 * TODO: If a system call returns without errors, errno can be set to 0?
 *       Or it must be set to previous value (last system call error)?
 */
#define SET_ERRNO_RETURN        \
    errno = (ret >> 31) & -ret; \
    return (ret | (ret >> 31));

int errno = 0;

/* TODO: Decide how define position 0 */
/* TODO: Complete the rest of error messages defined at errno.h */
const char *sys_errlist[] = {
   [0]      =  "Unknown Error\n",
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

    /* TODO: Figure out why the content of ebx is not saved */
    /*
    __asm__ __volatile__(
        "movl 0x08(%%ebp), %%ebx\n"
        "movl 0x0c(%%ebp), %%ecx\n"
        "movl 0x10(%%ebp), %%edx\n"
        "movl $0x04, %%eax\n"
        "int $0x80\n"
        : "=a" (ret)
    );
    */

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
        "movl $0x0a, %%eax\n"
        "int $0x80\n"
        : "=a" (ret)
    );

    SET_ERRNO_RETURN
}

