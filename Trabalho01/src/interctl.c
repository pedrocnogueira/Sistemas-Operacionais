#include "../include/common.h"

static Shared* shm=NULL;
static int io_countdown = 0;  // contador para I/O (0 = inativo)
static time_t io_start_time = 0;  // timestamp do início do I/O

static void on_alarm(int s){
    // IRQ0: timer periódico a cada QUANTUM_S
    if(shm->pid_kernel>0) {
        kill(shm->pid_kernel, SIG_IRQ0);
    }
    
    // Verifica se I/O deve terminar (baseado em tempo real)
    if(io_countdown > 0){
        time_t now = time(NULL);
        if(now - io_start_time >= IO_TIME_S){
            // I/O terminou! Envia IRQ1
            io_countdown = 0;  // marca como inativo
            if(shm->pid_kernel>0) {
                kill(shm->pid_kernel, SIG_IRQ1);
            }
        }
    }
    
    alarm(QUANTUM_S); // reprograma IRQ0
}

static void on_start_io(int s){
    // Kernel nos sinaliza para começar a contar IO_TIME_S segundos reais
    io_countdown = 1;  // marca como ativo
    io_start_time = time(NULL);  // registra timestamp do início
}


int main(){
    // anexa SHM
    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat interctl"); exit(1); }

    // Handlers
    signal(SIGUSR1, on_start_io);  // pedido para iniciar I/O
    signal(SIGALRM, on_alarm);     // timer unificado

    alarm(QUANTUM_S); // inicia IRQ0 periódico

    // loop trivial
    for(;;) pause();
}