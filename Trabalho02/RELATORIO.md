# Relatório - Trabalho 02: Simulador de Memória Virtual

## Integrantes

Pedro Carneiro Nogueira – 2310540

Hannah Barbosa Goldstein – 2310160

## 1. Objetivo

O objetivo deste trabalho é implementar um **simulador de memória virtual** em linguagem C, denominado `sim-virtual`. O simulador cria as estruturas de dados e mecanismos de mapeamento de memória (lógico → físico) necessários para realizar a paginação, implementando três algoritmos de substituição de páginas:

- **LRU** (Least Recently Used)
- **NRU** (Not Recently Used)
- **Ótimo** (algoritmo de Bélády)

O simulador processa uma sequência de acessos à memória (endereços hexadecimais com operações de leitura/escrita), detecta page faults, simula o processo de substituição de páginas e coleta estatísticas para gerar um relatório ao final da execução.

**Formato de execução:**
```
sim-virtual <algoritmo> <arquivo.log> <tam_pagina> <tam_memoria>
```

**Exemplo:**
```
sim-virtual LRU compilador.log 8 2
```
Executa o simulador com algoritmo LRU, arquivo compilador.log, páginas de 8KB e memória física de 2MB.

---

## 2. Estrutura do Programa

O programa está organizado em módulos, cada um responsável por uma funcionalidade específica:

### 2.1 Arquivos de Cabeçalho (.h)

#### types.h
Define todas as estruturas de dados utilizadas no simulador:

- **`Algorithm`**: Enumeração dos algoritmos suportados (ALG_LRU, ALG_NRU, ALG_OPTIMAL)
- **`PageTableEntry`**: Entrada da tabela de páginas com campos `valid` e `frame_number`
- **`Frame`**: Quadro de página com campos `occupied`, `page_number`, `R` (bit referência), `M` (bit modificação), `last_access` e `next_use`
- **`SimConfig`**: Configuração do simulador (algoritmo, arquivo, tamanhos, número de quadros)
- **`Statistics`**: Estatísticas da simulação (page faults, páginas escritas, total de acessos)
- **`AccessLog`**: Estrutura para pré-carregar acessos (usado pelo algoritmo Ótimo)

#### page_table.h
Declara funções para gerenciamento da tabela de páginas e quadros:

- `page_table_init()` / `page_table_free()`: Alocação e liberação da tabela
- `frames_init()` / `frames_free()`: Alocação e liberação dos quadros
- `find_free_frame()`: Busca quadro livre
- `load_page()`: Carrega página em um quadro
- `invalidate_page()`: Remove página de um quadro
- `update_page_bits()`: Atualiza bits R e M
- `get_page_index()`: Calcula índice da página a partir do endereço
- `calculate_offset_bits()`: Calcula bits de offset (s) para o tamanho de página
- `reset_reference_bits()`: Reseta bits R (usado pelo NRU)

#### algorithms.h
Declara funções dos algoritmos de substituição:

- `lru_select_victim()`: Seleção de vítima pelo LRU
- `nru_select_victim()`: Seleção de vítima pelo NRU
- `optimal_select_victim()`: Seleção de vítima pelo algoritmo Ótimo
- `select_victim()`: Função genérica que chama o algoritmo apropriado
- `preload_access_log()` / `free_access_log()`: Carregamento do log para o Ótimo

#### simulator.h
Declara funções da lógica principal:

- `run_simulation()`: Executa a simulação completa
- `print_report()`: Imprime o relatório final
- `parse_arguments()`: Processa argumentos de linha de comando
- `print_usage()`: Exibe mensagem de uso
- `validate_config()`: Valida parâmetros de configuração

### 2.2 Arquivos de Implementação (.c)

#### main.c
Ponto de entrada do programa. Realiza o parsing dos argumentos, valida a configuração, executa a simulação e imprime o relatório.

#### page_table.c
Implementa o gerenciamento da tabela de páginas e quadros físicos, incluindo:
- Alocação dinâmica das estruturas
- Mapeamento página → quadro
- Cálculo do índice de página: `page = addr >> s`

#### algorithms.c
Implementa os três algoritmos de substituição de páginas:
- **LRU**: Percorre todos os quadros e seleciona aquele com menor `last_access`
- **NRU**: Classifica páginas em 4 classes baseado nos bits R e M, seleciona da classe mais baixa
- **Ótimo**: Para cada quadro, busca no futuro quando a página será acessada novamente

#### simulator.c
Implementa o loop principal de simulação e funções auxiliares. Processa cada acesso à memória, detecta page faults e invoca os algoritmos de substituição quando necessário.

---

## 3. Solução

### 3.1 Fluxo de Execução

```
1. Parsing de argumentos
2. Validação de parâmetros
3. Cálculo de valores derivados:
   - Bits de offset (s): 8KB→13, 16KB→14, 32KB→15
   - Número de quadros: (memoria_MB × 1024) / pagina_KB
4. Alocação de estruturas (tabela de páginas, quadros)
5. Para algoritmo Ótimo: pré-carrega todo o arquivo de acessos
6. Loop principal de simulação:
   a. Lê endereço e operação (R/W)
   b. Calcula índice da página: page = addr >> s
   c. Se página está em memória (hit):
      - Atualiza bits R e M
   d. Se página não está em memória (page fault):
      - Incrementa contador de page faults
      - Se há quadro livre: usa-o
      - Senão: seleciona vítima pelo algoritmo
        - Se página vítima é suja (M=1): conta escrita
        - Invalida página vítima
      - Carrega nova página no quadro
   e. Incrementa contador de tempo
7. Imprime relatório final
```

### 3.2 Algoritmos de Substituição

#### LRU (Least Recently Used)
Seleciona a página que foi acessada há mais tempo:
```
para cada quadro ocupado:
    se last_access < menor_tempo:
        menor_tempo = last_access
        vitima = quadro
retorna vitima
```

#### NRU (Not Recently Used)
Classifica páginas em 4 classes e seleciona da classe mais baixa:
- Classe 0: R=0, M=0 (não referenciada, não modificada)
- Classe 1: R=0, M=1 (não referenciada, modificada)
- Classe 2: R=1, M=0 (referenciada, não modificada)
- Classe 3: R=1, M=1 (referenciada, modificada)

Os bits R são resetados periodicamente (a cada 1000 acessos).

#### Algoritmo Ótimo (Bélády)
Seleciona a página que será acessada mais tarde no futuro:
```
para cada quadro ocupado:
    procura no futuro quando página será usada
    se nunca será usada: retorna este quadro
    se será usada mais tarde que as outras: marca como vitima
retorna vitima
```

### 4.3 Saída do Programa

Exemplo de execução:
```
$ ./sim-virtual LRU compilador.log 8 2
Executando o simulador...
Arquivo de entrada: compilador.log
Tamanho da memoria fisica: 2 MB
Tamanho das paginas: 8 KB
Algoritmo de substituicao: LRU
Numero de Faltas de Paginas: 21091
Numero de Paginas Escritas: 3839
```

### 3.4 Resultados dos Testes

#### Tabela Comparativa - compilador.log

| Página | Memória | LRU (PF) | LRU (PE) | NRU (PF) | NRU (PE) | OTM (PF) | OTM (PE) |
|--------|---------|----------|----------|----------|----------|----------|----------|
| 8KB    | 1MB     | 36707    | 5775     | 42046    | 4529     | 21271    | 3711     |
| 8KB    | 2MB     | 21091    | 3839     | 38503    | 3276     | 10118    | 2190     |
| 8KB    | 4MB     | 7632     | 1756     | 32561    | 1689     | 4232     | 1078     |
| 16KB   | 1MB     | 47424    | 7770     | 63926    | 7254     | 29650    | 4882     |
| 16KB   | 2MB     | 30686    | 5068     | 36317    | 3968     | 17049    | 3233     |
| 16KB   | 4MB     | 15873    | 3123     | 33751    | 2775     | 7116     | 1665     |
| 32KB   | 1MB     | 60336    | 9429     | 126246   | 8577     | 38814    | 6203     |
| 32KB   | 2MB     | 38641    | 6186     | 46536    | 5804     | 23758    | 3750     |
| 32KB   | 4MB     | 23928    | 3826     | 30859    | 3125     | 12570    | 2431     |

#### Tabela Comparativa - matriz.log

| Página | Memória | LRU (PF) | LRU (PE) | NRU (PF) | NRU (PE) | OTM (PF) | OTM (PE) |
|--------|---------|----------|----------|----------|----------|----------|----------|
| 8KB    | 1MB     | 10928    | 3273     | 17315    | 2467     | 5361     | 1827     |
| 8KB    | 2MB     | 4745     | 1623     | 14824    | 1653     | 2969     | 1109     |
| 8KB    | 4MB     | 2950     | 985      | 12139    | 1066     | 2295     | 843      |
| 16KB   | 1MB     | 15904    | 4517     | 20589    | 3632     | 9914     | 3157     |
| 16KB   | 2MB     | 7876     | 2585     | 15138    | 2104     | 4136     | 1482     |
| 16KB   | 4MB     | 3828     | 1319     | 12692    | 1495     | 2370     | 944      |
| 32KB   | 1MB     | 32282    | 6582     | 63283    | 5418     | 19115    | 4787     |
| 32KB   | 2MB     | 13230    | 3877     | 16726    | 2860     | 8008     | 2618     |
| 32KB   | 4MB     | 5974     | 2011     | 13083    | 1744     | 3301     | 1171     |

#### Tabela Comparativa - compressor.log

| Página | Memória | LRU (PF) | LRU (PE) | NRU (PF) | NRU (PE) | OTM (PF) | OTM (PE) |
|--------|---------|----------|----------|----------|----------|----------|----------|
| 8KB    | 1MB     | 533      | 132      | 917      | 18       | 345      | 86       |
| 8KB    | 2MB     | 255      | 0        | 255      | 0        | 255      | 0        |
| 8KB    | 4MB     | 255      | 0        | 255      | 0        | 255      | 0        |
| 16KB   | 1MB     | 729      | 265      | 1189     | 235      | 483      | 166      |
| 16KB   | 2MB     | 366      | 86       | 710      | 0        | 239      | 55       |
| 16KB   | 4MB     | 209      | 0        | 209      | 0        | 209      | 0        |
| 32KB   | 1MB     | 911      | 345      | 1786     | 452      | 607      | 250      |
| 32KB   | 2MB     | 515      | 180      | 812      | 146      | 334      | 109      |
| 32KB   | 4MB     | 231      | 41       | 442      | 0        | 172      | 34       |

#### Tabela Comparativa - simulador.log

| Página | Memória | LRU (PF) | LRU (PE) | NRU (PF) | NRU (PE) | OTM (PF) | OTM (PE) |
|--------|---------|----------|----------|----------|----------|----------|----------|
| 8KB    | 1MB     | 16479    | 5546     | 24897    | 6000     | 9179     | 3528     |
| 8KB    | 2MB     | 8556     | 3395     | 22524    | 4506     | 4748     | 2160     |
| 8KB    | 4MB     | 4732     | 2094     | 20969    | 3969     | 3498     | 1620     |
| 16KB   | 1MB     | 29547    | 8267     | 34351    | 8403     | 15284    | 5427     |
| 16KB   | 2MB     | 12822    | 4813     | 21775    | 5671     | 7269     | 3017     |
| 16KB   | 4MB     | 6520     | 2859     | 19982    | 4367     | 3833     | 1838     |
| 32KB   | 1MB     | 48088    | 10446    | 71393    | 10589    | 28328    | 7581     |
| 32KB   | 2MB     | 22126    | 7003     | 26558    | 7075     | 11870    | 4421     |
| 32KB   | 4MB     | 9348     | 3744     | 18462    | 4820     | 5305     | 2312     |

#### Resumo Comparativo (8KB, 2MB)

| Arquivo        | LRU (PF) | NRU (PF) | OTM (PF) |
|----------------|----------|----------|----------|
| compilador.log | 21091    | 38503    | 10118    |
| matriz.log     | 4745     | 14824    | 2969     |
| compressor.log | 255      | 255      | 255      |
| simulador.log  | 8556     | 22524    | 4748     |

*PF = Page Faults, PE = Páginas Escritas (sujas)*

---

## 4. Observações e Conclusões

### 4.1 Análise dos Resultados

1. **Algoritmo Ótimo**: Sempre apresenta o menor número de page faults, como esperado. É o limite inferior teórico, pois conhece todos os acessos futuros (algoritmo de Bélády). Serve como referência para avaliar os outros algoritmos.

2. **Algoritmo LRU**: Apresenta desempenho intermediário, aproximando-se do ótimo em cargas de trabalho com boa localidade temporal. É uma aproximação prática do algoritmo ótimo.

3. **Algoritmo NRU**: Mais simples mas menos preciso, resultando em mais page faults na maioria dos casos. Sua vantagem é a simplicidade de implementação.

4. **Efeito do tamanho da memória**: Aumentar a memória física reduz significativamente os page faults para todos os algoritmos. Isso é esperado, pois mais quadros permitem manter mais páginas em memória.

5. **Efeito do tamanho da página**: Páginas maiores podem aumentar page faults devido a:
   - Menor número de quadros disponíveis (menos páginas cabem na memória)
   - Fragmentação interna (desperdício de espaço dentro das páginas)

6. **Caso especial - compressor.log**: O compressor acessa apenas ~255 páginas distintas (com páginas de 8KB). Com 2MB de memória (256 quadros), todas as páginas cabem na memória, resultando em apenas faltas compulsórias (inevitáveis no primeiro acesso). Por isso, todos os algoritmos apresentam o mesmo resultado.

### 4.2 Dificuldades Encontradas

- **Algoritmo Ótimo**: A implementação requer pré-carregar todo o arquivo de acessos na memória para poder "olhar o futuro", o que aumenta o consumo de memória do simulador.
- **Tamanho da tabela de páginas**: Com endereços de 32 bits e páginas de 8KB (13 bits de offset), a tabela pode ter até 2^19 entradas, exigindo alocação dinâmica cuidadosa.

### 4.3 O Que Funciona

- Todos os três algoritmos de substituição (LRU, NRU, Ótimo)
- Todos os tamanhos de página suportados (8, 16, 32 KB)
- Todos os tamanhos de memória suportados (1, 2, 4 MB)
- Leitura correta dos arquivos de log
- Contagem precisa de page faults e páginas sujas
- Validação de parâmetros de entrada

### 4.4 Conclusão

O simulador foi implementado com sucesso, permitindo comparar o desempenho dos três algoritmos de substituição de páginas em diferentes configurações de memória. Os resultados confirmam a teoria: o algoritmo Ótimo é inalcançável na prática (requer conhecimento do futuro), mas o LRU oferece uma boa aproximação. O NRU, apesar de mais simples, pode ser uma opção viável quando a sobrecarga do LRU não é aceitável.

