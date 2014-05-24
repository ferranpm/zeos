#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main"))) main(void)
{
    /* Run a test suite provided by lab course */
    /* runjp(); */

    unsigned char buff[10];

    int i, j, k;
    int pid = fork();
    if (pid == 0) {
        while (1);
    }
    else {
        void *a = sbrk(0);
        a = sbrk(0);
        a = sbrk(0);
        a = sbrk(0);
        int *b = (int*)sbrk(500);
        write(1, "x", 1);
        b[8] = 0;
        while(1);
    }

    return 0;
}

