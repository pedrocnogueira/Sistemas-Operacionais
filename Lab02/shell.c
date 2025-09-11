#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define TRUE 1
#define MAX_INPUT_SIZE 200
#define MAX_PARAMETERS 20

void type_prompt() {
    printf("shell> ");
}

int main(void) {
    char input[MAX_INPUT_SIZE]; // Buffer para entrada do usuário
    char* parameters[MAX_PARAMETERS]; // Array de ponteiros para parâmetros
    int status;

    while (TRUE) { // Loop infinito
    type_prompt(); // Exibe o prompt

    // Lê a linha de entrada do usuário
    if (fgets(input, sizeof(input), stdin) == NULL) {
    perror("Erro ao ler a entrada");
    continue;
    }

    // Remove o caractere de nova linha no final da entrada (caso exista)
    input[strcspn(input, "\n")] = '\0';

    // Divide a entrada em comando e parâmetros
    int i = 0;
    parameters[i] = strtok(input, " ");
    while (parameters[i] != NULL && i < MAX_PARAMETERS - 1) {
    i++;
    parameters[i] = strtok(NULL, " ");
    }
    parameters[i] = NULL; // Termina a lista de parâmetros com NULL

    if (parameters[0] == NULL) {
    continue; // Se não houver comando, continua o loop
    }

    // Cria um processo filho
    if (fork() != 0) {
    // Código do pai: espera o filho terminar
    waitpid(-1, &status, 0);
    } else {
        // Código do filho: executa o comando
        if (strcmp(parameters[0], "meuecho") == 0) {
            execve("./meuecho", parameters, NULL);
        } else if (strcmp(parameters[0], "meucat") == 0) {
            execve("./meucat", parameters, NULL);
        } else if (strcmp(parameters[0], "ls") == 0) {
            // Executa o comando "ls" do sistema
            execvp("ls", parameters);
        } else {
            // Comando não reconhecido
            printf("Comando desconhecido: %s\n", parameters[0]);
            exit(1);
        }
    }
  }
 return 0;
}
