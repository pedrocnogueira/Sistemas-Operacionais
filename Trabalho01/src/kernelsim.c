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
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] DEBUG: check_all_done - Done: %d/%d\n", hhmmss, shm->done_q.size, shm->nprocs);
    
    if(shm->done_q.size == shm->nprocs){
        log_all_done();
        fprintf(stderr,"[%s] KERNEL: Todos DONE - AUTO-SUICÍDIO em 3 segundos...\n", hhmmss);
        fflush(stderr);
        
        // AUTO-SUICÍDIO: mata a si mesmo após 3 segundos
        alarm(3);
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
        // Processo morreu
        shm->pcb[idx].st = ST_DONE;
        q_push(&shm->done_q, idx);
        running = -1;
        logevt("DEAD     ", idx);
        return;
    }
    
    running=-1;
    shm->pcb[idx].st = ST_READY;
    q_push(&shm->ready_q, idx);
    
    if(kill(shm->pcb[idx].pid, SIGSTOP) < 0){
        perror("SIGSTOP failed");
    }
    
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

// Um app terminou naturalmente (não usamos muito aqui, mas cobre caso)
// Melhorar handler SIGCHLD
static void on_chld(int s){
    int status;
    pid_t p;
    
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] DEBUG: SIGCHLD recebido\n", hhmmss);
    
    // Processa TODOS os filhos que terminaram
    while((p = waitpid(-1, &status, WNOHANG)) > 0){
        for(int i=0; i<shm->nprocs; i++){
            if(shm->pcb[i].pid == p){
                fprintf(stderr,"[%s] DEBUG: Processo A%d (PID=%d) terminou via SIGCHLD\n", hhmmss, shm->pcb[i].id, p);
                
                // Remove de qualquer fila
                if(running == i) running = -1;
                
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
                
                logevt("EXIT     ", i);
                check_all_done();
                
                // Despacha outro se necessário
                if(running < 0){
                    dispatch_next();
                }
                break;
            }
        }
    }
}
// Handler para término explícito de app
static void on_app_exit(int s){
    // Log de debug
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] DEBUG: on_app_exit chamado, running=%d\n", hhmmss, running);
    
    // Descobre qual app está terminando
    // Procura por qualquer processo que não esteja DONE e tenha PC >= 12
    int idx = -1;
    
    for(int i = 0; i < shm->nprocs; i++){
        if(shm->pcb[i].PC >= 12 && shm->pcb[i].st != ST_DONE){
            idx = i;
            fprintf(stderr,"[%s] DEBUG: Processo A%d (PC=%d, st=%s) terminando\n", hhmmss, shm->pcb[idx].id, shm->pcb[idx].PC, sstate(shm->pcb[idx].st));
            break;
        }
    }
    
    if(idx < 0) {
        fprintf(stderr,"[%s] DEBUG: Nenhum processo encontrado para terminar\n", hhmmss);
        return;
    }
    
    // Remove de execução
    if(running == idx) running = -1;
    
    // Se está em I/O, remove da fila de I/O
    if(shm->pcb[idx].st == ST_WAIT_IO){
        remove_from_queue(&shm->wait_q, idx);
        
        // Se era o processo atual em I/O, libera o dispositivo
        if(shm->device_curr == idx){
            shm->device_busy = 0;
            shm->device_curr = -1;
        }
    }
    
    // Para o processo
    kill(shm->pcb[idx].pid, SIGSTOP);
    
    // Marca como DONE
    shm->pcb[idx].st = ST_DONE;
    q_push(&shm->done_q, idx);
    
    logevt("EXIT     ", idx);
    check_all_done();
    
    // Despacha próximo
    if(running < 0){
        fprintf(stderr,"[%s] DEBUG: Despachando próximo após EXIT\n", hhmmss);
        dispatch_next();
    }
}

// Handler para SIGTERM (shutdown do launcher)
static void on_shutdown(int s){
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] KERNEL: Recebido SIGTERM, encerrando IMEDIATAMENTE...\n", hhmmss);
    fflush(stderr);
    
    // SAÍDA IMEDIATA - SEM CLEANUP, SEM DELAYS
    _exit(0);
}

// Handler para SIGALRM (auto-suicídio)
static void on_auto_suicide(int s){
    time_t t=time(NULL); struct tm* tm=localtime(&t);
    char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
    fprintf(stderr,"[%s] KERNEL: AUTO-SUICÍDIO executado!\n", hhmmss);
    fflush(stderr);
    
    // SUICÍDIO IMEDIATO
    _exit(0);
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
    signal(SIG_EXIT, on_app_exit);  // ← NOVO handler
    signal(SIGTERM, on_shutdown);   // ← Handler para shutdown
    signal(SIGALRM, on_auto_suicide); // ← Handler para auto-suicídio

    dispatch_next();

    // KERNEL: Funciona eternamente como em uma máquina real
    // Apenas responde a sinais e gerencia processos
    for(;;){
        pause();
        
        // Log de debug para verificar estado (apenas quando todos estão DONE)
        if(shm->done_q.size == shm->nprocs) {
            time_t t=time(NULL); struct tm* tm=localtime(&t);
            char hhmmss[16]; strftime(hhmmss,sizeof(hhmmss),"%H:%M:%S",tm);
            fprintf(stderr,"[%s] KERNEL: Todos processos DONE, aguardando novos processos...\n", hhmmss);
        }
    }
    
    // Este ponto nunca deve ser alcançado
    return 0;
}
