#include "../include/common.h"

static Shared* shm=NULL;
static int io_countdown = 0;  // contador para I/O (0 = inativo)
static volatile sig_atomic_t should_exit = 0; // flag para encerrar o intercontroller

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

// Handler para SIGTERM/SIGINT (shutdown do launcher)
static void on_shutdown(int s){
    // Apenas seta flag - async-signal-safe
    should_exit = 1;
}

int main(){
    // anexa SHM
    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat interctl"); exit(1); }

    // Handlers
    signal(SIGUSR1, on_start_io);  // pedido para iniciar I/O
    signal(SIGALRM, on_alarm);     // timer unificado
    signal(SIGTERM, on_shutdown);  // handler para shutdown
    signal(SIGINT, on_shutdown);   // handler para shutdown (Ctrl+C)

    alarm(QUANTUM_S); // inicia IRQ0 periódico

    // loop com verificação de shutdown
    for(;;) {
        pause();
        
        // Verifica se deve encerrar
        if(should_exit) {
            break;
        }
    }
}