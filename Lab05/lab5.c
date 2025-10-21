// lab5.c — versão do LAB3 usando THREADS (pthreads)
// INF1316 – Sistemas Operacionais
// Hannah Goldstein e Pedro Carneiro Nogueira — Paralelismo e Concorrência com Memória Compartilhada

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N 10000
#define WORKERS 10
#define CHUNK (N / WORKERS)

typedef struct {
    int id;
    int *a;
    int *sums;
} ThreadArgs;

/* ----------- utilitários ----------- */
static void fill_int(int *a, int n, int v) {
    for (int i = 0; i < n; i++)
        a[i] = v;
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ----------- EX1: Paralelismo (faixas disjuntas) ----------- */
void *worker_ex1(void *arg) {
    ThreadArgs *ta = (ThreadArgs *)arg;
    int start = ta->id * CHUNK;
    int end = start + CHUNK;
    int soma = 0;
    for (int i = start; i < end; i++) {
        ta->a[i] *= 2;
        soma += ta->a[i];
    }
    ta->sums[ta->id] = soma;
    return NULL;
}

static void run_ex1(void) {
    int *a = malloc(sizeof(int) * N);
    int *sums = calloc(WORKERS, sizeof(int));
    pthread_t threads[WORKERS];
    ThreadArgs args[WORKERS];

    fill_int(a, N, 10);

    double t0 = now_sec();

    for (int i = 0; i < WORKERS; i++) {
        args[i].id = i;
        args[i].a = a;
        args[i].sums = sums;
        pthread_create(&threads[i], NULL, worker_ex1, &args[i]);
    }

    for (int i = 0; i < WORKERS; i++)
        pthread_join(threads[i], NULL);

    double t1 = now_sec();

    int total = 0;
    for (int i = 0; i < WORKERS; i++)
        total += sums[i];

    int expected = N * 20; // 10000 * (10 * 2)
    printf("EX1 (threads): soma total = %d | esperada = %d | %s | tempo = %.6fs\n",
           total, expected, (total == expected ? "OK" : "ERRO"), t1 - t0);

    free(a);
    free(sums);
}

/* ----------- EX2: Concorrência (todos tocam tudo) ----------- */
void *worker_ex2(void *arg) {
    int *a = (int *)arg;
    for (int i = 0; i < N; i++) {
        int t = a[i];
        t *= 2;
        t += 2;
        a[i] = t;
    }
    return NULL;
}

static void run_ex2(void) {
    int *a = malloc(sizeof(int) * N);
    pthread_t threads[WORKERS];

    fill_int(a, N, 10);

    double t0 = now_sec();

    for (int i = 0; i < WORKERS; i++)
        pthread_create(&threads[i], NULL, worker_ex2, a);

    for (int i = 0; i < WORKERS; i++)
        pthread_join(threads[i], NULL);

    double t1 = now_sec();

    int first = a[0], minv = a[0], maxv = a[0];
    long long sum = 0;
    for (int i = 0; i < N; i++) {
        if (a[i] < minv) minv = a[i];
        if (a[i] > maxv) maxv = a[i];
        sum += a[i];
    }

    const int expected = 12286; // 2^10*10 + 2*(2^10-1)
    printf("EX2 (threads): first=%d min=%d max=%d avg=%.2f esperado=%d | tempo = %.6fs\n",
           first, minv, maxv, (double)sum / N, expected, t1 - t0);

    if (minv != maxv || first != expected)
        printf("EX2: divergência -> condições de corrida (lost updates)\n");
    else
        printf("EX2: coincidiu nesta execução, mas sem garantias sem sincronização\n");

    free(a);
}

/* ----------- main ----------- */
int main(void) {
    run_ex1();
    run_ex2();
    return 0;
}
