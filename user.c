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
        while (1) {
            write(1, "\nnew\n", 5);
            for (i = 0; i < 9999; i++)
                for (j = 0; j < 199; j++);
            read(1, buff, 1);
            write(1, buff, 1);
            /* write(1, "Direct: ", 8); */
            /* write(1, keyboard_buffer.buff, 10); */
            /* write(1, "Read: ", 6); */
            /* write(1, buff, sizeof(buff)); */
        }
    }

    return 0;
}

