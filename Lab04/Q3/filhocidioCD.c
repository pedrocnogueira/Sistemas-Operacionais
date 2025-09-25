// filhocidio.c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void childhandler(int sig) {
  int status;
  (void)sig;     // silencia aviso
  wait(&status); // reap do filho
  printf("Filho terminou (SIGCHLD).\n");
  fflush(stdout);
  exit(0); // encerra o pai ao saber que o filho terminou
}

int main(int argc, char *argv[]) {
  pid_t pid;
  int delay;

  if (argc < 3) {
    fprintf(stderr, "Uso: %s tempo programafilho [arg1 ... argn]\n", argv[0]);
    exit(1);
  }

  // instala tratador para SIGCHLD
  signal(SIGCHLD, childhandler);

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    // ---- FILHO ----
    execvp(argv[2], &argv[2]);
    exit(1);
  } else {
    // ----- PAI -----
    delay = atoi(argv[1]);
    sleep(delay); // espera o tempo-limite

    // se o filho já terminou, o handler acima já terá finalizado o pai.
    // se ainda não terminou, mata o filho com SIGKILL (como no slide).
    printf("Tempo limite (%d s) esgotado. Enviando SIGKILL ao filho %d.\n", delay, (int)pid);
    fflush(stdout);
    kill(pid, SIGKILL);

    // aguarda o SIGCHLD do término do filho (o handler chamará exit(0))
    pause();
  }
  return 0;
}
