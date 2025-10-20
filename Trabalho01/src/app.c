#include "../include/common.h"

static Shared* shm=NULL;
static int idx=-1;
static int maxPC=12;

static void on_cont(int s){}

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"usage: app <idx>\n"); return 1; }
    idx=atoi(argv[1]);

    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat app"); exit(1); }

    signal(SIGCONT, on_cont);
    
    raise(SIGSTOP);

    // Loop principal
    while(shm->pcb[idx].PC < maxPC){
        sleep(1);
        shm->pcb[idx].PC++;

        if(shm->pcb[idx].PC == 3 || shm->pcb[idx].PC == 8){
            time_t t=time(NULL); struct tm* tm=localtime(&t);
            char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
            fprintf(stderr,"[%s] A%d SOLICITA I/O (PC=%d)\n", hhmmss, shm->pcb[idx].id, shm->pcb[idx].PC);
            
            shm->pcb[idx].wants_io = 1;
            kill(shm->pid_kernel, SIG_SYSC);
            raise(SIGSTOP);
            
            t=time(NULL); tm=localtime(&t);
            strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
            fprintf(stderr,"[%s] A%d I/O CONCLUÍDO (PC=%d)\n", hhmmss, shm->pcb[idx].id, shm->pcb[idx].PC);
            shm->pcb[idx].wants_io = 0;
        }
    }
    
    // Terminou: AVISA O KERNEL ANTES DE SAIR
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] A%d TERMINANDO (PC=%d)\n", hhmmss, shm->pcb[idx].id, shm->pcb[idx].PC);
    
    // CRÍTICO: Sinaliza término ao kernel ANTES de exit
    kill(shm->pid_kernel, SIG_EXIT);
    
    // Aguarda um pouco para garantir que kernel processou
    usleep(100000); // 100ms
    
    exit(0);
}