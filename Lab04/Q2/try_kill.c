#include <signal.h>
#include <stdio.h>

void h(int s) { printf("handler SIGKILL? %d\n", s); }

int main(void) {
  if (signal(SIGKILL, h) == (void *)-1) {
    puts("Nao foi possivel instalar handler para SIGKILL");
  } else {
    puts("Instalou handler (nao esperado)");
  }
  for (;;); // manter o processo vivo
}
