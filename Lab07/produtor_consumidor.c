// Questão 1 - Produtor com 2 consumidores (assíncrono) usando System V message queue

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define MSG_KEY 0x1234
#define TOTAL_MESSAGES 256
#define QSIZE_ELEMS 64

// mtype: 1 = dado, 9 = poison-pill (encerrar)
struct message {
    long mtype;
    int  num;
};

static void set_queue_max64(int msgid) {
    struct msqid_ds ds;
    if (msgctl(msgid, IPC_STAT, &ds) == -1) {
        perror("IPC_STAT");
        exit(1);
    }
    // Cada mensagem carrega só um int => 4 bytes. 64 elementos = 64 * 4 = 256 bytes.
    // (Cabeça de mensagem é gerida pelo kernel; aqui limitamos o payload total.)
    ds.msg_qbytes = QSIZE_ELEMS * sizeof(int);
    if (msgctl(msgid, IPC_SET, &ds) == -1) {
        // Em alguns sistemas, reduzir é permitido, aumentar acima de msgmnb não.
        // Como estamos reduzindo para 256 bytes, deve funcionar em modo usuário.
        perror("IPC_SET (ajuste de msg_qbytes)");
        exit(1);
    }
}

void produtor(int msgid) {
    struct message msg;
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        msg.mtype = 1;
        msg.num   = i;
        if (msgsnd(msgid, &msg, sizeof(int), 0) == -1) {
            perror("msgsnd (dado)");
            exit(1);
        }
        printf("[PRODUTOR] Enviou %d\n", msg.num);
        fflush(stdout);
        sleep(1);
    }

    // Envia 2 poison pills (uma para cada consumidor)
    for (int k = 0; k < 2; k++) {
        msg.mtype = 9;
        msg.num   = -1;
        if (msgsnd(msgid, &msg, sizeof(int), 0) == -1) {
            perror("msgsnd (poison)");
            exit(1);
        }
    }
}

void consumidor(int msgid, int id, int intervalo) {
    struct message msg;
    int recebidas = 0;
    for (;;) {
        if (msgrcv(msgid, &msg, sizeof(int), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        if (msg.mtype == 9) {
            // poison pill → encerra imediatamente (sem sleep)
            printf("[CONSUMIDOR %d] Recebeu POISON. Finalizou. Total recebidas: %d\n",
                   id, recebidas);
            fflush(stdout);
            break;
        }

        // Mensagem de dado
        printf("[CONSUMIDOR %d] Recebeu %d\n", id, msg.num);
        fflush(stdout);
        recebidas++;

        // Ritmo de consumo específico de cada consumidor
        sleep(intervalo);
    }
}

int main(void) {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    // Limitar a fila a 64 elementos (payloads de 4 bytes cada)
    set_queue_max64(msgid);

    pid_t c1 = fork();
    if (c1 == 0) {
        consumidor(msgid, 1, 1);
        _exit(0);
    }
    pid_t c2 = fork();
    if (c2 == 0) {
        consumidor(msgid, 2, 2);
        _exit(0);
    }

    produtor(msgid);

    // Espera ambos finalizarem
    waitpid(c1, NULL, 0);
    waitpid(c2, NULL, 0);

    // Remove a fila
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("IPC_RMID");
        exit(1);
    }
    printf("Fila removida. Execução concluída.\n");
    return 0;
}
