/*
 * libc.c 
 */

#include <libc.h>
#include <types.h>

int errno;

/* TODO: Empties error_msg[0] for final release */
char **error_msg = (char *[]) {
    "OK\n",                                   /* 0 */
    "",
    "",
    "ERROR: Invalid file descriptor (!=1)\n",
    "",
    "ERROR: Buffer points to NULL\n",         /* 5 */
    "",
    "ERROR: Size of buffer < 0\n",
    "",
    "",
};

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write(int fd, char *buffer, int size)
{
    int ret;
    __asm__ __volatile__(
        "movl 0x08(%%ebp), %%ebx\n"
        "movl 0x0c(%%ebp), %%ecx\n"
        "movl 0x10(%%ebp), %%edx\n"
        "movl $0x04, %%eax\n"
        "int $0x80\n"
        : "=g" (ret)
        : "g" (fd), "g" (buffer), "g" (size)
    );

    /*
    if (ret < 0) {
      errno = -ret;
      ret = -1;
    }*/

    /* TODO: Write a macro for this repetitve task */
    errno = (ret >> 31) & -ret;
    return (ret | (ret >> 31));
}

void perror() {
    write(1, *(error_msg + errno), strlen(*(error_msg + errno)));
}

int gettime()
{
    int ret;
    __asm__ __volatile__(
        "movl $0x0a, %%eax\n"
        "int $0x80\n"
        : "=g" (ret)
        :
    );

    /*
    if (ret < 0) {
        errno = -ret;
        ret = -1;
    }*/

    /* TODO: Write a macro for this repetitve task */
    errno = (ret >> 31) & -ret;
    return (ret | (ret >> 31));
}

