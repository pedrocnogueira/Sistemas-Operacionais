/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: algorithms.c
 * Descrição: Implementação dos algoritmos de substituição de páginas
 */

#include "algorithms.h"
#include "page_table.h"

/*
 * LRU - Least Recently Used
 * Seleciona o quadro com a página menos recentemente acessada
 */
int lru_select_victim(Frame *frames, int num_frames) {
    int victim = -1;
    unsigned oldest_access = (unsigned)-1;  /* Máximo valor unsigned */
    
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].occupied) {
            if (frames[i].last_access < oldest_access) {
                oldest_access = frames[i].last_access;
                victim = i;
            }
        }
    }
    
    return victim;
}

/*
 * NRU - Not Recently Used
 * Classifica páginas em 4 classes baseado nos bits R e M:
 *   Classe 0: R=0, M=0 (não referenciada, não modificada)
 *   Classe 1: R=0, M=1 (não referenciada, modificada)
 *   Classe 2: R=1, M=0 (referenciada, não modificada)
 *   Classe 3: R=1, M=1 (referenciada, modificada)
 * Seleciona uma página da classe mais baixa não vazia
 */
int nru_select_victim(Frame *frames, int num_frames) {
    int class_victims[4] = {-1, -1, -1, -1};
    
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].occupied) {
            int nru_class = (frames[i].R << 1) | frames[i].M;
            if (class_victims[nru_class] == -1) {
                class_victims[nru_class] = i;
            }
        }
    }
    
    /* Retorna a primeira classe não vazia (menor prioridade = melhor vítima) */
    for (int c = 0; c < 4; c++) {
        if (class_victims[c] != -1) {
            return class_victims[c];
        }
    }
    
    return -1;
}

/*
 * Algoritmo Ótimo (Belady)
 * Substitui a página que será acessada mais tarde no futuro
 */
int optimal_select_victim(Frame *frames, int num_frames, 
                          PageTableEntry *page_table __attribute__((unused)), 
                          AccessLog *log, 
                          unsigned current_index, int offset_bits) {
    
    /* Para cada quadro ocupado, encontra quando sua página será usada novamente */
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].occupied) {
            frames[i].next_use = -1;  /* -1 significa que não será usada */
            
            /* Procura no futuro quando esta página será acessada */
            for (unsigned j = current_index + 1; j < log->count; j++) {
                unsigned future_page = get_page_index(log->addresses[j], offset_bits);
                if (future_page == frames[i].page_number) {
                    frames[i].next_use = (int)j;
                    break;
                }
            }
        }
    }
    
    /* Seleciona o quadro cuja página será usada mais tarde (ou nunca) */
    int victim = -1;
    int farthest_use = -2;  /* -2 é menor que -1, então qualquer valor ganha inicialmente */
    
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].occupied) {
            if (frames[i].next_use == -1) {
                /* Página nunca mais será usada - melhor candidata */
                return i;
            }
            if (frames[i].next_use > farthest_use) {
                farthest_use = frames[i].next_use;
                victim = i;
            }
        }
    }
    
    return victim;
}

/*
 * Seleciona vítima com base no algoritmo configurado
 */
int select_victim(Frame *frames, int num_frames, Algorithm alg,
                  PageTableEntry *page_table, AccessLog *log,
                  unsigned current_index, int offset_bits) {
    switch (alg) {
        case ALG_LRU:
            return lru_select_victim(frames, num_frames);
        case ALG_NRU:
            return nru_select_victim(frames, num_frames);
        case ALG_OPTIMAL:
            return optimal_select_victim(frames, num_frames, page_table, 
                                         log, current_index, offset_bits);
        default:
            return lru_select_victim(frames, num_frames);
    }
}

/*
 * Conta o número de linhas em um arquivo
 */
static unsigned count_lines(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }
    
    unsigned count = 0;
    unsigned addr;
    char rw;
    
    while (fscanf(file, "%x %c", &addr, &rw) == 2) {
        count++;
    }
    
    fclose(file);
    return count;
}

/*
 * Pré-carrega o arquivo de log para o algoritmo Ótimo
 */
AccessLog* preload_access_log(const char *filename) {
    unsigned count = count_lines(filename);
    if (count == 0) {
        fprintf(stderr, "Erro: Arquivo vazio ou não encontrado: %s\n", filename);
        return NULL;
    }
    
    AccessLog *log = (AccessLog*)malloc(sizeof(AccessLog));
    if (log == NULL) {
        fprintf(stderr, "Erro: Não foi possível alocar log de acessos\n");
        return NULL;
    }
    
    log->addresses = (unsigned*)malloc(count * sizeof(unsigned));
    log->operations = (char*)malloc(count * sizeof(char));
    log->count = count;
    
    if (log->addresses == NULL || log->operations == NULL) {
        fprintf(stderr, "Erro: Não foi possível alocar arrays do log\n");
        free(log->addresses);
        free(log->operations);
        free(log);
        return NULL;
    }
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erro: Não foi possível abrir arquivo: %s\n", filename);
        free(log->addresses);
        free(log->operations);
        free(log);
        return NULL;
    }
    
    for (unsigned i = 0; i < count; i++) {
        if (fscanf(file, "%x %c", &log->addresses[i], &log->operations[i]) != 2) {
            break;
        }
    }
    
    fclose(file);
    return log;
}

/*
 * Libera a memória do log de acessos
 */
void free_access_log(AccessLog *log) {
    if (log != NULL) {
        free(log->addresses);
        free(log->operations);
        free(log);
    }
}

