// INF1316 - Lab 6 - Parte 1 (sem semáforos)
// Compilar: gcc -O2 -pthread lab6_part1.c -o lab6_p1
// Rodar: ./lab6_p1            (10 threads)
//       ./lab6_p1 100         (100 threads para forçar corrida)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef N
#define N 10000
#endif

static int A[N];

typedef struct {
    int id;     // 1..T
    int delta;  // -id (ímpar) | +id (par)
    int T;
} TaskArg;

void* worker(void* arg){
    TaskArg* t = (TaskArg*)arg;
    // percorre TODO o vetor aplicando o delta
    for (int i = 0; i < N; i++) {
        // SEÇÃO NÃO-SINCRONIZADA (read-modify-write não atômica)
        A[i] += t->delta;
        // pequena “perturbação” para aumentar interleaving
        if ((i & 1023) == 0) sched_yield();
    }
    return NULL;
}

int main(int argc, char** argv){
    int T = 10;
    if (argc > 1) T = atoi(argv[1]);
    if (T < 1) T = 1;

    for (int i = 0; i < N; i++) A[i] = 10;

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

    // valor esperado para T=10: 10 + (-1+2-3+...+10) = 15
    // para T genérico: soma de sinais alternados de 1..T
    int sum = 0;
    for (int k = 1; k <= T; k++) sum += (k % 2) ? -k : +k;
    int expected = 10 + sum;

    int mism = 0, minv = A[0], maxv = A[0];
    int shown = 0;
    printf("Parte 1 (SEM sincronização) | T=%d\n", T);
    printf("Esperado por posição: %d\n", expected);

    for (int i = 0; i < N; i++){
        if (A[i] < minv) minv = A[i];
        if (A[i] > maxv) maxv = A[i];
        if (A[i] != expected) {
            if (shown < 20) {
                printf("  mismatch: idx=%d valor=%d (esperado=%d)\n", i, A[i], expected);
                shown++;
            }
            mism++;
        }
    }
    printf("Resumo: mismatches=%d | min=%d max=%d\n", mism, minv, maxv);
    if (mism == 0) {
        printf("Nenhuma divergência. Se o professor exigir, aumente T (ex.: 100, 1000).\n");
    }
    free(th); free(ar);
    return 0;
}
