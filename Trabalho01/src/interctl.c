#include "common.h"

static Shared* shm = NULL;
static int io_timer_active = 0; // 0 = quantum timer, 1 = I/O timer

static void on_usr1(int s){
    // I/O request recebido - inicia timer de I/O
    io_timer_active = 1;
    alarm(IO_TIME_S);
}

static void on_alarm(int s){

    pid_t kpid = shm->pid_kernel;
    if (kpid > 0) {
        (void)kill(kpid, SIG_IRQ1);
    }
    io_timer_active = 0;
}

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr,"Uso: %s <shmid>\n", argv[0]);
        exit(1);
    }
    
    // Conecta à memória compartilhada
    int shmid = atoi(argv[1]);
    shm = (Shared*)shmat(shmid, NULL, 0);
    if(shm == (void*)-1){ 
        perror("shmat interctl"); 
        exit(1); 
    }

    // Configura handlers de sinais
    signal(SIGUSR1, on_usr1);  // Pedido de I/O
    signal(SIGALRM, on_alarm); // Timer de I/O

    // Loop principal - apenas espera
    for(;;){
        sleep(1);
        pid_t kpid = shm->pid_kernel;
        if (kpid > 0) {
            (void)kill(kpid, SIG_IRQ0);
        }
    }
    
    return 0;
}