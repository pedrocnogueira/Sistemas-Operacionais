#include "../include/common.h"

static Shared* shm=NULL;
static int idx=-1;   // índice no vetor PCB (passado pelo argv)
static int maxPC=12; // quando chega aqui, “termina”

// Para rodar sob controle do Kernel, iniciamos “parados”:
static void on_start(int s){ /* ignore: só precisa existir para SIGCONT */ }

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"usage: app <idx>\n"); return 1; }
    idx=atoi(argv[1]);

    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat app"); exit(1); }

    signal(SIGCONT, on_start);
    raise(SIGSTOP); // entra “dormindo” até o kernel dar SIGCONT pela 1ª vez

    // Loop principal do “programa de usuário”
    // Emula CPU-bound com PC++, pedindo I/O em PC==3 e PC==8
    while(shm->pcb[idx].PC<maxPC){
        // faz “trabalho”
        sleep(1);
        shm->pcb[idx].PC++;

        if(shm->pcb[idx].PC==3 || shm->pcb[idx].PC==8){
            shm->pcb[idx].wants_io=1;
            // sinaliza syscall fake ao Kernel (trap)
            kill(shm->pid_kernel, SIG_SYSC);
            // devolve CPU (o Kernel nos dará SIGSTOP na troca)
        }
    }
    // terminou
    return 0;
}
