#include <stdio.h>
#include <unistd.h>

int main(void) {
    // loop eterno; imprime de vez em quando só p/ visual
    unsigned long c = 0;
    for(;;) {
        if ((++c % 10000000UL) == 0) {
            printf("filho %d rodando...\n", getpid());
            fflush(stdout);
            usleep(100000); // 0.1 s só p/ não inundar o terminal
        }
    }
}
