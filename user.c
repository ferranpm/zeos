#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main"))) main(void)
{
    /* Run a test suite provided by lab course */
    // runjp_rank(0, 28);

    char *fill = "FILL\n";
    char *pare = "PARE\n";
    int i;

    sem_init(0, 0);
    if (fork() == 0) {
        for (i = 0; i < 20; i++) {
            write(1, fill, strlen(fill));
        }
        sem_signal(0);
    }
    else {
        sem_wait(0);
        for (i = 0; i < 20; i++) {
            write(1, pare, strlen(pare));
        }
    }

    while (1);

    return 0;
}

