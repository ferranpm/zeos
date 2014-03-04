#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    /* Testing code for write() system call and perror() function */
    char *string = "Hola mon\n";
    if (write(2, string, strlen(string)) < 0) perror();
    if (write(1, string, strlen(string)) < 0) perror();
    if (write(1, 0, strlen(string)) < 0) perror();
    if (write(1, string, -3) < 0) perror();

    /* Testing code for gettime() system call */
    /* TODO: How can we initialize a pointer to NULL in user code? */
    char time;

    /* Run a test suite provided by lab course */
    // runjp();
    
    while(1) {
        int ret = gettime();
        if ((ret >= 0) && (ret % 500 == 0)) {
            itoa(ret, &time);
            write(1, &time, strlen(&time));
            write(1, "\n", 2);
        }
    }

    return 0;
}

