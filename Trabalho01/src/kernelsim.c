#include "../include/common.h"

static Shared* shm=NULL;
static int running = -1; // índice do A em execução, -1 se nenhum

// Logging simples
static void logevt(const char* what, int idx){
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    if(idx>=0)
        fprintf(stderr,"[%s] %-10s A%d (st=%s, PC=%d)\n",
            hhmmss, what, shm->pcb[idx].id, sstate(shm->pcb[idx].st), shm->pcb[idx].PC);
    else
        fprintf(stderr,"[%s] %-10s\n", hhmmss, what);
}

// Despacha próximo da ready_q
static void dispatch_next(){
    if(running>=0){ // já tem alguém rodando
        // nada aqui: preempção acontece nos handlers
        return;
    }
    int idx = q_pop(&shm->ready_q);
    if(idx<0){ return; } // nada pronto
    running = idx;
    shm->pcb[idx].st = ST_RUNNING;
    // Continua o processo (se for primeira vez, ainda está parado por default)
    kill(shm->pcb[idx].pid, SIGCONT);
    logevt("DISPATCH →", idx);
}

// Para quem está rodando e empilha de volta nos prontos
static void preempt_running(){
    if(running<0) return;
    int idx=running; running=-1;
    shm->pcb[idx].st = ST_READY;
    q_push(&shm->ready_q, idx);
    kill(shm->pcb[idx].pid, SIGSTOP);
    logevt("PREEMPT  ←", idx);
}

// Inicia serviço de I/O para cabeça da wait_q, se possível
static void try_start_io(){
    if(shm->device_busy) return;
    if(q_empty(&shm->wait_q)) return;
    int idx = shm->device_curr = shm->wait_q.q[shm->wait_q.head]; // olha sem pop
    shm->device_busy=1;
    // pede ao InterController agendar IRQ1 para daqui a IO_TIME_S
    kill(shm->pid_inter, SIGUSR1);
    logevt("I/O START ", idx);
}

// ----------------- Handlers de “interrupção” -----------------
static void on_irq0(int s){ // Timer: fim de quantum → preempção RR
    preempt_running();
    dispatch_next();
}
static void on_irq1(int s){ // Fim de I/O: libera 1 da wait_q → ready
    if(!shm->device_busy) return;
    int idx = q_pop(&shm->wait_q);
    shm->device_busy=0; shm->device_curr=-1;
    if(idx>=0){
        shm->pcb[idx].st=ST_READY; q_push(&shm->ready_q, idx);
        logevt("I/O DONE  ", idx);
    }
    try_start_io();       // se tem mais esperando, encadeia novo +3s
    if(running<0) dispatch_next();
}

// Pedido de I/O vindo do app (syscall fake)
static void on_sysc(int s){
    // Qual app pediu? (heurística: o que está RUNNING marca wants_io=1)
    if(running<0) return;
    int idx=running;
    shm->pcb[idx].wants_io=0;
    // Bloqueia em I/O
    kill(shm->pcb[idx].pid, SIGSTOP);
    shm->pcb[idx].st=ST_WAIT_IO;
    q_push(&shm->wait_q, idx);
    running=-1;
    logevt("TRAP I/O  ", idx);
    try_start_io();
    dispatch_next();
}

// Um app terminou naturalmente (não usamos muito aqui, mas cobre caso)
static void on_chld(int s){
    int status; pid_t p=waitpid(-1,&status,WNOHANG);
    if(p>0){
        for(int i=0;i<shm->nprocs;i++){
            if(shm->pcb[i].pid==p){
                shm->pcb[i].st=ST_DONE; q_push(&shm->done_q,i);
                if(running==i) running=-1;
                logevt("EXIT     ", i);
                break;
            }
        }
    }
}

int main(){
    // SHM
    int shmid=atoi(getenv("SO_SHMID"));
    shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat kernel"); exit(1); }
    shm->pid_kernel=getpid();

    // Handlers (slides de Sinais)
    signal(SIGCHLD, on_chld);
    signal(SIG_IRQ0, on_irq0);
    signal(SIG_IRQ1, on_irq1);
    signal(SIG_SYSC, on_sysc);

    // Inicial: todos READY; escolhe um
    dispatch_next();

    // Loop principal “autônomo” (slide: escalonador como processo)
    for(;;){
        pause(); // desperta só por sinais (IRQ0/IRQ1/syscall/CHLD)
        // encerra quando todos DONE?
        if(shm->done_q.size==shm->nprocs) break;
    }
    // Para garantir morte dos filhos remanescentes
    for(int i=0;i<shm->nprocs;i++) if(shm->pcb[i].st!=ST_DONE) kill(shm->pcb[i].pid,SIGKILL);
    return 0;
}
