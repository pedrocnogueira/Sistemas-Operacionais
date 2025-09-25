#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void fpe_handler(int sig) {
  printf("SIGFPE recebido (%d). Encerrando de forma controlada.\n", sig);
  fflush(stdout);
  exit(1);
}

int main(void) {
  double a, b;
  signal(SIGFPE, fpe_handler);
  if (scanf("%lf %lf", &a, &b) != 2)
    return 1;

  printf("a + b = %.10g\n", a + b);
  printf("a - b = %.10g\n", a - b);
  printf("a * b = %.10g\n", a * b);
  printf("a / b = %.10g\n", a / b);
  return 0;
}
