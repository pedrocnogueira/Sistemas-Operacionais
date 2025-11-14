#include "common.h"

int main(int argc, char** argv){
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <idx> <shmid>\n", argv[0]);
        exit(1);
    }

    int idx   = atoi(argv[1]);
    int shmid = atoi(argv[2]);

    // Anexa à SHM
    Shared* shm = (Shared*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) { perror("shmat app"); exit(1); }

    // Sanidade mínima: idx válido e PCB existente
    if (idx < 0 || idx >= MAX_A || shm->pcb[idx].id == -1) {
        fprintf(stderr, "Atenção: idx=%d inválido para este teste.\n", idx);
        exit(1);
    }

    const int task_id    = shm->pcb[idx].id;
    const int io_profile = shm->pcb[idx].io_profile; // 0: nunca I/O; 1: I/O em PC=3 e 8

    time_t t = time(NULL); struct tm* tmv = localtime(&t);
    char hhmmss[16]; strftime(hhmmss, sizeof(hhmmss), "%H:%M:%S", tmv);
    fprintf(stderr, "[%s] A%d INICIANDO (idx=%d, I/O=%s)\n", hhmmss, task_id, idx, io_profile ? "SIM" : "NAO");

    // Loop de trabalho até completar MAX_PC iterações
    while (shm->pcb[idx].PC < MAX_PC) {
        // Se este app tem perfil de I/O e chegou nos PCs gatilho, solicita I/O ao kernel
        if (io_profile && (shm->pcb[idx].PC == 2 || shm->pcb[idx].PC == 7)) {
            shm->pcb[idx].wants_io = 1;              // pedido pendente
            if (shm->pid_kernel > 0) {
                kill(shm->pid_kernel, SIG_SYSC);     // avisa o kernel
            }
        }

        // Executa UMA iteração e incrementa o PC (contexto do app)
        shm->pcb[idx].PC++;

        // Simula "trabalho" de 1s
        sleep(1);
    }

    t = time(NULL); tmv = localtime(&t);
    strftime(hhmmss, sizeof(hhmmss), "%H:%M:%S", tmv);
    fprintf(stderr, "[%s] A%d TERMINANDO (PC=%d)\n", hhmmss, task_id, shm->pcb[idx].PC);

    return 0;
}
