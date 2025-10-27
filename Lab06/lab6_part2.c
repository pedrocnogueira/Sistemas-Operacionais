// INF1316 - Lab 6 - Parte 2 (com semáforos POSIX nomeados)
// Compilar:  gcc -O2 -pthread lab6_part2.c -o lab6_p2
// Rodar: ./lab6_p2            (10 threads)
//       ./lab6_p2 100         (100 threads)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>      // O_CREAT
#include <sys/stat.h>   // modos 0666
#include <unistd.h>
#include <string.h>

#ifndef N
#define N 10000
#endif

static int A[N];
static sem_t* sem_global;         // semáforo nomeado
static const char* SEM_NAME = "/lab6_mutex_demo";

typedef struct {
    int id;     // 1..T
    int delta;  // -id (ímpar) | +id (par)
    int T;
} TaskArg;

void* worker(void* arg){
    TaskArg* t = (TaskArg*)arg;
    for (int i = 0; i < N; i++){
        sem_wait(sem_global);     // Down
        A[i] += t->delta;         // seção crítica
        sem_post(sem_global);     // Up
        if ((i & 1023) == 0) sched_yield();
    }
    return NULL;
}

int main(int argc, char** argv){
    int T = 10;
    if (argc > 1) T = atoi(argv[1]);
    if (T < 1) T = 1;

    for (int i = 0; i < N; i++) A[i] = 10;

    // cria/abre semáforo nomeado com valor inicial 1
    sem_global = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem_global == SEM_FAILED){
        perror("sem_open");
        return 1;
    }

    pthread_t* th = malloc(sizeof(pthread_t) * T);
    TaskArg*   ar = malloc(sizeof(TaskArg)   * T);

    for (int k = 1; k <= T; k++){
        ar[k-1].id = k;
        ar[k-1].delta = (k % 2) ? -k : +k;
        ar[k-1].T = T;
        if (pthread_create(&th[k-1], NULL, worker, &ar[k-1]) != 0){
            perror("pthread_create"); return 1;
        }
    }
    for (int k = 0; k < T; k++) pthread_join(th[k], NULL);

    // fecha e remove o semáforo nomeado
    sem_close(sem_global);
    sem_unlink(SEM_NAME);

    // validação
    int sum = 0;
    for (int k = 1; k <= T; k++) sum += (k % 2) ? -k : +k;
    int expected = 10 + sum;

    int mism = 0, minv = A[0], maxv = A[0];
    for (int i = 0; i < N; i++){
        if (A[i] < minv) minv = A[i];
        if (A[i] > maxv) maxv = A[i];
        if (A[i] != expected) mism++;
    }

    printf("Parte 2 (COM semáforo nomeado) | T=%d\n", T);
    printf("Esperado por posição: %d | min=%d max=%d | mismatches=%d\n",
           expected, minv, maxv, mism);
    if (mism == 0) printf("OK: nenhuma divergência (corrida eliminada).\n");

    free(th); free(ar);
    return 0;
}
