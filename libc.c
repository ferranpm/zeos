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
  int ret;
  __asm__ __volatile__(
      "movl 8(%%ebp), %%ebx\n"
      "movl 12(%%ebp), %%ecx\n"
      "movl 16(%%ebp), %%edx\n"
      "movl $0x04, %%eax\n"
      "int $0x80\n"
      : "=g" (ret)
      : "g" (fd), "g" (buffer), "g" (size)
      );

  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

void perror() {
  char *msg_3 = "ERROR: Invalid file descriptor (!=1)\n";
  char *msg_5 = "ERROR: Buffer points to NULL\n";
  char *msg_7 = "ERROR: size <= 0\n";
  switch (errno) {
    case 3:
      write(1, msg_3, strlen(msg_3));
      break;
    case 5:
      write(1, msg_5, strlen(msg_5));
      break;
    case 7:
      write(1, msg_7, strlen(msg_7));
      break;
  }
}
