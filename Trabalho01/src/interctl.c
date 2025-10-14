#include "../include/common.h"

static Shared* shm=NULL;

static void on_alarm(int s){
    // IRQ0 para o Kernel (timer) — slide SIGALRM
    if(shm->pid_kernel>0) kill(shm->pid_kernel, SIG_IRQ0);
    alarm(QUANTUM_S); // reprograma
}

static void on_start_io(int s){
    // Kernel nos sinaliza para começar a contar IO_TIME_S para o processo atual
    alarm(IO_TIME_S);
}

int main(){
    // anexa SHM (somente leitura serve)
    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat interctl"); exit(1); }

    // Handlers
    signal(SIG_IRQ1, SIG_IGN);           // não usamos aqui
    signal(SIG_IRQ0, SIG_IGN);           // idem
    signal(SIGUSR1, on_start_io);        // “começar temporizador de I/O”
    signal(SIGALRM, on_alarm);

    alarm(QUANTUM_S); // inicia IRQ0 periódico

    // loop trivial
    for(;;) pause();
}
