#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MSG_KEY 5678
#define TOTAL_MESSAGES 32

struct message {
    long mtype;
    int num;
};

void produtor(int msgid) {
    struct message msg, ack;
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        msg.mtype = 1; // tipo 1: mensagem de dados
        msg.num = i;
        if (msgsnd(msgid, &msg, sizeof(int), 0) == -1) {
            perror("Erro ao enviar mensagem");
            exit(1);
        }
        printf("[PRODUTOR] Enviou %d\n", msg.num);

        // Aguarda confirmação do consumidor (mtype = 2)
        if (msgrcv(msgid, &ack, sizeof(int), 2, 0) == -1) {
            perror("Erro ao receber ACK");
            exit(1);
        }
        printf("[PRODUTOR] Recebeu ACK do consumidor para %d\n", ack.num);
        sleep(1);
    }
}

void consumidor(int msgid) {
    struct message msg, ack;
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        if (msgrcv(msgid, &msg, sizeof(int), 1, 0) == -1) {
            perror("Erro ao receber mensagem");
            exit(1);
        }
        printf("[CONSUMIDOR] Recebeu %d\n", msg.num);

        // Envia confirmação (ACK)
        ack.mtype = 2;
        ack.num = msg.num;
        if (msgsnd(msgid, &ack, sizeof(int), 0) == -1) {
            perror("Erro ao enviar ACK");
            exit(1);
        }
        printf("[CONSUMIDOR] Enviou ACK para %d\n", ack.num);
        sleep(1);
    }
}

int main() {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Erro ao criar fila");
        exit(1);
    }

    pid_t pid = fork();

    if (pid == 0) {
        consumidor(msgid);
        exit(0);
    } else {
        produtor(msgid);
        wait(NULL);
        msgctl(msgid, IPC_RMID, NULL);
        printf("Fila removida. Execução concluída.\n");
    }

    return 0;
}
