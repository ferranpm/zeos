/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

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
    __asm__ __volatile__(
        "movl 8(%%esp), %%ebx\n"
        "movl 12(%%esp), %%ecx\n"
        "movl 16(%%esp), %%edx\n"
        "movl $0x04, %%eax\n"
        "int $0x80\n"
        :
        : "g" (fd), "g" (buffer), "g" (size)
    );
    /* __asm__ __volatile__( */
    /*     "popl %%ebx\n" */
    /*     "popl %%ecx\n" */
    /*     "popl %%edx\n" */
    /*     "movl $0x04, %%eax\n" */
    /*     "int $0x80\n" */
    /*     "ret\n" */
    /*     : */
    /*     : "g" (fd), "g" (buffer), "g" (size) */
    /* ); */

    /* TODO: Parameter passing (parameters of the stack must be copied to
     * the registers ebx, ecx, edx, esi, edi)
     */

    /* TODO: Put the identifier of the write system call to eax (it is 4) */

    /* TODO: Generate the trap (int $0x80) */

    /* TODO: Process the result */

    /* TODO: Return */
}
