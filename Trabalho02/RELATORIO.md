# Relatório - Trabalho 02: Simulador de Memória Virtual

## Integrantes

Pedro Carneiro Nogueira – 2310540

Hannah Barbosa Goldstein – 2310160

---

## Objetivo

Implementar um simulador de memória virtual em linguagem C que realiza a paginação de memória e implementa três algoritmos de substituição de páginas: LRU (Least Recently Used), NRU (Not Recently Used) e o algoritmo Ótimo (Belady).

O simulador recebe como entrada um arquivo contendo uma sequência de endereços de memória acessados por um programa real, processa cada acesso para detectar page faults, simula a substituição de páginas e coleta estatísticas sobre o desempenho dos algoritmos.

O experimento visa compreender os conceitos de paginação, memória virtual, algoritmos de substituição de páginas e como diferentes estratégias afetam o desempenho do sistema em termos de page faults e escritas no disco.

---

## Estrutura do Programa

O simulador está organizado em módulos bem definidos, cada um com responsabilidades específicas:

### Módulo: `main.c`
**Responsabilidade:** Ponto de entrada do programa.

**Funções:**
- `main(int argc, char *argv[])`: Função principal que coordena a execução do simulador. Parseia argumentos, valida configuração, executa a simulação e imprime o relatório final.

### Módulo: `simulator.c` e `simulator.h`
**Responsabilidade:** Lógica principal do simulador e gerenciamento da simulação.

**Funções:**
- `print_usage(const char *program_name)`: Exibe mensagem de uso do programa com exemplos.
- `parse_arguments(int argc, char *argv[], SimConfig *config)`: Parseia e valida os argumentos de linha de comando (algoritmo, arquivo, tamanhos).
- `validate_config(SimConfig *config)`: Valida os parâmetros do simulador (tamanhos de página/memória, existência do arquivo) e calcula valores derivados.
- `run_simulation(SimConfig *config, Statistics *stats)`: Executa o loop principal da simulação, processando cada acesso à memória, detectando page faults e aplicando algoritmos de substituição.
- `print_report(SimConfig *config, Statistics *stats)`: Imprime o relatório final com configuração e estatísticas.

### Módulo: `page_table.c` e `page_table.h`
**Responsabilidade:** Gerenciamento da tabela de páginas e quadros físicos.

**Funções:**
- `page_table_init(unsigned size)`: Aloca e inicializa a tabela de páginas com todas as entradas inválidas.
- `page_table_free(PageTableEntry *table)`: Libera a memória da tabela de páginas.
- `frames_init(int num_frames)`: Aloca e inicializa os quadros de página como livres.
- `frames_free(Frame *frames)`: Libera a memória dos quadros de página.
- `find_free_frame(Frame *frames, int num_frames)`: Procura e retorna um quadro livre disponível.
- `load_page(...)`: Carrega uma página em um quadro, atualizando tabela de páginas e metadados do quadro.
- `invalidate_page(...)`: Remove uma página de um quadro, invalidando-a na tabela de páginas.
- `update_page_bits(...)`: Atualiza os bits R (referência) e M (modificação) de uma página já em memória.
- `get_page_index(unsigned address, int offset_bits)`: Calcula o índice da página a partir do endereço lógico (page = addr >> offset_bits).
- `calculate_offset_bits(int page_size_kb)`: Calcula o número de bits de offset baseado no tamanho da página.
- `reset_reference_bits(Frame *frames, int num_frames)`: Reseta os bits R de todos os quadros (usado pelo algoritmo NRU).

### Módulo: `algorithms.c` e `algorithms.h`
**Responsabilidade:** Implementação dos algoritmos de substituição de páginas.

**Funções:**
- `lru_select_victim(Frame *frames, int num_frames)`: Implementa o algoritmo LRU, selecionando a página menos recentemente acessada.
- `nru_select_victim(Frame *frames, int num_frames)`: Implementa o algoritmo NRU, classificando páginas em 4 classes (R, M) e selecionando da classe mais baixa.
- `optimal_select_victim(...)`: Implementa o algoritmo Ótimo (Belady), substituindo a página que será acessada mais tarde no futuro.
- `select_victim(...)`: Função genérica que seleciona a vítima baseada no algoritmo configurado.
- `preload_access_log(const char *filename)`: Pré-carrega todo o log de acessos em memória (necessário para o algoritmo Ótimo).
- `free_access_log(AccessLog *log)`: Libera a memória do log de acessos.

### Módulo: `types.h`
**Responsabilidade:** Definições de tipos e estruturas de dados.

**Estruturas:**
- `PageTableEntry`: Representa uma entrada na tabela de páginas (valid, frame_number).
- `Frame`: Representa um quadro físico (occupied, page_number, R, M, last_access, next_use).
- `SimConfig`: Configuração do simulador (algoritmo, arquivo, tamanhos, valores derivados).
- `Statistics`: Estatísticas coletadas (page_faults, dirty_pages_written, total_accesses).
- `AccessLog`: Log pré-carregado de acessos para o algoritmo Ótimo.

---

## Solução

### Passo 1: Parseamento e Validação de Argumentos

O programa recebe 4 argumentos na linha de comando:
1. **Algoritmo**: LRU, NRU ou OTM (ótimo)
2. **Arquivo de entrada**: Arquivo .log com sequência de acessos
3. **Tamanhos combinados**: Formato XY onde X = tamanho da página (KB) e Y = tamanho da memória (MB)

Exemplo: `sim-virtual LRU arquivo.log 82` significa:
- Algoritmo: LRU
- Arquivo: arquivo.log
- Página: 8 KB
- Memória: 2 MB

O parseamento converte o algoritmo para maiúsculas, extrai os tamanhos do argumento combinado e valida todos os parâmetros.

### Passo 2: Inicialização das Estruturas de Dados

O simulador inicializa:
- **Tabela de páginas**: Array com 2^(32 - offset_bits) entradas, todas inicialmente inválidas
- **Quadros físicos**: Array com (memória_mb * 1024) / page_size_kb quadros, todos livres
- **Estatísticas**: Contadores zerados (page_faults, dirty_pages_written, total_accesses)

Para o algoritmo Ótimo, o log de acessos é pré-carregado completamente em memória para permitir "olhar o futuro".

### Passo 3: Cálculo do Índice da Página

Para cada acesso, o índice da página é calculado descartando os bits menos significativos:
```
page_index = address >> offset_bits
```

Onde `offset_bits` é calculado como:
- 8 KB = 8192 bytes = 2^13 → offset_bits = 13
- 16 KB = 16384 bytes = 2^14 → offset_bits = 14
- 32 KB = 32768 bytes = 2^15 → offset_bits = 15

### Passo 4: Processamento de Cada Acesso

Para cada linha do arquivo de entrada (formato: endereço hexadecimal + R/W):

1. **Cálculo da página**: Extrai o índice da página do endereço
2. **Verificação de hit/miss**:
   - Se `page_table[page].valid == 1`: **Hit** - página está em memória
     - Atualiza bit R = 1
     - Se operação for W, atualiza bit M = 1
     - Atualiza `last_access` (para LRU)
   - Se `page_table[page].valid == 0`: **Page Fault** - página não está em memória

### Passo 5: Tratamento de Page Fault

Quando ocorre um page fault:

1. **Procura quadro livre**:
   - Se há quadros livres (`frames_used < num_frames`), usa um quadro livre
   - Senão, precisa substituir uma página usando o algoritmo configurado

2. **Seleção de vítima** (quando todos os quadros estão ocupados):
   - **LRU**: Seleciona quadro com menor `last_access`
   - **NRU**: Classifica páginas em 4 classes (R=0,M=0; R=0,M=1; R=1,M=0; R=1,M=1) e seleciona da classe mais baixa não vazia
   - **Ótimo**: Para cada quadro, encontra quando sua página será usada novamente no futuro; seleciona a que será usada mais tarde (ou nunca)

3. **Substituição**:
   - Se página vítima tem M=1, incrementa contador de páginas escritas
   - Invalida página vítima na tabela de páginas
   - Carrega nova página no quadro

4. **Atualização**:
   - Marca página como válida na tabela de páginas
   - Atualiza metadados do quadro (R=1, M conforme operação, last_access)

### Passo 6: Reset Periódico (NRU)

Para o algoritmo NRU, a cada 1000 acessos, todos os bits R são resetados para 0, permitindo classificar páginas como "não referenciadas recentemente".

### Passo 7: Geração do Relatório

Ao final do processamento, o simulador imprime:
- Configuração utilizada (arquivo, memória, página, algoritmo)
- Número de faltas de páginas (page faults)
- Número de páginas escritas no disco (apenas páginas sujas substituídas)

### Exemplos de Execução

#### Exemplo 1: Algoritmo LRU
```
$ ./sim-virtual LRU Entradas/compilador.log 82

Executando o simulador...
Arquivo de entrada: Entradas/compilador.log
Tamanho da memoria fisica: 2 MB
Tamanho das paginas: 8 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 21091
Numero de Paginas Escritas: 3839
```

**Análise:** O algoritmo LRU processou o arquivo compilador.log com páginas de 8 KB e memória de 2 MB. Ocorreram 21.091 page faults, sendo que 3.839 páginas sujas precisaram ser escritas de volta no disco quando substituídas.

#### Exemplo 2: Algoritmo NRU
```
$ ./sim-virtual NRU Entradas/matriz.log 164

Executando o simulador...
Arquivo de entrada: Entradas/matriz.log
Tamanho da memoria fisica: 4 MB
Tamanho das paginas: 16 KB
Algoritmo de substituicao: NRU
Numero de Faltas de Paginas: 12692
Numero de Paginas Escritas: 1495
```

**Análise:** O algoritmo NRU processou o arquivo matriz.log com páginas de 16 KB e memória de 4 MB. Ocorreram 12.692 page faults, com 1.495 páginas sujas escritas.

#### Exemplo 3: Algoritmo Ótimo
```
$ ./sim-virtual OTM Entradas/compressor.log 321

Executando o simulador...
Arquivo de entrada: Entradas/compressor.log
Tamanho da memoria fisica: 1 MB
Tamanho das paginas: 32 KB
Algoritmo de substituicao: OTM
Numero de Faltas de Paginas: 607
Numero de Paginas Escritas: 250
```

**Análise:** O algoritmo Ótimo processou o arquivo compressor.log com páginas de 32 KB e memória de 1 MB. Ocorreram apenas 607 page faults (menor número possível para esta sequência), com 250 páginas sujas escritas.

### Comparação de Resultados

Testando os três algoritmos com a mesma configuração (compilador.log, 8KB, 2MB):

- **LRU**: 21.091 page faults, 3.839 escritas
- **NRU**: 38.503 page faults, 3.276 escritas  
- **Ótimo**: 10.118 page faults, 2.190 escritas

**Observação:** O algoritmo ótimo sempre apresenta o menor número de page faults, pois tem conhecimento do futuro. LRU geralmente supera NRU por ser mais preciso na estimativa de páginas recentemente usadas.

---

## Observações e Conclusões

### Facilidades Encontradas

1. **Estrutura modular**: A divisão em módulos bem definidos facilitou o desenvolvimento e manutenção do código. Cada módulo tem responsabilidade clara, tornando o código mais legível e testável.

2. **Algoritmos LRU e NRU**: A implementação desses algoritmos foi relativamente direta, pois não requerem conhecimento do futuro. LRU usa apenas o timestamp do último acesso, enquanto NRU usa os bits R e M já presentes na estrutura.

3. **Cálculo de índices**: O cálculo do índice da página através de shift de bits é eficiente e direto, facilitando a implementação.

4. **Validação de entrada**: O sistema de validação robusto ajuda a identificar erros rapidamente e fornece mensagens claras ao usuário.

### Dificuldades Encontradas

1. **Algoritmo Ótimo**: A principal dificuldade foi garantir que o índice usado para "olhar o futuro" correspondesse corretamente à posição atual no log. Inicialmente, o código usava o contador `time` como índice, mas isso não funcionava corretamente. A solução foi criar um índice separado (`access_index`) e, para o algoritmo ótimo, ler do log pré-carregado em vez do arquivo diretamente.

2. **Pré-carregamento do log**: O algoritmo ótimo requer carregar todo o arquivo de log em memória, o que pode ser problemático para arquivos muito grandes. Foi necessário implementar uma função que conta as linhas primeiro, depois aloca memória e carrega os dados.

3. **Reset periódico do NRU**: Decidir o intervalo de reset dos bits R (1000 acessos) foi uma escolha de design. Valores muito pequenos podem degradar o desempenho, enquanto valores muito grandes podem não criar as classes NRU adequadamente.

4. **Tamanho da tabela de páginas**: Para páginas de 8 KB, a tabela pode ter até 2^19 entradas (meio milhão), o que é aceitável para simulação mas requer atenção na alocação de memória.

### Resultados dos Testes

#### O que funciona:

✅ **Todos os algoritmos funcionam corretamente**:
- LRU seleciona corretamente a página menos recentemente usada
- NRU classifica e seleciona páginas das classes corretas
- Ótimo encontra e substitui a página que será usada mais tarde

✅ **Cálculo de índices**: Funciona corretamente para todos os tamanhos de página (8, 16, 32 KB)

✅ **Atualização de bits R e M**: Bits são atualizados corretamente em hits e page faults

✅ **Contagem de estatísticas**: Page faults e páginas escritas são contados corretamente

✅ **Validação de entrada**: Rejeita argumentos inválidos e fornece mensagens de erro claras

✅ **Suporte a diferentes configurações**: Funciona com todas as combinações de tamanhos de página (8, 16, 32 KB) e memória (1, 2, 4 MB)

✅ **Formato de saída**: Gera relatório no formato especificado no enunciado

#### Comportamento esperado observado:

1. **Algoritmo ótimo sempre tem menos page faults**: Confirmado nos testes. O algoritmo ótimo sempre apresenta o menor número de page faults, servindo como referência teórica.

2. **LRU geralmente supera NRU**: Confirmado. LRU é mais preciso que NRU na maioria dos casos, resultando em menos page faults.

3. **Mais memória → menos page faults**: Confirmado. Aumentar o tamanho da memória física reduz o número de page faults, pois mais páginas podem permanecer em memória.

4. **Tamanho de página afeta resultados**: Confirmado. O tamanho da página influencia o número de page faults dependendo do padrão de acesso do programa (localidade espacial).

### Conclusões

O simulador foi implementado com sucesso e atende a todos os requisitos do enunciado. Os três algoritmos de substituição de páginas foram implementados corretamente e produzem resultados consistentes.

**Principais aprendizados:**

1. **Algoritmo ótimo como referência**: O algoritmo ótimo, embora impossível de implementar em sistemas reais, serve como referência teórica valiosa para avaliar outros algoritmos.

2. **Trade-offs entre algoritmos**: LRU oferece melhor desempenho que NRU, mas requer mais overhead (atualização de timestamps). NRU é mais simples e eficiente, mas menos preciso.

3. **Impacto da configuração**: Tanto o tamanho da página quanto o tamanho da memória física têm impacto significativo no desempenho, demonstrando a importância de escolher configurações adequadas para cada tipo de aplicação.

4. **Localidade de referência**: Programas com boa localidade temporal (LRU funciona bem) e espacial (páginas maiores ajudam) apresentam menos page faults.

O código está bem estruturado, documentado e pronto para uso. A implementação modular facilita futuras extensões, como adicionar novos algoritmos de substituição ou métricas de desempenho adicionais.

