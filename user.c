#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    /* Testing code for write() system call and perror() function */
    /* char *string = "Hola mon\n"; */
    /* if (write(2, string, strlen(string)) < 0) perror(); */
    /* if (write(1, string, strlen(string)) < 0) perror(); */
    /* if (write(1, 0, strlen(string)) < 0) perror(); */
    /* if (write(1, string, -3) < 0) perror(); */

    /* Testing code for gettime() system call */
    /* char time; */

    /* Run a test suite provided by lab course */
    /* runjp(); */

    /* Testing code for getpid() system call */
    char char_pid;
    pid = getpid();
    if (pid < 0) perror();
    else {
        itoa(pid, &char_pid);
        write(1, "getpid(): ", 11);
        write(1, &char_pid, strlen(&char_pid));
    }
    write(1, "\n", 2);

    /* Testing code for system call fork() */
    /* TODO: Figures out another better way to tests it */
    pid = fork();
    if (pid < 0) {
        perror();
    }
    else if (pid == 0) {
        while (1)
            write(1, "FILL\n", 6);
    }
    else {
        while (1)
            write(1, "PARE\n", 6);
    }

    return 0;
}

