#include <stdio.h>
#include <stdlib.h>

#define MAX_A 6

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

void print_queue(Queue* q, const char* name) {
    printf("%s: head=%d, tail=%d, size=%d, queue=[", name, q->head, q->tail, q->size);
    for(int i = 0; i < q->size; i++) {
        int pos = (q->head + i) % MAX_A;
        printf("%d", q->q[pos]);
        if(i < q->size - 1) printf(",");
    }
    printf("]\n");
}

int main() {
    Queue q;
    q_init(&q);
    
    printf("=== Teste da Fila Circular ===\n");
    
    // Teste 1: Push sequencial
    printf("\n1. Push sequencial (0,1,2):\n");
    q_push(&q, 0);
    print_queue(&q, "Após push(0)");
    q_push(&q, 1);
    print_queue(&q, "Após push(1)");
    q_push(&q, 2);
    print_queue(&q, "Após push(2)");
    
    // Teste 2: Pop sequencial
    printf("\n2. Pop sequencial:\n");
    int val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    // Teste 3: Push/Pop intercalado
    printf("\n3. Push/Pop intercalado:\n");
    q_push(&q, 3);
    q_push(&q, 4);
    print_queue(&q, "Após push(3,4)");
    
    val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    q_push(&q, 5);
    print_queue(&q, "Após push(5)");
    
    val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    val = q_pop(&q);
    printf("Pop retornou: %d\n", val);
    print_queue(&q, "Após pop");
    
    return 0;
}
