#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main"))) main(void)
{
    /* Run a test suite provided by lab course */
    runjp_rank(0,10);

    /*unsigned char buff[10];
    int i, j, k;
    int pid = fork();

    if (pid == 0) {
        while (1);
    }
    else {
        while(1) {
            write(1, "\nnew\n", 5);
            for (i = 0; i < 9999; i++)
                for (j = 0; j < 199; j++);
            read(1, buff, 10);
            write(1, "UEUE\n", 5);
            write(1, buff, 10);
        }
    }*/

    while(1);
    
    return 0;
}

