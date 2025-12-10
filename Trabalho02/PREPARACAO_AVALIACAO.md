# Preparação para Avaliação - Trabalho 02: Simulador de Memória Virtual

## 📋 Explicações Rápidas dos Algoritmos

### 1. LRU (Least Recently Used)
**Conceito:** Substitui a página que foi acessada há mais tempo.

**Implementação:**
- Cada quadro mantém um campo `last_access` que armazena o instante (timestamp) do último acesso
- Quando há um page fault e precisa substituir, percorre todos os quadros ocupados
- Seleciona o quadro com o menor valor de `last_access` (mais antigo)
- A cada acesso a uma página em memória, atualiza `last_access` com o tempo atual

**Código chave:**
```c
// Atualização no hit
frames[frame].last_access = time;

// Seleção da vítima
for (int i = 0; i < num_frames; i++) {
    if (frames[i].last_access < oldest_access) {
        oldest_access = frames[i].last_access;
        victim = i;
    }
}
```

**Complexidade:** O(n) onde n = número de quadros

---

### 2. NRU (Not Recently Used)
**Conceito:** Classifica páginas em 4 classes baseado nos bits R (referência) e M (modificação), e seleciona uma página da classe mais baixa.

**Classes:**
- **Classe 0:** R=0, M=0 (não referenciada, não modificada) - **melhor candidata**
- **Classe 1:** R=0, M=1 (não referenciada, modificada)
- **Classe 2:** R=1, M=0 (referenciada, não modificada)
- **Classe 3:** R=1, M=1 (referenciada, modificada) - **pior candidata**

**Implementação:**
- Calcula a classe de cada página: `nru_class = (R << 1) | M`
- Mantém um array `class_victims[4]` que armazena a primeira página encontrada em cada classe
- Retorna a primeira classe não vazia (menor número = menor prioridade = melhor vítima)
- **Reset periódico:** A cada 1000 acessos, todos os bits R são resetados para 0

**Código chave:**
```c
// Cálculo da classe
int nru_class = (frames[i].R << 1) | frames[i].M;

// Seleção (primeira classe não vazia)
for (int c = 0; c < 4; c++) {
    if (class_victims[c] != -1) {
        return class_victims[c];
    }
}
```

**Decisão para múltiplas páginas na mesma classe:**
- **Implementação atual:** Seleciona a **primeira página encontrada** na classe mais baixa
- Não há critério de desempate adicional (como FIFO ou LRU dentro da classe)
- Isso é uma simplificação válida, pois o NRU não especifica qual página escolher dentro da mesma classe

**Complexidade:** O(n) onde n = número de quadros

---

### 3. Algoritmo Ótimo (Belady)
**Conceito:** Substitui a página que será acessada mais tarde no futuro (ou nunca mais será acessada).

**Implementação:**
- **Pré-carrega todo o log de acessos** em memória antes de iniciar a simulação
- Para cada quadro ocupado, procura no futuro quando aquela página será acessada novamente
- Se a página não será mais acessada (`next_use = -1`), ela é imediatamente escolhida
- Caso contrário, escolhe a página com o maior `next_use` (será acessada mais tarde)

**Código chave:**
```c
// Busca no futuro
for (unsigned j = current_index + 1; j < log->count; j++) {
    unsigned future_page = get_page_index(log->addresses[j], offset_bits);
    if (future_page == frames[i].page_number) {
        frames[i].next_use = (int)j;
        break;
    }
}

// Seleção: nunca mais usada > usada mais tarde
if (frames[i].next_use == -1) {
    return i;  // Melhor candidata
}
```

**Por que pré-carrega o log?**
- O algoritmo ótimo precisa "olhar o futuro" para tomar decisões
- Não é possível em um sistema real, mas é útil para comparação teórica
- Permite calcular o limite inferior de page faults

**Complexidade:** O(n × m) onde n = quadros, m = acessos futuros restantes

---

## ❓ Questões que o Professor Pode Fazer

### 1. **"Como vocês implementaram a decisão para caso tenha múltiplas páginas em uma classe no NRU?"**

**Resposta:**
Quando há múltiplas páginas na mesma classe NRU, nossa implementação seleciona a **primeira página encontrada** durante a varredura dos quadros. Isso é feito através do array `class_victims[4]`, onde armazenamos apenas o primeiro índice encontrado para cada classe.

```c
if (class_victims[nru_class] == -1) {
    class_victims[nru_class] = i;  // Primeira página da classe
}
```

**Justificativa:**
- O algoritmo NRU não especifica um critério de desempate dentro da mesma classe
- Nossa escolha é determinística e simples de implementar
- Alternativas poderiam ser FIFO ou LRU dentro da classe, mas isso adicionaria complexidade sem benefício teórico claro

---

### 2. **"Por que o algoritmo Ótimo precisa pré-carregar o log? Isso é realista?"**

**Resposta:**
O algoritmo ótimo precisa pré-carregar o log porque ele toma decisões baseadas em **acessos futuros**. Para cada página em memória, ele precisa saber quando (ou se) ela será acessada novamente.

**Não é realista** - em um sistema operacional real, não temos como saber os acessos futuros. O algoritmo ótimo é usado apenas como:
- **Referência teórica:** fornece o limite inferior de page faults
- **Comparação:** permite avaliar quão próximos outros algoritmos estão do ideal
- **Pesquisa:** útil para entender o comportamento de algoritmos

---

### 3. **"Como vocês atualizam os bits R e M? Quando isso acontece?"**

**Resposta:**
Os bits são atualizados em dois momentos:

1. **No hit (página já em memória):**
   - Bit R sempre é setado para 1 (página foi referenciada)
   - Bit M é setado para 1 apenas se a operação for escrita (W)
   - Atualiza também `last_access` para LRU

2. **No page fault (carregamento de nova página):**
   - Bit R é setado para 1 (página acabou de ser carregada)
   - Bit M é setado para 0 inicialmente, e depois para 1 se a operação for escrita

**Código:**
```c
// No hit
frames[frame].R = 1;
if (operation == 'W' || operation == 'w') {
    frames[frame].M = 1;
}

// No load
frames[frame_number].R = 1;
frames[frame_number].M = 0;  // Depois pode ser atualizado se for escrita
```

---

### 4. **"Qual a diferença entre a tabela de páginas e os quadros (frames)?"**

**Resposta:**
- **Tabela de Páginas:** Mapeia páginas virtuais → quadros físicos
  - Uma entrada por página virtual possível
  - Campos: `valid` (página está em memória?) e `frame_number` (qual quadro?)
  - Tamanho: `2^(32 - offset_bits)` entradas

- **Quadros (Frames):** Representam a memória física
  - Um quadro por página que cabe na memória física
  - Campos: `occupied`, `page_number`, `R`, `M`, `last_access`, `next_use`
  - Tamanho: `(memória_total / tamanho_página)` quadros

**Relação:**
- Tabela de páginas: "A página virtual X está no quadro Y?"
- Quadros: "O quadro Y contém a página X e seus metadados"

---

### 5. **"Como vocês calculam o número de quadros e o tamanho da tabela de páginas?"**

**Resposta:**

**Número de quadros:**
```c
num_frames = (memory_size_mb * 1024) / page_size_kb
```
Exemplo: 2 MB / 8 KB = 2048 KB / 8 KB = 256 quadros

**Bits de offset:**
```c
offset_bits = log2(page_size_bytes)
```
- 8 KB = 8192 bytes = 2^13 → offset_bits = 13
- 16 KB = 16384 bytes = 2^14 → offset_bits = 14
- 32 KB = 32768 bytes = 2^15 → offset_bits = 15

**Tamanho da tabela de páginas:**
```c
page_table_size = 2^(32 - offset_bits)
```
- Com 8 KB: 2^(32-13) = 2^19 = 524.288 entradas
- Com 16 KB: 2^(32-14) = 2^18 = 262.144 entradas
- Com 32 KB: 2^(32-15) = 2^17 = 131.072 entradas

---

### 6. **"Por que o NRU reseta os bits R periodicamente? Qual o intervalo?"**

**Resposta:**
O reset periódico dos bits R é necessário para que o algoritmo NRU funcione corretamente. Sem o reset, todas as páginas eventualmente teriam R=1, e o algoritmo perderia sua capacidade de distinguir páginas recentemente usadas das não usadas.

**Intervalo:** A cada **1000 acessos** (`NRU_RESET_INTERVAL = 1000`)

**Implementação:**
```c
if (config->algorithm == ALG_NRU && time > 0 && time % NRU_RESET_INTERVAL == 0) {
    reset_reference_bits(frames, config->num_frames);
}
```

**Por que 1000?**
- É um valor razoável que permite distinguir páginas recentes das antigas
- Muito pequeno: perde informação útil
- Muito grande: todas as páginas ficam com R=1

---

### 7. **"Como vocês contam as páginas escritas (dirty pages)?"**

**Resposta:**
Contamos uma página escrita quando:
1. Há um page fault e precisamos substituir uma página
2. A página vítima tem o bit M = 1 (foi modificada)
3. Antes de invalidá-la, incrementamos `dirty_pages_written`

**Código:**
```c
if (frames[frame].M) {
    stats->dirty_pages_written++;
}
invalidate_page(page_table, frames, frame);
```

**Significado:** Representa o custo de I/O para escrever páginas modificadas de volta ao disco antes de substituí-las.

---

### 8. **"Qual a complexidade de cada algoritmo? Há otimizações possíveis?"**

**Resposta:**

| Algoritmo | Complexidade | Otimizações Possíveis |
|-----------|--------------|----------------------|
| **LRU** | O(n) por substituição | Usar heap ou estrutura ordenada para O(log n) |
| **NRU** | O(n) por substituição | Similar ao LRU, mas menos crítico |
| **Ótimo** | O(n × m) onde m = acessos futuros | Pré-processar índices de próximos acessos para O(n) |

**Otimizações não implementadas:**
- **LRU:** Poderia usar uma lista duplamente ligada ou heap para O(log n)
- **Ótimo:** Poderia pré-processar um array de "próximo acesso" para cada página, reduzindo para O(n)

---

### 9. **"Como vocês extraem o número da página de um endereço virtual?"**

**Resposta:**
Usamos deslocamento de bits (bit shift):

```c
unsigned get_page_index(unsigned address, int offset_bits) {
    return address >> offset_bits;
}
```

**Exemplo:**
- Endereço: `0x12345678`
- Offset bits: 13 (página de 8 KB)
- Página: `0x12345678 >> 13 = 0x2468`

O offset (parte baixa do endereço) é descartado, e apenas os bits superiores (número da página) são mantidos.

---

### 10. **"O que acontece quando a memória ainda tem quadros livres?"**

**Resposta:**
Quando `frames_used < num_frames`, não há necessidade de substituir uma página. O código procura um quadro livre usando `find_free_frame()` e carrega a nova página diretamente:

```c
if (frames_used < config->num_frames) {
    frame = find_free_frame(frames, config->num_frames);
    frames_used++;
} else {
    // Precisa substituir
    frame = select_victim(...);
}
```

Neste caso, não há página vítima para invalidar, e não há escrita de página suja.

---

## 🎯 Pontos-Chave para Memorizar

1. **NRU:** 4 classes, seleciona primeira da classe mais baixa, reset de R a cada 1000 acessos
2. **LRU:** Usa timestamp (`last_access`), substitui a mais antiga
3. **Ótimo:** Pré-carrega log, olha o futuro, não é realista mas é referência teórica
4. **Bits R e M:** Atualizados em hits e no carregamento
5. **Tabela de páginas vs Quadros:** Tabela mapeia virtual→físico, quadros têm metadados
6. **Cálculo de quadros:** `(memória_MB * 1024) / página_KB`
7. **Páginas escritas:** Contadas quando substituímos página com M=1

---

## 💡 Dicas para a Avaliação

- **Seja específico:** Cite nomes de funções e estruturas quando possível
- **Mencione trade-offs:** Por exemplo, "Ótimo não é realista, mas é útil para comparação"
- **Explique decisões de design:** "Escolhemos primeira página da classe no NRU porque..."
- **Conheça os números:** Intervalo de reset (1000), tamanhos válidos (8/16/32 KB, 1/2/4 MB)
- **Entenda a estrutura:** Saiba diferenciar tabela de páginas de quadros
