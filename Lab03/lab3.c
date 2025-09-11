#include <stdio.h>     // printf, perror
#include <sys/ipc.h>   // IPC_*, key_t
#include <sys/shm.h>   // shmget, shmat, shmdt, shmctl
#include <sys/stat.h>  // S_IRUSR, S_IWUSR
#include <sys/types.h> // pid_t, key_t
#include <sys/wait.h>  // waitpid
#include <unistd.h>    // fork, _exit

#define N 10000
#define WORKERS 10
#define CHUNK (N / WORKERS)

/* --- helpers System V --- */
static int shm_create(size_t bytes) {
  int shmid =
      shmget(IPC_PRIVATE, bytes, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  if (shmid == -1) {
    perror("shmget");
    _exit(1);
  }
  return shmid;
}

static void *shm_attach(int shmid) {
  void *addr = shmat(shmid, NULL, 0); // RW attach
  if (addr == (void *)-1) {
    perror("shmat");
    _exit(1);
  }
  return addr;
}

static void shm_detach(void *addr) {
  if (shmdt(addr) == -1) {
    perror("shmdt");
    _exit(1);
  }
}

static void shm_destroy(int shmid) {
  // Marca para remoção; é liberado quando o último processo der shmdt
  if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("shmctl IPC_RMID");
    _exit(1);
  }
}

/* --- util --- */
static void fill_int(int *a, int n, int v) {
  for (int i = 0; i < n; i++)
    a[i] = v;
}

/* ---------------- EX1: Paralelismo (faixas disjuntas) ---------------- */
static void run_ex1(void) {
  // cria segmentos: vetor principal e vetor de somas parciais
  int shmid_a = shm_create(sizeof(int) * N);
  int shmid_sums = shm_create(sizeof(int) * WORKERS);

  // anexa no coordenador (pai); os filhos herdam o mapeamento após fork()
  int *a = (int *)shm_attach(shmid_a);
  int *sums = (int *)shm_attach(shmid_sums);

  pid_t pids[WORKERS];

  fill_int(a, N, 10);
  for (int w = 0; w < WORKERS; w++)
    sums[w] = 0;

  for (int w = 0; w < WORKERS; w++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      _exit(1);
    }
    if (pid == 0) {
      int start = w * CHUNK;
      int end = start + CHUNK;
      int s = 0;
      for (int i = start; i < end; i++) {
        a[i] *= 2;
        s += a[i];
      }
      sums[w] = s; // “retorno” da parcela do trabalhador w
      _exit(0);
    }
    pids[w] = pid;
  }

  for (int w = 0; w < WORKERS; w++) {
    waitpid(pids[w], 0, 0);
  }

  int total = 0;
  for (int w = 0; w < WORKERS; w++) {
    total += sums[w];
  }

  int expected = N * 20; // 10000 * 10 * 2 = 200000
  printf("EX1: soma total = %d | esperada = %d | %s\n", total, expected, (total == expected) ? "OK" : "ERRO");

  // limpeza
  shm_detach(a);
  shm_detach(sums);
  shm_destroy(shmid_a);
  shm_destroy(shmid_sums);
}

/* ---------- EX2: Concorrência (todos tocam tudo, sem lock) ----------- */
static void run_ex2(void) {

  int shmid_a = shm_create(sizeof(int) * N);
  int *a = (int *)shm_attach(shmid_a);

  pid_t pids[WORKERS];

  fill_int(a, N, 10);

  for (int w = 0; w < WORKERS; w++) {

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      _exit(1);
    }

    if (pid == 0) {
      for (int i = 0; i < N; i++) {
        int t = a[i]; //read
        a[i] = t * 2; // write
        a[i] += 2;    // write
      }
      _exit(0);
    }

    pids[w] = pid;
  }

  for (int w = 0; w < WORKERS; w++) {
    waitpid(-1, 0, 0);
  }

  int first = a[0], minv = a[0], maxv = a[0];
  int sum = 0;
  for (int i = 0; i < N; i++) {
    if (a[i] < minv)
      minv = a[i];
    if (a[i] > maxv)
      maxv = a[i];
    sum += a[i];
  }

  const int expected = 12286; // 2^10*10 + 2*(2^10-1)
  printf("EX2: first=%d min=%d max=%d avg=%.2f esperado=%d\n", first, minv, maxv, (double)sum / N, expected);

  if (minv != maxv || first != expected) {
    printf("EX2: divergência indica condições de corrida (lost updates) em R-M-W não atômico.\n");
  } else {
    printf("EX2: coincidiu com o esperado nesta execução, mas sem garantias sem sincronização.\n");
  }

  // limpeza
  shm_detach(a);
  shm_destroy(shmid_a);
}

int main(void) {
  run_ex1();
  run_ex2();
  return 0;
}
