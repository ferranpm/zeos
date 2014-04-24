#include <libc.h>

char buff[24];
int pid;

void func_clone() {
    char *s = "Hola soc el clone\n";
    write(1, s, strlen(s));
    exit(-1);
}

int pila[1000];

int __attribute__ ((__section__(".text.main"))) main(void)
{
    /* Run a test suite provided by lab course */
    // runjp_rank(0, 28);

    clone(func_clone, &pila[1000]);
    while (1);

    return 0;
}

