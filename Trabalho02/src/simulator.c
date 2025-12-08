/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: simulator.c
 * Descrição: Lógica principal do simulador
 */

#include <ctype.h>
#include "simulator.h"

/*
 * Exibe mensagem de uso do programa
 */
void print_usage(const char *program_name) {
    printf("Uso: %s <algoritmo> <arquivo.log> <tam_pagina><tam_memoria>\n", program_name);
    printf("\n");
    printf("Argumentos:\n");
    printf("  algoritmo    : LRU, NRU ou OTM (ótimo)\n");
    printf("  arquivo.log  : arquivo com sequência de acessos à memória\n");
    printf("  tam_pagina   : tamanho da página em KB (8, 16 ou 32)\n");
    printf("  tam_memoria  : tamanho da memória física em MB (1, 2 ou 4)\n");
    printf("\n");
    printf("Exemplo: %s LRU arquivo.log 82\n", program_name);
    printf("         (página de 8KB, memória de 2MB, algoritmo LRU)\n");
}

/*
 * Parseia os argumentos de linha de comando
 * Formato: sim-virtual <alg> <arquivo> <tam_pag><tam_mem>
 * O último argumento combina tamanho de página e memória (ex: 82 = 8KB e 2MB)
 */
int parse_arguments(int argc, char *argv[], SimConfig *config) {
    if (argc != 4) {
        return 0;
    }
    
    /* Algoritmo */
    char alg_upper[MAX_ALGORITHM];
    strncpy(alg_upper, argv[1], MAX_ALGORITHM - 1);
    alg_upper[MAX_ALGORITHM - 1] = '\0';
    
    /* Converte para maiúsculas */
    for (int i = 0; alg_upper[i]; i++) {
        alg_upper[i] = toupper((unsigned char)alg_upper[i]);
    }
    
    if (strcmp(alg_upper, "LRU") == 0) {
        config->algorithm = ALG_LRU;
        strcpy(config->algorithm_name, "LRU");
    } else if (strcmp(alg_upper, "NRU") == 0) {
        config->algorithm = ALG_NRU;
        strcpy(config->algorithm_name, "NRU");
    } else if (strcmp(alg_upper, "OTM") == 0 || strcmp(alg_upper, "OTIMO") == 0 || 
               strcmp(alg_upper, "ÓTIMO") == 0 || strcmp(alg_upper, "OPTIMAL") == 0) {
        config->algorithm = ALG_OPTIMAL;
        strcpy(config->algorithm_name, "OTM");
    } else {
        fprintf(stderr, "Erro: Algoritmo desconhecido: %s\n", argv[1]);
        return 0;
    }
    
    /* Arquivo de entrada */
    strncpy(config->input_file, argv[2], MAX_FILENAME - 1);
    config->input_file[MAX_FILENAME - 1] = '\0';
    
    /* Tamanho de página e memória (formato: XY onde X = página, Y = memória) */
    char *sizes = argv[3];
    int len = strlen(sizes);
    
    if (len < 2) {
        fprintf(stderr, "Erro: Formato inválido para tamanhos. Use: <tam_pag><tam_mem>\n");
        return 0;
    }
    
    /* Último caractere é o tamanho da memória */
    char mem_char = sizes[len - 1];
    config->memory_size_mb = mem_char - '0';
    
    /* Caracteres anteriores são o tamanho da página */
    char page_str[10];
    strncpy(page_str, sizes, len - 1);
    page_str[len - 1] = '\0';
    config->page_size_kb = atoi(page_str);
    
    return 1;
}

/*
 * Valida os parâmetros do simulador
 */
int validate_config(SimConfig *config) {
    /* Valida tamanho de página */
    if (config->page_size_kb != 8 && config->page_size_kb != 16 && 
        config->page_size_kb != 32) {
        fprintf(stderr, "Erro: Tamanho de página inválido: %d KB\n", config->page_size_kb);
        fprintf(stderr, "      Valores válidos: 8, 16 ou 32 KB\n");
        return 0;
    }
    
    /* Valida tamanho de memória */
    if (config->memory_size_mb != 1 && config->memory_size_mb != 2 && 
        config->memory_size_mb != 4) {
        fprintf(stderr, "Erro: Tamanho de memória inválido: %d MB\n", config->memory_size_mb);
        fprintf(stderr, "      Valores válidos: 1, 2 ou 4 MB\n");
        return 0;
    }
    
    /* Verifica se arquivo existe */
    FILE *file = fopen(config->input_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Erro: Não foi possível abrir arquivo: %s\n", config->input_file);
        return 0;
    }
    fclose(file);
    
    /* Calcula valores derivados */
    config->page_offset_bits = calculate_offset_bits(config->page_size_kb);
    config->num_frames = (config->memory_size_mb * 1024) / config->page_size_kb;
    
    /* Tamanho da tabela de páginas (32 bits - offset bits) */
    config->page_table_size = 1U << (32 - config->page_offset_bits);
    
    return 1;
}

/*
 * Executa a simulação completa
 */
void run_simulation(SimConfig *config, Statistics *stats) {
    /* Inicializa estatísticas */
    stats->page_faults = 0;
    stats->dirty_pages_written = 0;
    stats->total_accesses = 0;
    
    /* Aloca estruturas de dados */
    PageTableEntry *page_table = page_table_init(config->page_table_size);
    Frame *frames = frames_init(config->num_frames);
    
    if (page_table == NULL || frames == NULL) {
        page_table_free(page_table);
        frames_free(frames);
        return;
    }
    
    /* Para o algoritmo Ótimo, pré-carrega o log */
    AccessLog *access_log = NULL;
    if (config->algorithm == ALG_OPTIMAL) {
        access_log = preload_access_log(config->input_file);
        if (access_log == NULL) {
            page_table_free(page_table);
            frames_free(frames);
            return;
        }
    }
    
    /* Abre arquivo de entrada (ou usa log pré-carregado para ótimo) */
    FILE *file = NULL;
    if (config->algorithm != ALG_OPTIMAL) {
        file = fopen(config->input_file, "r");
        if (file == NULL) {
            fprintf(stderr, "Erro: Não foi possível abrir arquivo: %s\n", config->input_file);
            page_table_free(page_table);
            frames_free(frames);
            free_access_log(access_log);
            return;
        }
    }
    
    unsigned time = 0;
    unsigned addr;
    char rw;
    int frames_used = 0;
    unsigned access_index = 0;
    
    /* Loop principal de simulação */
    int continue_loop = 1;
    while (continue_loop) {
        /* Lê acesso do arquivo ou do log pré-carregado */
        if (config->algorithm == ALG_OPTIMAL) {
            if (access_index >= access_log->count) {
                break;
            }
            addr = access_log->addresses[access_index];
            rw = access_log->operations[access_index];
        } else {
            if (fscanf(file, "%x %c", &addr, &rw) != 2) {
                break;
            }
        }
        
        unsigned page = get_page_index(addr, config->page_offset_bits);
        stats->total_accesses++;
        
        /* Verifica se a página está em memória */
        if (page_table[page].valid) {
            /* Hit: atualiza bits R e M */
            update_page_bits(page_table, frames, page, rw, time);
        } else {
            /* Page Fault */
            stats->page_faults++;
            
            int frame;
            
            /* Procura quadro livre */
            if (frames_used < config->num_frames) {
                frame = find_free_frame(frames, config->num_frames);
                frames_used++;
            } else {
                /* Precisa substituir uma página */
                frame = select_victim(frames, config->num_frames, config->algorithm,
                                       page_table, access_log, access_index, 
                                       config->page_offset_bits);
                
                /* Se página vítima foi modificada, conta escrita no disco */
                if (frames[frame].M) {
                    stats->dirty_pages_written++;
                }
                
                /* Invalida a página vítima */
                invalidate_page(page_table, frames, frame);
            }
            
            /* Carrega nova página */
            load_page(page_table, frames, page, frame, time);
            
            /* Se for escrita, marca como modificada */
            if (rw == 'W' || rw == 'w') {
                frames[frame].M = 1;
            }
        }
        
        /* Reset periódico dos bits R para NRU */
        if (config->algorithm == ALG_NRU && time > 0 && time % NRU_RESET_INTERVAL == 0) {
            reset_reference_bits(frames, config->num_frames);
        }
        
        time++;
        access_index++;
    }
    
    /* Limpa recursos */
    if (file != NULL) {
        fclose(file);
    }
    page_table_free(page_table);
    frames_free(frames);
    free_access_log(access_log);
}

/*
 * Imprime o relatório final
 */
void print_report(SimConfig *config, Statistics *stats) {
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", config->input_file);
    printf("Tamanho da memoria fisica: %d MB\n", config->memory_size_mb);
    printf("Tamanho das paginas: %d KB\n", config->page_size_kb);
    printf("Algoritmo de substituicao: %s\n", config->algorithm_name);
    printf("Numero de Faltas de Paginas: %u\n", stats->page_faults);
    printf("Numero de Paginas Escritas: %u\n", stats->dirty_pages_written);
}

