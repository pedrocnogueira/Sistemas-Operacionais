// lab1_processos.c
// Alunos:
// - Pedro Carneiro Nogueira - 2310540
// - Hannah Barbosa Goldstein - 2310160
// -----------------------------------------------------------------------------
// Objetivo do Lab:
// - Criar três processos relacionados (PAI -> FILHO -> NETO)
// - Cada um executa 100 iterações, atualizando sua própria cópia da variável N:
//     PAI   : soma +1 em cada iteração
//     FILHO : soma +2 em cada iteração
//     NETO  : soma +5 em cada iteração
// - FILHO deve esperar o NETO terminar; PAI deve esperar o FILHO terminar.
// - Observação importante: após fork(), cada processo tem sua *própria cópia*
//   de N (modelo copy-on-write). Logo, os incrementos não se "veem" entre eles.
// - As saídas (printf) podem sair intercaladas, pois os processos rodam em
// paralelo.
//
// Notas rápidas:
// - waitpid(-1, ...) espera por *qualquer* filho (o primeiro que terminar/mudar
// de estado).
//   Aqui funciona porque cada processo tem apenas um filho relevante.
//   Se houvesse vários filhos, seria preciso laçar waitpid ou esperar um PID
//   específico.
// - _exit(0) termina o processo imediatamente (sem rodar handlers de atexit). É
// comum usar
//   em processos filhos após fork().
// -----------------------------------------------------------------------------

#include <stdio.h>    // printf, perror
#include <stdlib.h>   // exit, EXIT_*
#include <sys/wait.h> // waitpid
#include <unistd.h>   // fork, getpid, getppid, _exit

#define ITERS 100 // Quantidade de iterações que cada processo executará

int main(void) {

  int N =
      0; // Variável base (cada processo terá sua *própria* cópia após os forks)

  // ---------------------------------------------------------------------------
  // 1) Primeiro fork: cria o FILHO a partir do PAI
  //    - Retorno no PAI  : PID (>0) do FILHO
  //    - Retorno no FILHO: 0
  // ---------------------------------------------------------------------------
  pid_t pid_filho = fork();

  if (pid_filho == 0) {
    // -------------------------------------------------------------------------
    // *** Estamos no FILHO ***
    // 2) Segundo fork: o FILHO cria o NETO
    //    - Retorno no FILHO: PID (>0) do NETO
    //    - Retorno no NETO : 0
    // -------------------------------------------------------------------------
    pid_t pid_neto = fork();

    if (pid_neto == 0) {
      // -----------------------------------------------------------------------
      // *** Estamos no NETO ***
      // NETO: executa as iterações
      // -----------------------------------------------------------------------
      for (int i = 0; i < ITERS; i++) {
        N += 5;
        printf("NETO: iter %03d, N = %03d\n", i, N);
      }
      printf("NETO: Finalizando\n");

      // Termina o NETO. Usamos _exit para encerrar sem interferir em buffers
      // herdados.
      exit(0);
    } else {
      // -----------------------------------------------------------------------
      // *** Ainda no FILHO ***
      // FILHO: executa as iterações
      // -----------------------------------------------------------------------
      for (int i = 0; i < ITERS; i++) {
        N += 2;
        printf("FILHO: iter %03d, N = %03d\n", i, N);
      }

      // FILHO espera o NETO terminar.
      // Aqui usamos waitpid(-1, ...) que espera "qualquer" filho; como há
      // apenas o NETO, está correto. Em códigos com múltiplos filhos, prefira
      // waitpid(pid_neto, ...).
      waitpid(-1, NULL, 0);

      printf("FILHO: Finalizando\n");
      exit(0); // Encerra o FILHO
    }
  } else {
    // -------------------------------------------------------------------------
    // *** Estamos no PAI ***
    // PAI: executa as iterações
    // -------------------------------------------------------------------------
    for (int i = 0; i < ITERS; i++) {
      N += 1;
      printf("PAI: iter %03d, N = %03d\n", i, N);
    }

    // PAI espera o FILHO terminar.
    waitpid(-1, NULL, 0);

    printf("PAI: Finalizando\n");
    return 0; // Encerra o PAI (processo original)
  }
}
