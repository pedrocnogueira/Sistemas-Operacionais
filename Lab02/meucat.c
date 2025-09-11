#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
    fprintf(stderr, "Uso: meucat ...\n");
    return 1;
    }
    
    for (int i = 1; i < argc; i++) {
    FILE *file = fopen(argv[i], "r");
    if (file == NULL) {
    perror(argv[i]); // Imprime erro se não conseguir abrir o arquivo
    continue;
    }
    
    // Imprime o conteúdo do arquivo
    char ch;
    while ((ch = fgetc(file)) != EOF) {
    putchar(ch);
    }
    
    fclose(file); // Fecha o arquivo
    }
    
    return 0;
    }
