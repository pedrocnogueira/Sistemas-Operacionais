// coordenador.c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t spawn_filho(const char *prog) {
  pid_t p = fork();
  if (p < 0) {
    perror("fork");
    exit(1);
  }
  if (p == 0) {                       // filho
    execlp(prog, prog, (char *)NULL); // execvp simplificado
    perror("exec");
    exit(1);
  }
  return p; // pai recebe PID do filho
}

int main(void) {
  // 1) cria dois filhos que executam programa de loop eterno
  pid_t f1 = spawn_filho("./loop_infinito");
  pid_t f2 = spawn_filho("./loop_infinito");

  // 2) para os dois (garante controle inicial)
  kill(f1, SIGSTOP);
  kill(f2, SIGSTOP);

  // 3) roda por 15 segundos alternando 1s cada (preempção por sinais)
  for (int t = 0; t < 15; ++t) {
    pid_t run = (t % 2 == 0) ? f1 : f2;  // quem vai rodar agora
    pid_t stop = (t % 2 == 0) ? f2 : f1; // quem deve ficar parado

    kill(stop, SIGSTOP); // garante o outro parado
    kill(run, SIGCONT);  // continua o escolhido
    sleep(1);            // deixa rodar 1 segundo
    kill(run, SIGSTOP);  // preempção: para após 1s
  }

  // 4) mata os filhos e encerra
  kill(f1, SIGKILL);
  kill(f2, SIGKILL);
  wait(NULL);
  wait(NULL); // reap
  puts("Coordenador terminou.");
  return 0;
}
