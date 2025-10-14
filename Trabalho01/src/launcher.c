#include "../include/common.h"

static int create_shm(size_t sz){
    int shmid=shmget(IPC_PRIVATE, sz, SHM_PERMS);
    if(shmid<0){ perror("shmget"); exit(1); }
    return shmid;
}

int main(){
    // 1) cria SHM e zera (slide Memória Compartilhada)
    int shmid=create_shm(sizeof(Shared));
    Shared* shm=(Shared*)shmat(shmid,NULL,0);
    if(shm==(void*)-1){ perror("shmat"); exit(1); }
    memset(shm,0,sizeof(*shm));
    q_init(&shm->ready_q); q_init(&shm->wait_q); q_init(&shm->done_q);

    shm->nprocs = USE_A;
    for(int i=0;i<USE_A;i++){
        shm->pcb[i]=(PCB){ .pid=0, .id=i+1, .PC=0, .st=ST_NEW, .wants_io=0 };
        q_push(&shm->ready_q, i); // entram como READY
        shm->pcb[i].st = ST_READY;
    }

    // 2) exporta shmid via variável de ambiente para filhos usarem shmat
    char shmid_str[32]; snprintf(shmid_str,sizeof(shmid_str), "%d", shmid);
    setenv("SO_SHMID", shmid_str, 1);

    // 3) fork kernel
    pid_t k=fork();
    if(k==0){ execl("./kernelsim","kernelsim", NULL); perror("exec kernel"); exit(1); }
    shm->pid_kernel=k;

    // 4) fork interctl
    pid_t ic=fork();
    if(ic==0){ execl("./interctl","interctl", NULL); perror("exec interctl"); exit(1); }
    shm->pid_inter=ic;

    // 5) fork apps/netos
    for(int i=0;i<USE_A;i++){
        pid_t p=fork();
        if(p==0){
            char id[8]; snprintf(id,sizeof(id), "%d", i);
            execl("./app","app", id, NULL);
            perror("exec app"); exit(1);
        }
        shm->pcb[i].pid = p;
    }

    // 6) espera kernel terminar tudo (ou Ctrl-C)
    int status=0; waitpid(k,&status,0);

    // 7) limpeza
    shmdt(shm); shmctl(shmid, IPC_RMID, 0);
    return 0;
}
