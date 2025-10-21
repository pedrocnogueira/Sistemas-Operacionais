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

// Logging detalhado para I/O
static void log_io_cycle(const char* phase, int idx, int wait_q_size){
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] I/O %-8s A%d | Fila I/O: %d processos\n",
        hhmmss, phase, shm->pcb[idx].id, wait_q_size);
}


// Log quando todos estão em DONE
static void log_all_done(){
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] TODOS DONE | Ready: %d, Wait: %d, Done: %d\n",
        hhmmss, shm->ready_q.size, shm->wait_q.size, shm->done_q.size);
}


// Verifica se todos os processos estão em DONE
static void check_all_done(){
    if(shm->done_q.size == shm->nprocs){
        log_all_done();
        time_t t=time(NULL); struct tm* tm=localtime(&t);
        char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
        fprintf(stderr,"[%s] KERNEL: Todos processos DONE, encerrando...\n", hhmmss);
        
        // Ignora todos os sinais para evitar condições de corrida
        signal(SIGCHLD, SIG_IGN);
        signal(SIG_IRQ0, SIG_IGN);
        signal(SIG_IRQ1, SIG_IGN);
        signal(SIG_SYSC, SIG_IGN);
        
        exit(0);
    }
}

// Remove um processo de uma fila (ready_q ou wait_q)
static void remove_from_queue(Queue* q, int idx){
    for(int j = 0; j < q->size; j++){
        int pos = (q->head + j) % MAX_A;
        if(q->q[pos] == idx){
            // Move todos os elementos uma posição para frente
            for(int k = j; k < q->size - 1; k++){
                int curr = (q->head + k) % MAX_A;
                int next = (q->head + k + 1) % MAX_A;
                q->q[curr] = q->q[next];
            }
            q->size--;
            break;
        }
    }
}

// Despacha próximo da ready_q
static void dispatch_next(){
    if(running>=0) return;
    
    while(1){
        int idx = q_pop(&shm->ready_q);
        if(idx<0) return; // fila vazia
        
        // CRÍTICO: Pula processos que já estão DONE
        if(shm->pcb[idx].st == ST_DONE){
            continue; // pega próximo
        }
        
        // Verifica se processo ainda está vivo
        if(kill(shm->pcb[idx].pid, 0) < 0){
            // Processo morreu, marca e tenta próximo
            shm->pcb[idx].st = ST_DONE;
            q_push(&shm->done_q, idx);
            logevt("DEAD     ", idx);
            continue;
        }
        
        // Processo válido, despacha
        running = idx;
        shm->pcb[idx].st = ST_RUNNING;
        
        if(kill(shm->pcb[idx].pid, SIGCONT) < 0){
            perror("SIGCONT failed");
            running = -1;
            continue;
        }
        
        logevt("DISPATCH →", idx);
        break;
    }
}

// Para quem está rodando e empilha de volta nos prontos
static void preempt_running(){
    if(running<0) return;
    
    int idx=running;
    
    // CRÍTICO: Se já está DONE, não preempta
    if(shm->pcb[idx].st == ST_DONE){
        running = -1;
        return;
    }
    
    // Verifica se processo ainda está vivo
    if(kill(shm->pcb[idx].pid, 0) < 0){
        // Processo morreu - marca como DONE e remove de filas
        shm->pcb[idx].st = ST_DONE;
        q_push(&shm->done_q, idx);
        running = -1;
        logevt("DEAD     ", idx);
        check_all_done();
        return;
    }
    
    // Preempção: para o processo e coloca na fila ready
    running=-1;
    shm->pcb[idx].st = ST_READY;
    q_push(&shm->ready_q, idx);
    
    // Envia SIGSTOP para parar o processo
    if(kill(shm->pcb[idx].pid, SIGSTOP) < 0){
        perror("SIGSTOP failed");
    }
    
    // Aguarda um pouco para garantir que o processo parou
    usleep(10000); // 10ms
    
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
    log_io_cycle("INÍCIO", idx, shm->wait_q.size);
}

// ----------------- Handlers de "interrupção" -----------------
static void on_irq0(int s){ // Timer: fim de quantum → preempção RR
    // IRQ0 = fim do quantum, preempta processo atual
    preempt_running();
    // Escolhe próximo processo da fila ready
    dispatch_next();
}
static void on_irq1(int s){ // Fim de I/O: libera 1 da wait_q → ready
    if(!shm->device_busy) return;
    int idx = q_pop(&shm->wait_q);
    shm->device_busy=0; shm->device_curr=-1;
    if(idx>=0){
        shm->pcb[idx].st=ST_READY; q_push(&shm->ready_q, idx);
        log_io_cycle("FIM", idx, shm->wait_q.size);
    }
    try_start_io();       // se tem mais esperando, encadeia novo +3s
    if(running<0) dispatch_next();
}

// Pedido de I/O vindo do app (syscall fake)
static void on_sysc(int s){
    // Processa TODOS os processos que pediram I/O
    for(int i = 0; i < shm->nprocs; i++){
        if(shm->pcb[i].wants_io && (shm->pcb[i].st == ST_RUNNING || shm->pcb[i].st == ST_READY)){
            int idx = i;
            
            // Limpa flag
            shm->pcb[idx].wants_io = 0;
            
            // Se estava rodando, para de rodar
            if(idx == running){
                running = -1;
            }
            
            // Remove da ready_q se estiver lá
            if(shm->pcb[idx].st == ST_READY){
                remove_from_queue(&shm->ready_q, idx);
            }
            
            // Bloqueia em I/O
            kill(shm->pcb[idx].pid, SIGSTOP);
            shm->pcb[idx].st = ST_WAIT_IO;
            q_push(&shm->wait_q, idx);
            log_io_cycle("SOLICITA", idx, shm->wait_q.size);
        }
    }
    
    try_start_io();
    
    if(running < 0){
        dispatch_next();
    }
}

// Flag para indicar que há filhos para reap
static volatile sig_atomic_t children_to_reap = 0;

// Handler SIGCHLD - apenas seta flag
static void on_chld(int s){
    children_to_reap = 1;
}

// Função para reapar filhos mortos
static void reap_children(void){
    int status;
    pid_t pid;
    
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        // Encontra o processo no PCB
        for(int i=0; i<shm->nprocs; i++){
            if(shm->pcb[i].pid == pid){
                // Remove de qualquer fila
                if(running == i) running = -1;
                
                // Remove da ready_q se estiver lá
                if(shm->pcb[i].st == ST_READY){
                    remove_from_queue(&shm->ready_q, i);
                }
                
                // Se está em I/O, remove da fila de I/O
                if(shm->pcb[i].st == ST_WAIT_IO){
                    remove_from_queue(&shm->wait_q, i);
                    
                    // Se era o processo atual em I/O, libera o dispositivo
                    if(shm->device_curr == i){
                        shm->device_busy = 0;
                        shm->device_curr = -1;
                    }
                }
                
                // Marca como terminado
                shm->pcb[i].st = ST_DONE;
                q_push(&shm->done_q, i);
                
                logevt("DONE     ", i);
                check_all_done();
                
                // Despacha outro se necessário
                if(running < 0){
                    dispatch_next();
                }
                break;
            }
        }
    }
    children_to_reap = 0;
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

    dispatch_next();

    // Loop principal do kernel
    for(;;){
        // Verifica se há filhos para reap
        if(children_to_reap){
            reap_children();
        }
        
        // Pequena pausa para permitir processamento de sinais
        usleep(1000); // 1ms
    }
    
    return 0;
}
