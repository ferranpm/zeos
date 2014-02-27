#include <libc.h>

char buff[24];

int pid;

long inner (long n) {
    int i;
    long suma = 0;
    for (i=0; i<n; i++) suma += i;
    return suma;
}

long outer (long n) {
    int i;
    long acum = 0;
    for (i=0; i<n; i++) acum += inner(i);
    return acum;
}

int add(int par1, int par2) {
    int suma = 0;
    __asm__ __volatile(
        "movl %1, %%eax\n"
        "addl %2, %%eax\n"
	"movl %%eax, %0\n"
	: "=g" (suma)
	: "g" (par1), "g" (par2)
	: "ax", "memory"
    );
    return suma;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  long count, acum;
  count = 75;
  acum = 0;
  acum = outer(count);
  volatile int suma = 0, a = 4, b = 6;
  suma = add(a, b);
  while(1) { }
  return 0;
}
