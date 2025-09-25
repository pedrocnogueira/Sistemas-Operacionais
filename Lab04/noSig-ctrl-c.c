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
  void (*p)(int);

  // p = signal(SIGINT, intHandler);
  printf("Endereco do manipulador anterior de SIGINT: %p\n", (void *)p);

  // p = signal(SIGQUIT, quitHandler);
  printf("Endereco do manipulador anterior de SIGQUIT: %p\n", (void *)p);

  puts("Ctrl-C tratado (nao termina). Use Ctrl-\\ para terminar.");
  EVER;
}
