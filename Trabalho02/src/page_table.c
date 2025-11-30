/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: page_table.c
 * Descrição: Implementação do gerenciamento da tabela de páginas e quadros
 */

#include "page_table.h"

/*
 * Inicializa a tabela de páginas com todas as entradas inválidas
 */
PageTableEntry* page_table_init(unsigned size) {
    PageTableEntry *table = (PageTableEntry*)malloc(size * sizeof(PageTableEntry));
    if (table == NULL) {
        fprintf(stderr, "Erro: Não foi possível alocar tabela de páginas\n");
        return NULL;
    }
    
    for (unsigned i = 0; i < size; i++) {
        table[i].valid = 0;
        table[i].frame_number = -1;
    }
    
    return table;
}

/*
 * Libera a memória da tabela de páginas
 */
void page_table_free(PageTableEntry *table) {
    if (table != NULL) {
        free(table);
    }
}

/*
 * Inicializa os quadros de página como livres
 */
Frame* frames_init(int num_frames) {
    Frame *frames = (Frame*)malloc(num_frames * sizeof(Frame));
    if (frames == NULL) {
        fprintf(stderr, "Erro: Não foi possível alocar quadros de página\n");
        return NULL;
    }
    
    for (int i = 0; i < num_frames; i++) {
        frames[i].occupied = 0;
        frames[i].page_number = 0;
        frames[i].R = 0;
        frames[i].M = 0;
        frames[i].last_access = 0;
        frames[i].next_use = -1;
    }
    
    return frames;
}

/*
 * Libera a memória dos quadros de página
 */
void frames_free(Frame *frames) {
    if (frames != NULL) {
        free(frames);
    }
}

/*
 * Procura um quadro livre
 * Retorna o índice do quadro livre ou -1 se não houver
 */
int find_free_frame(Frame *frames, int num_frames) {
    for (int i = 0; i < num_frames; i++) {
        if (!frames[i].occupied) {
            return i;
        }
    }
    return -1;
}

/*
 * Carrega uma página em um quadro
 */
void load_page(PageTableEntry *page_table, Frame *frames, 
               unsigned page_number, int frame_number, unsigned time) {
    /* Atualiza a tabela de páginas */
    page_table[page_number].valid = 1;
    page_table[page_number].frame_number = frame_number;
    
    /* Atualiza o quadro */
    frames[frame_number].occupied = 1;
    frames[frame_number].page_number = page_number;
    frames[frame_number].R = 1;
    frames[frame_number].M = 0;
    frames[frame_number].last_access = time;
    frames[frame_number].next_use = -1;
}

/*
 * Invalida uma página (remove do quadro)
 */
void invalidate_page(PageTableEntry *page_table, Frame *frames, int frame_number) {
    unsigned page_number = frames[frame_number].page_number;
    
    /* Invalida na tabela de páginas */
    page_table[page_number].valid = 0;
    page_table[page_number].frame_number = -1;
    
    /* Limpa o quadro */
    frames[frame_number].occupied = 0;
    frames[frame_number].R = 0;
    frames[frame_number].M = 0;
}

/*
 * Atualiza os bits R e M de uma página já em memória
 */
void update_page_bits(PageTableEntry *page_table, Frame *frames, 
                      unsigned page_number, char operation, unsigned time) {
    int frame = page_table[page_number].frame_number;
    
    if (frame >= 0) {
        frames[frame].R = 1;
        frames[frame].last_access = time;
        
        if (operation == 'W' || operation == 'w') {
            frames[frame].M = 1;
        }
    }
}

/*
 * Obtém o índice da página a partir do endereço
 * page = addr >> s
 */
unsigned get_page_index(unsigned address, int offset_bits) {
    return address >> offset_bits;
}

/*
 * Calcula os bits de offset a partir do tamanho da página em KB
 * 8 KB = 8192 bytes = 2^13, então s = 13
 * 16 KB = 16384 bytes = 2^14, então s = 14
 * 32 KB = 32768 bytes = 2^15, então s = 15
 */
int calculate_offset_bits(int page_size_kb) {
    int bytes = page_size_kb * 1024;
    int s = 0;
    
    while ((1 << s) < bytes) {
        s++;
    }
    
    return s;
}

/*
 * Reseta os bits R de todos os quadros (usado pelo NRU)
 */
void reset_reference_bits(Frame *frames, int num_frames) {
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].occupied) {
            frames[i].R = 0;
        }
    }
}

