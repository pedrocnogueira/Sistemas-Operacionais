/*
 * Simulador de Memória Virtual - sim-virtual
 * INF1316 - Sistemas Operacionais
 * 
 * Arquivo: page_table.h
 * Descrição: Gerenciamento da tabela de páginas e quadros
 */

#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "types.h"

/* Inicializa a tabela de páginas */
PageTableEntry* page_table_init(unsigned size);

/* Libera a tabela de páginas */
void page_table_free(PageTableEntry *table);

/* Inicializa os quadros de página */
Frame* frames_init(int num_frames);

/* Libera os quadros de página */
void frames_free(Frame *frames);

/* Verifica se há quadro livre disponível */
int find_free_frame(Frame *frames, int num_frames);

/* Carrega uma página em um quadro */
void load_page(PageTableEntry *page_table, Frame *frames, 
               unsigned page_number, int frame_number, unsigned time);

/* Invalida uma página (remove do quadro) */
void invalidate_page(PageTableEntry *page_table, Frame *frames, int frame_number);

/* Atualiza bits R e M de uma página */
void update_page_bits(PageTableEntry *page_table, Frame *frames, 
                      unsigned page_number, char operation, unsigned time);

/* Obtém o índice da página a partir do endereço */
unsigned get_page_index(unsigned address, int offset_bits);

/* Calcula os bits de offset a partir do tamanho da página em KB */
int calculate_offset_bits(int page_size_kb);

/* Reseta os bits R de todos os quadros (para NRU) */
void reset_reference_bits(Frame *frames, int num_frames);

#endif /* PAGE_TABLE_H */

