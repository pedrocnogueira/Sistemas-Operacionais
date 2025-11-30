/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: algorithms.h
 * Descrição: Algoritmos de substituição de páginas
 */

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "types.h"

/* 
 * LRU - Least Recently Used
 * Retorna o índice do quadro que contém a página menos recentemente usada
 */
int lru_select_victim(Frame *frames, int num_frames);

/* 
 * NRU - Not Recently Used
 * Retorna o índice do quadro com base nas classes NRU (R, M bits)
 * Classes: 0 (R=0,M=0), 1 (R=0,M=1), 2 (R=1,M=0), 3 (R=1,M=1)
 */
int nru_select_victim(Frame *frames, int num_frames);

/* 
 * Ótimo (Belady)
 * Retorna o índice do quadro cuja página será acessada mais tarde no futuro
 * current_index: posição atual no log de acessos
 */
int optimal_select_victim(Frame *frames, int num_frames, 
                          PageTableEntry *page_table, AccessLog *log, 
                          unsigned current_index, int offset_bits);

/* 
 * Seleciona vítima com base no algoritmo configurado
 */
int select_victim(Frame *frames, int num_frames, Algorithm alg,
                  PageTableEntry *page_table, AccessLog *log,
                  unsigned current_index, int offset_bits);

/* 
 * Pré-carrega o arquivo de log para o algoritmo Ótimo
 */
AccessLog* preload_access_log(const char *filename);

/* 
 * Libera a memória do log de acessos
 */
void free_access_log(AccessLog *log);

#endif /* ALGORITHMS_H */

