#include <libc.h>

char buff[24];

int pid;

/* Example of __asm__ syntax (not used in user code) */
/*
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
*/


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    /* Testing code for write() system call and perror() function */
    char *string = "Hola mon\n";
    write(2, string, strlen(string));
    perror();
    write(1, string, strlen(string));
    perror();
    write(1, 0, strlen(string));
    perror();
    write(1, string, -3);
    perror();

    while(1) {}

    return 0;
}

