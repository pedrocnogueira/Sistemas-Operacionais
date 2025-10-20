#pragma once
// Cabeçalhos padrão
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// ---------- Parâmetros de simulação ----------
#define MAX_A      6        // até 6 apps/netos
#define USE_A      3        // quantos lançar de fato
#define QUANTUM_S  1        // IRQ0 a cada 1s (slide: timer) 
#define IO_TIME_S  3        // IRQ1 = fim de I/O após 3s

// ---------- Estados ----------
typedef enum { ST_NEW, ST_READY, ST_RUNNING, ST_WAIT_IO, ST_DONE } State;

// ---------- PCB (Tabela de Processos, por slide) ----------
typedef struct {
    pid_t pid;
    int   id;        // A1=1, A2=2...
    int   PC;        // “program counter” simulado
    State st;
    int   wants_io;  // pedido de I/O pendente (syscall fake)
} PCB;

// Filas circulares simples
typedef struct {
    int q[MAX_A];
    int head, tail, size;
} Queue;

static inline void q_init(Queue* q){ q->head=q->tail=q->size=0; }
static inline int  q_empty(Queue* q){ return q->size==0; }
static inline int  q_push(Queue* q,int v){
    if(q->size==MAX_A) return -1;
    q->q[q->tail]=(v); q->tail=(q->tail+1)%MAX_A; q->size++; return 0;
}
static inline int  q_pop(Queue* q){
    if(q->size==0) return -1;
    int v=q->q[q->head]; q->head=(q->head+1)%MAX_A; q->size--; return v;
}

// ---------- Área de Memória Compartilhada ----------
typedef struct {
    // PIDs de serviços
    pid_t pid_kernel;
    pid_t pid_inter;

    // PCBs dos apps
    int   nprocs;          // quantos A em uso
    PCB   pcb[MAX_A];

    // Filas de estados (índices em pcb[])
    Queue ready_q;
    Queue wait_q;
    Queue done_q;

    // Dispositivo D1
    int   device_busy;     // 0 livre, 1 ocupado
    int   device_curr;     // idx do A atendido
} Shared;

#define SHM_PERMS (IPC_CREAT|IPC_EXCL|0600)

// Chaves de sinais (separadas conforme slides de sinais)
#define SIG_IRQ0  SIGALRM   // timer
#define SIG_IRQ1  SIGUSR2   // fim de I/O
#define SIG_SYSC  SIGUSR1   // pedido de I/O (syscall fake)
#define SIG_EXIT  SIGTERM   // aviso de término

// Helpers
static inline const char* sstate(State s){
    switch(s){ case ST_NEW: return "NEW"; case ST_READY: return "READY";
        case ST_RUNNING:return "RUNNING"; case ST_WAIT_IO:return "WAIT_IO";
        case ST_DONE:return "DONE"; default:return "?"; }
}
