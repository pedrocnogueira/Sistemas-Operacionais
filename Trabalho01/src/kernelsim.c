#include "common.h"

// ================== Estado do Kernel ==================
static Shared* shm = NULL;
static int running = -1; // índice do A em execução, -1 se nenhum

// Flags acionadas por handlers (handlers NÃO fazem trabalho pesado)
static int flag_irq0 = 0;        // tick/quantum
static int flag_irq1 = 0;        // fim de I/O
static int flag_sysc = 0;        // syscall fake
static int flag_chld = 0;        // término de filho(s)

// ================== Utilidades de log ==================
static void now_hhmmss(char buf[16]) {
    time_t t = time(NULL);
    struct tm* tmv = localtime(&t);
    strftime(buf, 16, "%H:%M:%S", tmv);
}

static void logevt(const char* what, int idx) {
    char hhmmss[16]; now_hhmmss(hhmmss);
    if (idx >= 0) {
        fprintf(stderr, "[%s] %-10s A%d (st=%s, PC=%d)\n",
                hhmmss, what, shm->pcb[idx].id, sstate(shm->pcb[idx].st), shm->pcb[idx].PC);
    } else {
        fprintf(stderr, "[%s] %-10s\n", hhmmss, what);
    }
}

static void log_io_cycle(const char* phase, int idx, int wait_sz) {
    char hhmmss[16]; now_hhmmss(hhmmss);
    fprintf(stderr, "[%s] I/O %-8s A%d | Fila I/O: %d processos\n",
            hhmmss, phase, shm->pcb[idx].id, wait_sz);
}

static void log_all_done(void) {
    char hhmmss[16]; now_hhmmss(hhmmss);
    fprintf(stderr, "[%s] TODOS DONE | Ready: %d, Wait: %d, Done: %d\n",
            hhmmss, shm->ready_q.size, shm->wait_q.size, shm->done_q.size);
}

// ================== Filas: utilitário ==================
static void remove_from_queue(Queue* q, int idx){
    if (q->size == 0) return;

    // procura a posição lógica (0..size-1) do idx dentro da janela [head, head+size)
    int found = -1;
    for (int j = 0; j < q->size; j++){
        int pos = (q->head + j) % MAX_A;
        if (q->q[pos] == idx){ found = j; break; }
    }
    if (found < 0) return; // idx não está na fila

    // compacta: shift à esquerda dentro da janela lógica
    for (int k = found; k < q->size - 1; k++){
        int cur = (q->head + k) % MAX_A;
        int nxt = (q->head + k + 1) % MAX_A;
        q->q[cur] = q->q[nxt];
    }

    // reduz o tamanho e corrige tail de acordo com o novo size
    q->size--;
    q->tail = (q->head + q->size) % MAX_A;
}

// ================== Núcleo do fluxo ==================
static void check_all_done(void) {
    if (shm->done_q.size == shm->nprocs) {
        log_all_done();
        char hhmmss[16]; now_hhmmss(hhmmss);
        fprintf(stderr, "[%s] KERNEL: Todos processos DONE\n", hhmmss);

        // Evita corridas enquanto encerramos
        signal(SIGCHLD, SIG_IGN);
        signal(SIG_IRQ0, SIG_IGN);
        signal(SIG_IRQ1, SIG_IGN);
        signal(SIG_SYSC, SIG_IGN);

        kill(shm->pid_launcher, SIGUSR1);
    }
}

static void mark_done(int i) {
    // retirar de qualquer “papel” ativo
    if (running == i) running = -1;

    if (shm->pcb[i].st == ST_READY) {
        remove_from_queue(&shm->ready_q, i);
    }
    if (shm->pcb[i].st == ST_WAIT_IO) {
        remove_from_queue(&shm->wait_q, i);
        if (shm->device_curr == i) {
            shm->device_busy = 0;
            shm->device_curr = -1;
        }
    }

    shm->pcb[i].st = ST_DONE;
    q_push(&shm->done_q, i);
    logevt("DONE      ", i);
    check_all_done();
}

static void try_start_io(void) {
    if (shm->device_busy) return;
    if (q_empty(&shm->wait_q)) return;

    int idx = shm->wait_q.q[shm->wait_q.head]; // peek (sem pop neste momento)
    shm->device_busy = 1;
    shm->device_curr = idx;

    // avisar InterController: “começou I/O agora”
    if (shm->pid_inter > 0) {
        kill(shm->pid_inter, SIGUSR1);
    }
    log_io_cycle("INÍCIO", idx, shm->wait_q.size);
}

static void dispatch_next(void) {
    if (running >= 0) return;

    while (1) {
        int idx = q_pop(&shm->ready_q);
        if (idx < 0) return; // fila vazia

        if (shm->pcb[idx].st == ST_DONE) continue;

        // defensivo: existe ainda?
        if (shm->pcb[idx].pid <= 0 || kill(shm->pcb[idx].pid, 0) < 0) {
            mark_done(idx);
            continue;
        }

        // despachar
        shm->pcb[idx].st = ST_RUNNING;
        running = idx;

        if (kill(shm->pcb[idx].pid, SIGCONT) < 0) {
            // morreu nesse meio tempo
            running = -1;
            mark_done(idx);
            continue;
        }

        logevt("DISPATCH →", idx);
        return;
    }
}

static void preempt_running(void) {
    if (running < 0) return;
    int idx = running;

    // parar ANTES de re-enfileirar (evita corrida)
    if (kill(shm->pcb[idx].pid, SIGSTOP) < 0) {
        // se morreu, finalize
        running = -1;
        mark_done(idx);
        return;
    }

    running = -1;
    shm->pcb[idx].st = ST_READY;
    q_push(&shm->ready_q, idx);
    logevt("PREEMPT  ←", idx);
}

// ================== “Handlers” (só flags!) ==================
static void h_irq0(int s){ (void)s; flag_irq0 = 1; }
static void h_irq1(int s){ (void)s; flag_irq1 = 1; }
static void h_sysc(int s){ (void)s; flag_sysc = 1; }
static void h_chld(int s){ (void)s; flag_chld = 1; }

// ================== Tratadores no laço principal ==================
static void handle_irq0(void) {
    flag_irq0 = 0;
    preempt_running();
    dispatch_next();

                    time_t t = time(NULL); struct tm* tm = localtime(&t);
        char hhmmss[16]; strftime(hhmmss, sizeof(hhmmss), "%H:%M:%S", tm);

        fprintf(stderr, "[%s] KERNEL - Running: %d, Ready: [", hhmmss, running);
        for(int i = 0; i < shm->ready_q.size; i++){
            int pos = (shm->ready_q.head + i) % MAX_A;
            fprintf(stderr, "%d", shm->ready_q.q[pos]);
            if(i < shm->ready_q.size - 1) fprintf(stderr, ",");
        }
        fprintf(stderr, "] Wait: [");
        for(int i = 0; i < shm->wait_q.size; i++){
            int pos = (shm->wait_q.head + i) % MAX_A;
            fprintf(stderr, "%d", shm->wait_q.q[pos]);
            if(i < shm->wait_q.size - 1) fprintf(stderr, ",");
        }
        fprintf(stderr, "] Done: [");
        for(int i = 0; i < shm->done_q.size; i++){
            int pos = (shm->done_q.head + i) % MAX_A;
            fprintf(stderr, "%d", shm->done_q.q[pos]);
            if(i < shm->done_q.size - 1) fprintf(stderr, ",");
        }
        fprintf(stderr, "]\n");

}

static void handle_irq1(void) {
    flag_irq1 = 0;

    if (!shm->device_busy) return; // spurious
    int idx = q_pop(&shm->wait_q);
    shm->device_busy = 0;
    shm->device_curr = -1;

    if (idx >= 0) {
        shm->pcb[idx].st = ST_READY;
        q_push(&shm->ready_q, idx);
        log_io_cycle("FIM", idx, shm->wait_q.size);
    }

    // encadear próximo I/O, se houver
    try_start_io();

    if (running < 0) dispatch_next();
}

static void handle_sysc(void) {
    flag_sysc = 0;

    // varre e consome pedidos (wants_io = 1)
    for (int i = 0; i < shm->nprocs; i++) {
        if (!shm->pcb[i].wants_io) continue;

        // consome o pedido
        shm->pcb[i].wants_io = 0;

        // só bloqueia quem está READY ou RUNNING
        if (shm->pcb[i].st == ST_RUNNING || shm->pcb[i].st == ST_READY) {
            // se estava RUNNING, pare antes
            if (i == running) {
                kill(shm->pcb[i].pid, SIGSTOP);
                running = -1;
            } else if (shm->pcb[i].st == ST_READY) {
                remove_from_queue(&shm->ready_q, i);
            }

            // bloqueia em I/O
            shm->pcb[i].st = ST_WAIT_IO;
            q_push(&shm->wait_q, i);
            log_io_cycle("SOLICITA", i, shm->wait_q.size);
        }
    }

    // se o dispositivo está livre, inicia atendimento
    try_start_io();

    if (running < 0) dispatch_next();
}

static void reap_children(void) {
    flag_chld = 0;

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // localizar PCB pelo pid
        int who = -1;
        for (int i = 0; i < shm->nprocs; i++) {
            if (shm->pcb[i].pid == pid) { who = i; break; }
        }
        if (who < 0) continue; // pid estranho (não esperado), ignore

        // se estava no I/O, não deixe o dispositivo preso
        if (shm->pcb[who].st == ST_WAIT_IO) {
            remove_from_queue(&shm->wait_q, who);
            if (shm->device_curr == who) {
                shm->device_busy = 0;
                shm->device_curr = -1;
                try_start_io(); // encadeia próximo
            }
        }
        // se estava pronto, retire da fila
        if (shm->pcb[who].st == ST_READY) {
            remove_from_queue(&shm->ready_q, who);
        }
        // se era o que rodava
        if (running == who) {
            running = -1;
        }

        mark_done(who);

        if (running < 0) dispatch_next();
    }
}

// ================== Main ==================
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <shmid>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    shm = (Shared*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat kernel");
        return 1;
    }
    shm->pid_kernel = getpid();

    // Instala handlers (apenas flags)
    struct sigaction sa = {0};

    sa.sa_handler = h_chld;  sigemptyset(&sa.sa_mask); sigaction(SIGCHLD, &sa, NULL);
    sa.sa_handler = h_irq0;  sigemptyset(&sa.sa_mask); sigaction(SIG_IRQ0, &sa, NULL);
    sa.sa_handler = h_irq1;  sigemptyset(&sa.sa_mask); sigaction(SIG_IRQ1, &sa, NULL);
    sa.sa_handler = h_sysc;  sigemptyset(&sa.sa_mask); sigaction(SIG_SYSC, &sa, NULL);

    // Cria os apps como FILHOS do kernel (para receber SIGCHLD corretamente)
    for (int i = 0; i < shm->nprocs; i++) {
        if (shm->pcb[i].id == -1) continue;

        pid_t pid = fork();
        if (pid == 0) {
            // filho: exec app
            char idx_str[16], shmid_str[32];
            snprintf(idx_str, sizeof(idx_str), "%d", i);
            snprintf(shmid_str, sizeof(shmid_str), "%d", shmid);
            execl("./app", "app", idx_str, shmid_str, NULL);
            perror("execl app");
            _exit(1);
        }
        shm->pcb[i].pid = pid;
        shm->pcb[i].st  = ST_READY;
        q_push(&shm->ready_q, i);

        usleep(50000); // 50ms para evitar busy loop
    }

    // Primeiro despacho
    dispatch_next();

    // Laço principal
    for (;;) {
        if (flag_chld) reap_children();
        if (flag_irq1) handle_irq1();
        if (flag_sysc) handle_sysc();
        if (flag_irq0) handle_irq0();

        usleep(1000); // 1ms para evitar busy loop
    }

    return 0;
}
