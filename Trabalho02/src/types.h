/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: types.h
 * Descrição: Definições de tipos e estruturas de dados
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constantes */
#define MAX_FILENAME 256
#define MAX_ALGORITHM 10

/* Algoritmos suportados */
typedef enum {
    ALG_LRU,
    ALG_NRU,
    ALG_OPTIMAL
} Algorithm;

/* Entrada da Tabela de Páginas */
typedef struct {
    int valid;              /* Página está em memória? */
    int frame_number;       /* Número do quadro físico (-1 se não mapeado) */
} PageTableEntry;

/* Quadro de Página (Frame) */
typedef struct {
    int occupied;           /* Quadro está ocupado? */
    unsigned page_number;   /* Número da página virtual */
    int R;                  /* Bit de referência */
    int M;                  /* Bit de modificação (dirty) */
    unsigned last_access;   /* Instante do último acesso (para LRU) */
    int next_use;           /* Próximo uso futuro (para Ótimo) */
} Frame;

/* Configuração do Simulador */
typedef struct {
    Algorithm algorithm;        /* Algoritmo de substituição */
    char algorithm_name[MAX_ALGORITHM]; /* Nome do algoritmo */
    char input_file[MAX_FILENAME];      /* Arquivo .log */
    int page_size_kb;           /* 8, 16 ou 32 KB */
    int memory_size_mb;         /* 1, 2 ou 4 MB */
    int num_frames;             /* Calculado: memory_size / page_size */
    int page_offset_bits;       /* s = log2(page_size) */
    unsigned page_table_size;   /* Número de entradas na tabela de páginas */
} SimConfig;

/* Estatísticas da Simulação */
typedef struct {
    unsigned page_faults;
    unsigned dirty_pages_written;
    unsigned total_accesses;
} Statistics;

/* Estrutura para armazenar acessos futuros (algoritmo Ótimo) */
typedef struct {
    unsigned *addresses;    /* Array de endereços */
    char *operations;       /* Array de operações (R/W) */
    unsigned count;         /* Número total de acessos */
} AccessLog;

#endif /* TYPES_H */

