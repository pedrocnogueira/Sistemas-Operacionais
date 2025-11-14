#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#define IRQ0 SIGUSR1     // sinal de preempcao por timeslice
#define IRQ1 SIGUSR2     // sinal de aviso p o fim de I/O
#define MAX 30        // Iterações máximas por processo


int *pc;        
int *estado;    
int *io_req;   
pid_t *pid_apps;
int segmento_pc, segmento_estado, segmento_pid, segmento_io;
int fila_prontos[7];
int fila_espera[7];
int ini_p = 0, fim_p = 0, ini_e = 0, fim_e = 0;
int n;
int atual = -1;
int cont_preempcao = 0;
time_t inicio_exec, fim_exec;


//tad de fila, p usar no prontos e espera
void enfileira(int fila[], int* fim, int valor){
    fila[*fim] = valor;
    *fim = (*fim + 1) % 7;
}
int desenfileira(int fila[], int* ini, int fim){
    if (*ini == fim) return -1;
    int v = fila[*ini];
    *ini = (*ini + 1) % 7;
    return v;
}

void printf_fila(const char* nome, int fila[], int ini, int fim){

    printf("%s: ", nome);

    int i = ini;
    while (i != fim) 
    {
        printf("A%d ", fila[i]); i = (i + 1) % 7;
    }
    printf("\n");
}

//handlers de sinais 
void handler_irq0(int s){

    if (atual == -1) //CPU VAZIA
        return;

    cont_preempcao++;
    printf("mandou IRQ0 -> preempção de A%d.\n", atual);
    kill(pid_apps[atual], SIGSTOP);
    
    if (estado[atual] == 1) 
        estado[atual] = 0;   // EXEC -> PRONTO

    if (estado[atual] != 3 && estado[atual] != 2)
        enfileira(fila_prontos, &fim_p, atual);
   
    //coloca o atual de prontos na cpu
    int prox = desenfileira(fila_prontos, &ini_p, fim_p);
    if (prox != -1) {
        atual = prox;
        estado[atual] = 1;
        printf(" Executando A%d \n", atual);
        kill(pid_apps[atual], SIGCONT);
    } else {
        atual = -1;
    }
}

void handler_irq1(int s){
    int p = desenfileira(fila_espera, &ini_e, fim_e);

    if (p != -1 && estado[p] != 3) {
        printf("mandou IRQ1 -> fim de I/O.\n");
        estado[p] = 0; // PRONTO
        enfileira(fila_prontos, &fim_p, p);
        printf("A%d sai de esp e ta em prontos\n", p);
       
        if (atual == -1) {
            int prox = desenfileira(fila_prontos, &ini_p, fim_p);
            if (prox != -1) {
                atual = prox;
                estado[atual] = 1;
                printf(" Executando A%d \n", atual);
                kill(pid_apps[atual], SIGCONT);
            }
        }
    }
}

//PROCESSOS GENERICOS 
void processoA(int id){

    //apps ignoram SIGUSR1/SIGUSR2 — apenas o Kernel trata IRQ0/IRQ1
    //tivemos alguns probls c isso entao colocamos esses ignores e esse raise para começarem parados
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    raise(SIGSTOP); 

    while (1) {
        if (estado[id] == 3) 
            break;
        
        if (pc[id] >= MAX) {
            printf("App %d terminou (PC=%d)\n", id, pc[id]);
            estado[id] = 3;
            exit(0);
        }
        //incrementa PC ao receber CPU (após SIGCONT do Kernel)
        pc[id]++;
        printf("App %d executando (PC=%d)\n", id, pc[id]);

        //determinamos que os procs A2, A3, A4 fazem I/O a cada 10 iterações
        if ((id == 1 || id == 2 || id == 3) && (pc[id] % 10 == 0) && pc[id] < MAX) 
        {
            printf("App %d iniciou I/O (D1) \n", id); //TEMPO DE IO do enunc
            estado[id] = 2; // entra em ESPERA
            io_req[id] = 1;// avisa o KERNEL sim com o vet de mem compartilhada q mudou estado
            raise(SIGSTOP);      
            continue;
        }
        sleep(1); 
        // Kernel q vai fazer controle das filas e escalar procs com os IRQ0 e IRQ1
    }
    exit(0);
}


void intercontroller(pid_t kernel_pid){
    int cont = 0;
    while (1) {
        sleep(1); // 1s p que cd loop tenha dur de 1s, gera timeslice 1s e sinal de I/O
        kill(kernel_pid, IRQ0);  // passou 1 s e gera preemp por tempo
        cont++;
        if (cont == 3) {

            kill(kernel_pid, IRQ1);  // a cada 3 s gera aviso de fim de I/O, manda aviso para o kernel
            cont = 0;
        }
    }
}


void kernelSim(int num_Ais){

    signal(IRQ0, handler_irq0);
    signal(IRQ1, handler_irq1);
    //inicializa a fila de PRONTOS com todos os apps
    for (int i = 0; i < num_Ais; i++) 
    {
        enfileira(fila_prontos, &fim_p, i);
    }

    atual = desenfileira(fila_prontos, &ini_p, fim_p);
    if (atual != -1) {
        estado[atual] = 1;
        printf("Executando A%d \n", atual);
        kill(pid_apps[atual], SIGCONT);
    }

    //loop até todos concluírem MAX
    while (1) {
        //aqui colocamos os procs q pedem i/o na fila de espera
        for (int i = 0; i < num_Ais; i++) {
            if (io_req[i]) {

                io_req[i] = 0; 
                if (estado[i] == 2) {
                    enfileira(fila_espera, &fim_e, i);
                    printf("A%d vai para ESPERA\n", i);
                }
                if (atual == i) 
                {
                    atual = -1; //se era o atual que entrou em I/O, garanta que CPU fique livre
                }
            }
        }

        //aqui verificamos se quai procs concluiram MAX
        int concluidos = 0; // qnts ja concluiram 
        for (int i = 0; i < num_Ais; i++) {
            if (pc[i] >= MAX && estado[i] != 3) {
                estado[i] = 3;
                printf("processo A%d finalizado - (PC=%d) \n", i, pc[i]);
                kill(pid_apps[i], SIGSTOP);
                //manda stop pro proc p parar de executar
            }
            if (estado[i] == 3) 
            {
                concluidos++;
            }
        }

        if (concluidos == num_Ais) 
        {
            break; //se todos concluirem MAX, termina o loop
        }

        if (atual == -1) // esse -1 indica q cpu ta vazia, tem q escalar alguem
        {
            int prox = desenfileira(fila_prontos, &ini_p, fim_p);
            if (prox != -1) {
                atual = prox;
                estado[atual] = 1;
                printf("Executando A%d - cpu vazia \n", atual);
                kill(pid_apps[atual], SIGCONT);
                //manda continue pro proc p continuar executando
            }
        }

        
        printf("\nStatus Atual:\n");
        for (int i = 0; i < num_Ais; i++) 
        {
            if (estado[i] == 0)       
            {
                printf("A%d: pronto (PC=%d)\n", i, pc[i]);
            }
            else if (estado[i] == 1) 
            {
                printf("A%d: exec (PC=%d)\n", i, pc[i]);
            }
            else if (estado[i] == 2) 
            {
                printf("A%d: espera i/o (PC=%d)\n", i, pc[i]);
            }
            else                     
            {
                printf("A%d: concluido (PC=%d)\n", i, pc[i]); 
            }
        }
        //aqui imprimimos as filas de PRONTOS e ESPERA
        printf_fila("Fila PRONTOS", fila_prontos, ini_p, fim_p);
        printf_fila("Fila ESPERA",  fila_espera,  ini_e, fim_e);
        sleep(1);
    }


    printf("\nTodos os processos completaram %d iterações.\n", MAX);

    for (int i = 0; i < num_Ais; i++) 
    {
        kill(pid_apps[i], SIGKILL);
    }

    exit(0);
}


int main(int argc, char* argv[]){
    // recebe como parametro o numero de apps na linha de comando
    //ai vamos verificar se o numero de apps esta entre 3 e 6

    if (argc < 2) {
        printf("Uso: %s <3..6>\n", argv[0]);
        exit(1);
    }
    //colocamos essa func atoi para converter o parametro de string para int
    n = atoi(argv[1]);

    if (n < 3 || n > 6) {
        puts("n deve ser 3..6");
        exit(1);
    }

    //como sao procs diferentes vamos usar o shm para compartilhar a memoria entre eles

    //prim, o vetor com os contadores de iteracoes de cada app
    segmento_pc = shmget(IPC_PRIVATE, sizeof(int)* n, IPC_CREAT | S_IRUSR | S_IWUSR);
    pc = (int*) shmat(segmento_pc, 0, 0); //pc[i] eh o contador de iteracoes do app i
    //dps, o vetor com os estados de cada app (pronto, executando, esperando, finalizado)
    segmento_estado = shmget(IPC_PRIVATE, sizeof(int)* n, IPC_CREAT | S_IRUSR | S_IWUSR);
    estado = (int*) shmat(segmento_estado, 0, 0); //estado[i] eh o estado do app i
    // 0=PRONTO, 1=EXEC, 2=ESPERA, 3=FIM
    
    //dps, o vetor com os pedidos de I/O de cada app (0 ou 1, 1 p pedido de I/O e 0 p nao ter pedido de I/O)
    segmento_io = shmget(IPC_PRIVATE, sizeof(int)* n, IPC_CREAT | S_IRUSR | S_IWUSR);
    io_req = (int*) shmat(segmento_io, 0, 0); //io_req[i] eh o pedido de I/O do app i
    //dps, o vetor com os pids de cada app (pid_t eh o tipo do pid)
    segmento_pid = shmget(IPC_PRIVATE, sizeof(pid_t)* n, IPC_CREAT | S_IRUSR | S_IWUSR);
    pid_apps = (pid_t*) shmat(segmento_pid, 0, 0); //pid_apps[i] eh o pid do app i

    //so inicialisando os vetores que criamos
    for (int i = 0; i < n; i++)
    { 
        pc[i] = 0; 
        estado[i] = 0;
        io_req[i] = 0; 
    }

    //criamos o kernel, intercontroel e os apps (com o n da linha de comando)
    //KERNEL
    pid_t kernel_pid = fork();
    if (kernel_pid < 0) {
        perror("erro do fork do kernel");
        exit(1);
    }
    else if (kernel_pid == 0) {
        //se o fork deu certo e o processo eh o filho, executa o kernelSim
        kernelSim(n);
        exit(0);
    }

    //INTERCONTROLLER
    pid_t inter_pid = fork();
    if (inter_pid < 0) {
        perror("erro do fork do intercontroller");
        exit(1);
    }
    else if (inter_pid == 0) {
        intercontroller(kernel_pid);
        exit(0);
    }

    //APPS
    for (int i = 0; i < n; i++) {

        pid_t pid_app = fork();
        if (pid_app < 0) {
            perror("erro do fork do app");
            exit(1);
        }
        else if (pid_app == 0) {
            //se o fork deu certo e o processo eh o filho, executa o processoA
            processoA(i);
            exit(0);
        }
        else {
            //se o fork deu certo e o processo eh o pai, guarda o pid do filho
            pid_apps[i] = pid_app;
        }
    }

    // esse wait eh p esperar o kernel terminar p garantir q todos completem MAX
    waitpid(kernel_pid, NULL, 0);

    //manda kill pro intercontroller p terminar e parar de gerar sinais
    kill(inter_pid, SIGKILL);
    for (int i = 0; i < n; i++)
    {
        waitpid(pid_apps[i], NULL, 0);
    }

    //liberamos a memoria compartilhada
    shmdt(pc); 
    shmdt(estado); 
    shmdt(io_req); 
    shmdt(pid_apps);
    shmctl(segmento_pc, IPC_RMID, 0);
    shmctl(segmento_estado, IPC_RMID, 0);
    shmctl(segmento_io, IPC_RMID, 0);
    shmctl(segmento_pid, IPC_RMID, 0);


    puts("Acabou o programa");
    return 0;
}