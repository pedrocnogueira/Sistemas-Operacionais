/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: main.c
 * Descrição: Ponto de entrada do programa
 * 
 * Uso: sim-virtual <algoritmo> <arquivo.log> <tam_pagina><tam_memoria>
 * Exemplo: sim-virtual LRU arquivo.log 82
 *          (página de 8KB, memória de 2MB, algoritmo LRU)
 */

#include <stdio.h>
#include <stdlib.h>
#include "simulator.h"

int main(int argc, char *argv[]) {
    SimConfig config;
    Statistics stats;
    
    /* Parseia argumentos de linha de comando */
    if (!parse_arguments(argc, argv, &config)) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* Valida configuração */
    if (!validate_config(&config)) {
        return 1;
    }
    
    /* Executa simulação */
    run_simulation(&config, &stats);
    
    /* Imprime relatório */
    print_report(&config, &stats);
    
    return 0;
}

