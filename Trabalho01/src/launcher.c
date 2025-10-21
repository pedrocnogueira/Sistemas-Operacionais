#include "../include/common.h"

// Variáveis globais para cleanup
static Shared* shm = NULL;
static int shmid = -1;
static pid_t pid_kernel = -1;
static pid_t pid_inter = -1;
static pid_t pids_apps[MAX_A];
static int num_apps = 0;

// Handler para SIGINT (Ctrl+C)
static void cleanup_handler(int sig){
    fprintf(stderr, "\n[LAUNCHER] Recebido sinal de interrupção (Ctrl+C). Limpando...\n");
    
    // 1. Mata o kernel (ele vai matar os apps)
    if(pid_kernel > 0){
        fprintf(stderr, "[LAUNCHER] Terminando KernelSim (PID %d)...\n", pid_kernel);
        kill(pid_kernel, SIGTERM);
        waitpid(pid_kernel, NULL, 0);
    }
    
    // 2. Mata o InterController
    if(pid_inter > 0){
        fprintf(stderr, "[LAUNCHER] Terminando InterController (PID %d)...\n", pid_inter);
        kill(pid_inter, SIGTERM);
        waitpid(pid_inter, NULL, 0);
    }
    
    // 3. Mata apps remanescentes (caso kernel não tenha matado)
    for(int i = 0; i < num_apps; i++){
        if(pids_apps[i] > 0){
            // Verifica se ainda está vivo
            if(kill(pids_apps[i], 0) == 0){
                fprintf(stderr, "[LAUNCHER] Terminando App A%d (PID %d)...\n", i+1, pids_apps[i]);
                kill(pids_apps[i], SIGKILL);
                waitpid(pids_apps[i], NULL, 0);
            }
        }
    }
    
    // 4. Libera memória compartilhada
    if(shm != NULL && shm != (void*)-1){
        fprintf(stderr, "[LAUNCHER] Liberando memória compartilhada...\n");
        shmdt(shm);
    }
    
    if(shmid >= 0){
        shmctl(shmid, IPC_RMID, 0);
    }
    
    fprintf(stderr, "[LAUNCHER] Limpeza concluída. Encerrando.\n");
    exit(0);
}

static int create_shm(size_t sz){
    int id = shmget(IPC_PRIVATE, sz, SHM_PERMS);
    if(id < 0){ 
        perror("shmget"); 
        exit(1); 
    }
    return id;
}

// Função para mostrar uso
static void show_usage(const char* progname) {
    fprintf(stderr, "Uso: %s [--test1|--test2|--test3]\n", progname);
    fprintf(stderr, "  --test1: Executa apenas A1, A2, A3 (sem I/O)\n");
    fprintf(stderr, "  --test2: Executa apenas A4, A5, A6 (com I/O)\n");
    fprintf(stderr, "  --test3: Executa todas as 6 tarefas (A1-A6)\n");
    fprintf(stderr, "  Sem parâmetros: Executa teste padrão (A1, A2, A3)\n");
}

int main(int argc, char* argv[]){
    // Parse dos argumentos
    int test_mode = 1; // padrão: teste 1
    int num_tasks = TASKS_WITHOUT_IO;
    int start_id = 1; // A1
    
    if (argc > 1) {
        if (strcmp(argv[1], "--test1") == 0) {
            test_mode = 1;
            num_tasks = TASKS_WITHOUT_IO;
            start_id = 1; // A1, A2, A3
        } else if (strcmp(argv[1], "--test2") == 0) {
            test_mode = 2;
            num_tasks = TASKS_WITH_IO;
            start_id = 4; // A4, A5, A6
        } else if (strcmp(argv[1], "--test3") == 0) {
            test_mode = 3;
            num_tasks = MAX_A; // A1, A2, A3, A4, A5, A6
            start_id = 1;
        } else {
            show_usage(argv[0]);
            exit(1);
        }
    }
    
    fprintf(stderr, "[LAUNCHER] Iniciando Teste %d com %d tarefas (A%d-A%d)\n", 
            test_mode, num_tasks, start_id, start_id + num_tasks - 1);

    // Instala handler de SIGINT ANTES de criar qualquer coisa
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler); // também trata SIGTERM
    
    // 1) cria SHM e zera
    shmid = create_shm(sizeof(Shared));
    shm = (Shared*)shmat(shmid, NULL, 0);
    if(shm == (void*)-1){ 
        perror("shmat"); 
        exit(1); 
    }
    
    memset(shm, 0, sizeof(*shm));
    q_init(&shm->ready_q); 
    q_init(&shm->wait_q); 
    q_init(&shm->done_q);

    shm->nprocs = num_tasks;
    num_apps = num_tasks;
    
    for(int i = 0; i < num_tasks; i++){
        int task_id = start_id + i; // ID real da tarefa (1-6)
        shm->pcb[i] = (PCB){ .pid=0, .id=task_id, .PC=0, .st=ST_NEW, .wants_io=0 };
        q_push(&shm->ready_q, i);
        shm->pcb[i].st = ST_READY;
        pids_apps[i] = 0; // inicializa
    }

    // 2) exporta shmid via variável de ambiente
    char shmid_str[32]; 
    snprintf(shmid_str, sizeof(shmid_str), "%d", shmid);
    setenv("SO_SHMID", shmid_str, 1);

    // 3) fork kernel
    pid_kernel = fork();
    if(pid_kernel == 0){ 
        execl("./kernelsim", "kernelsim", NULL); 
        perror("exec kernel"); 
        exit(1); 
    }
    shm->pid_kernel = pid_kernel;

    // 4) fork interctl
    pid_inter = fork();
    if(pid_inter == 0){ 
        execl("./interctl", "interctl", NULL); 
        perror("exec interctl"); 
        exit(1); 
    }
    shm->pid_inter = pid_inter;

    // 5) fork apps/netos
    for(int i = 0; i < num_tasks; i++){
        pid_t p = fork();
        if(p == 0){
            char id[8]; 
            snprintf(id, sizeof(id), "%d", i); // índice na tabela PCB (0-5)
            execl("./app", "app", id, NULL);
            perror("exec app"); 
            exit(1);
        }
        pids_apps[i] = p;
        shm->pcb[i].pid = p;
    }

    fprintf(stderr, "[LAUNCHER] Sistema iniciado. Pressione Ctrl+C para interromper.\n");

    // 6) Aguarda kernel terminar normalmente (como no código original)
    int status = 0; 
    waitpid(pid_kernel, &status, 0);
    
    fprintf(stderr, "[LAUNCHER] KernelSim terminou. Limpando processos restantes...\n");

    // 7) Limpeza normal (quando kernel termina por conta própria)
    // Mata InterController
    if(pid_inter > 0){
        kill(pid_inter, SIGTERM);
        waitpid(pid_inter, NULL, 0);
    }
    
    // Mata apps remanescentes
    for(int i = 0; i < num_apps; i++){
        if(pids_apps[i] > 0 && kill(pids_apps[i], 0) == 0){
            kill(pids_apps[i], SIGKILL);
            waitpid(pids_apps[i], NULL, 0);
        }
    }
    
    // Libera SHM
    shmdt(shm); 
    shmctl(shmid, IPC_RMID, 0);
    
    fprintf(stderr, "[LAUNCHER] Sistema encerrado automaticamente após conclusão de todos os processos.\n");
    return 0;
}