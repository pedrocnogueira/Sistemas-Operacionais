#include "../include/common.h"

static Shared* shm=NULL;
static int io_countdown = 0;  // contador para I/O (0 = inativo)

static void on_alarm(int s){
    // IRQ0: timer periódico a cada QUANTUM_S
    if(shm->pid_kernel>0) {
        kill(shm->pid_kernel, SIG_IRQ0);
    }
    
    // Decrementa contador de I/O se ativo
    if(io_countdown > 0){
        io_countdown--;
        if(io_countdown == 0){
            // I/O terminou! Envia IRQ1
            if(shm->pid_kernel>0) {
                kill(shm->pid_kernel, SIG_IRQ1);
            }
        }
    }
    
    alarm(QUANTUM_S); // reprograma IRQ0
}

static void on_start_io(int s){
    // Kernel nos sinaliza para começar a contar IO_TIME_S
    // Como IRQ0 dispara a cada QUANTUM_S (1s), precisamos contar IO_TIME_S vezes
    io_countdown = IO_TIME_S;  // 3 segundos = 3 ticks
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