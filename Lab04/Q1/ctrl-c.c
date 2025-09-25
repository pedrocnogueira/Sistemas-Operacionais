// ctrl-c.c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define EVER for (;;)

void intHandler(int sinal) {
  printf("Voce pressionou Ctrl-C (SIGINT=%d)\n", sinal);
  fflush(stdout);
}

void quitHandler(int sinal) {
  printf("Recebi Ctrl-\\ (SIGQUIT=%d). Terminando o processo...\n", sinal);
  fflush(stdout);
  exit(0);
}

int main(void) {

  signal(SIGINT, intHandler);
  signal(SIGQUIT, quitHandler);

  puts("Ctrl-C tratado (nao termina). Use Ctrl-\\ para terminar.\n\n");
  EVER;
}
