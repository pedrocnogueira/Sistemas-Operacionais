#include "common.h"

static int should_exit = 0;

static void on_usr1(int s) {
    (void)s;
    should_exit = 1;
}

int main(int argc, char** argv){
    if(argc<2){
        fprintf(stderr,"Uso: %s --test1|--test2|--test3\n",argv[0]);
        exit(1);
    }

    // Cria SHM
    int shmid = shmget(IPC_PRIVATE, sizeof(Shared), IPC_CREAT|0600);
    if (shmid < 0){ perror("shmget"); exit(1); }

    Shared* shm = (Shared*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1){ perror("shmat"); exit(1); }

    // Inicializa SHM
    memset(shm, 0, sizeof(Shared));
    q_init(&shm->ready_q);
    q_init(&shm->wait_q);
    q_init(&shm->done_q);
    for (int i=0;i<MAX_A;i++) shm->pcb[i].id = -1;
    shm->pid_launcher = getpid();

    signal(SIGUSR1, on_usr1);

    // Define processos baseado no teste (compactos desde 0)
    if (strcmp(argv[1],"--test1") == 0){
        shm->nprocs = 3;
        for (int i=0;i<3;i++){
            shm->pcb[i].id = i+1;
            shm->pcb[i].st = ST_NEW;
            shm->pcb[i].PC = 0;
            shm->pcb[i].io_profile = 0; // sem I/O
        }
    } else if (strcmp(argv[1],"--test2") == 0){
        shm->nprocs = 3;
        for (int i=0;i<3;i++){
            shm->pcb[i].id = i+1;
            shm->pcb[i].st = ST_NEW;
            shm->pcb[i].PC = 0;
            shm->pcb[i].io_profile = 1; // com I/O (PC=3 e 8)
        }
    } else if (strcmp(argv[1],"--test3") == 0){
        shm->nprocs = 6;
        for (int i=0;i<6;i++){
            shm->pcb[i].id = i+1;
            shm->pcb[i].st = ST_NEW;
            shm->pcb[i].PC = 0;
            shm->pcb[i].io_profile = (i >= 3); // últimas 3 com I/O
        }
    } else {
        fprintf(stderr, "Teste inválido: %s\n", argv[1]);
        exit(1);
    }

    // Cria InterController
    pid_t pid_inter = fork();
    if (pid_inter == 0){
        char shmid_str[32];
        snprintf(shmid_str, sizeof(shmid_str), "%d", shmid);
        execl("./interctl","interctl", shmid_str, NULL);
        perror("execl interctl"); exit(1);
    }
    shm->pid_inter = pid_inter;

    // Cria Kernel
    pid_t pid_kernel = fork();
    if (pid_kernel == 0){
        char shmid_str[32];
        snprintf(shmid_str, sizeof(shmid_str), "%d", shmid);
        execl("./kernelsim","kernelsim", shmid_str, NULL);
        perror("execl kernelsim"); exit(1);
    }
    shm->pid_kernel = pid_kernel;

    // Aguarda kernel terminar
    while(!should_exit) {
        pause();
    }

    kill(pid_kernel, SIGTERM);
    waitpid(pid_kernel, NULL, 0);
    printf("[LAUNCHER] Kernel finalizado. Encerrando sistema...\n");

    // Encerra o intercontroller e limpa SHM
    kill(pid_inter, SIGTERM);
    waitpid(pid_inter, NULL, 0);

    printf("[LAUNCHER] InterController encerrado.\n");
    printf("[LAUNCHER] Limpando memória compartilhada...\n");

    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    
    printf("[LAUNCHER] Sistema completamente encerrado.\n");
    return 0;
}
