/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: simulator.h
 * Descrição: Lógica principal do simulador
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h"
#include "page_table.h"
#include "algorithms.h"

/* Intervalo para resetar bits R no NRU (em número de acessos) */
#define NRU_RESET_INTERVAL 1000

/* Executa a simulação completa */
void run_simulation(SimConfig *config, Statistics *stats);

/* Imprime o relatório final */
void print_report(SimConfig *config, Statistics *stats);

/* Parseia os argumentos de linha de comando */
int parse_arguments(int argc, char *argv[], SimConfig *config);

/* Exibe mensagem de uso do programa */
void print_usage(const char *program_name);

/* Valida os parâmetros do simulador */
int validate_config(SimConfig *config);

#endif /* SIMULATOR_H */

