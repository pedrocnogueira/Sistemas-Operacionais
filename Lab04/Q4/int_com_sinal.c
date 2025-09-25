#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void fpe_handler(int sig) {
  printf("SIGFPE recebido (%d) durante operacao inteira. Encerrando com "
         "seguranca.\n",
         sig);
  fflush(stdout);
  exit(1);
}

int main(void) {
  int a, b;
  signal(SIGFPE, fpe_handler);
  if (scanf("%d %d", &a, &b) != 2)
    return 1;

  printf("a + b = %d\n", a + b);
  printf("a - b = %d\n", a - b);
  printf("a * b = %d\n", a * b);
  printf("a / b = %d\n", a / b);
  return 0;
}
