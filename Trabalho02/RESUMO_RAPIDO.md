# 📝 Resumo Rápido - Algoritmos de Substituição

## LRU (Least Recently Used)
- **O que faz:** Substitui página menos recentemente acessada
- **Como:** Mantém `last_access` (timestamp), escolhe menor valor
- **Complexidade:** O(n) por substituição

## NRU (Not Recently Used)
- **O que faz:** Classifica em 4 classes (R,M), escolhe da classe mais baixa
- **Classes:** 0=(0,0), 1=(0,1), 2=(1,0), 3=(1,1)
- **Múltiplas na mesma classe:** Primeira encontrada (sem critério adicional)
- **Reset:** Bits R resetados a cada 1000 acessos
- **Complexidade:** O(n) por substituição

## Ótimo (Belady)
- **O que faz:** Substitui página que será usada mais tarde (ou nunca)
- **Como:** Pré-carrega log, busca próximo acesso futuro
- **Realista?** Não - precisa ver o futuro
- **Complexidade:** O(n × m) onde m = acessos futuros

---

## 🔑 Questões Frequentes

**Q: Múltiplas páginas na mesma classe NRU?**  
A: Primeira encontrada na varredura (sem desempate adicional)

**Q: Por que ótimo pré-carrega?**  
A: Precisa ver acessos futuros para decidir (não é realista)

**Q: Quando atualiza R e M?**  
A: No hit (R=1 sempre, M=1 se escrita) e no load (R=1, M=0 inicial)

**Q: Diferença tabela vs quadros?**  
A: Tabela mapeia virtual→físico, quadros têm metadados (R,M,last_access)

**Q: Reset NRU?**  
A: A cada 1000 acessos, reseta todos os bits R

**Q: Páginas escritas?**  
A: Contadas quando substituímos página com M=1

**Q: Cálculo quadros?**  
A: `(memória_MB × 1024) / página_KB`

**Q: Extração de página?**  
A: `address >> offset_bits` (desloca bits de offset)

---

## 📊 Estruturas Principais

```c
PageTableEntry {
    int valid;
    int frame_number;
}

Frame {
    int occupied;
    unsigned page_number;
    int R, M;
    unsigned last_access;  // LRU
    int next_use;          // Ótimo
}
```
