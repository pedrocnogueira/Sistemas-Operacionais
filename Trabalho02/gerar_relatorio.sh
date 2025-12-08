#!/bin/bash
# Script para gerar relatório completo dos testes do simulador de memória virtual
# Uso: ./gerar_relatorio.sh [arquivo_saida]

OUTPUT_FILE="${1:-relatorio_testes.txt}"
SIMULADOR="./sim-virtual"
ENTRADAS_DIR="Entradas"

# Verifica se o simulador existe
if [ ! -f "$SIMULADOR" ]; then
    echo "Erro: Simulador não encontrado. Execute 'make' primeiro."
    exit 1
fi

# Arrays de configuração
ARQUIVOS=("compilador" "matriz" "compressor" "simulador")
PAGINAS=(8 16 32)
MEMORIAS=(1 2 4)
ALGORITMOS=("LRU" "NRU" "OTM")

# Função para extrair apenas os números do resultado
extrair_resultado() {
    local output="$1"
    local page_faults=$(echo "$output" | grep "Faltas de Paginas" | awk '{print $NF}')
    local paginas_escritas=$(echo "$output" | grep "Paginas Escritas" | awk '{print $NF}')
    echo "$page_faults $paginas_escritas"
}

# Início do relatório
{
    echo "================================================================================"
    echo "         RELATÓRIO DE TESTES - SIMULADOR DE MEMÓRIA VIRTUAL"
    echo "================================================================================"
    echo ""
    echo "Data de geração: $(date '+%d/%m/%Y %H:%M:%S')"
    echo ""
    echo "================================================================================"
    echo ""

    # Para cada arquivo de entrada
    for arquivo in "${ARQUIVOS[@]}"; do
        echo "================================================================================"
        echo "                         ARQUIVO: ${arquivo}.log"
        echo "================================================================================"
        echo ""
        
        # Cabeçalho da tabela
        printf "%-8s | %-8s | %-12s | %-12s | %-12s | %-12s | %-12s | %-12s |\n" \
               "Página" "Memória" "LRU-PF" "LRU-PE" "NRU-PF" "NRU-PE" "OTM-PF" "OTM-PE"
        printf "%-8s-+-%-8s-+-%-12s-+-%-12s-+-%-12s-+-%-12s-+-%-12s-+-%-12s-+\n" \
               "--------" "--------" "------------" "------------" "------------" "------------" "------------" "------------"
        
        # Para cada combinação de tamanho
        for pag in "${PAGINAS[@]}"; do
            for mem in "${MEMORIAS[@]}"; do
                # Executa os três algoritmos
                resultado_lru=$($SIMULADOR LRU "$ENTRADAS_DIR/${arquivo}.log" "${pag}${mem}" 2>/dev/null)
                resultado_nru=$($SIMULADOR NRU "$ENTRADAS_DIR/${arquivo}.log" "${pag}${mem}" 2>/dev/null)
                resultado_otm=$($SIMULADOR OTM "$ENTRADAS_DIR/${arquivo}.log" "${pag}${mem}" 2>/dev/null)
                
                # Extrai os valores
                read lru_pf lru_pe <<< $(extrair_resultado "$resultado_lru")
                read nru_pf nru_pe <<< $(extrair_resultado "$resultado_nru")
                read otm_pf otm_pe <<< $(extrair_resultado "$resultado_otm")
                
                # Imprime linha da tabela
                printf "%-8s | %-8s | %-12s | %-12s | %-12s | %-12s | %-12s | %-12s |\n" \
                       "${pag}KB" "${mem}MB" "$lru_pf" "$lru_pe" "$nru_pf" "$nru_pe" "$otm_pf" "$otm_pe"
            done
        done
        echo ""
        echo "Legenda: PF = Page Faults, PE = Páginas Escritas (sujas)"
        echo ""
        echo ""
    done

    # Resumo comparativo
    echo "================================================================================"
    echo "                           ANÁLISE COMPARATIVA"
    echo "================================================================================"
    echo ""
    echo "Configuração padrão: Página 8KB, Memória 2MB"
    echo ""
    printf "%-15s | %-12s | %-12s | %-12s |\n" "Arquivo" "LRU (PF)" "NRU (PF)" "OTM (PF)"
    printf "%-15s-+-%-12s-+-%-12s-+-%-12s-+\n" "---------------" "------------" "------------" "------------"
    
    for arquivo in "${ARQUIVOS[@]}"; do
        resultado_lru=$($SIMULADOR LRU "$ENTRADAS_DIR/${arquivo}.log" 82 2>/dev/null)
        resultado_nru=$($SIMULADOR NRU "$ENTRADAS_DIR/${arquivo}.log" 82 2>/dev/null)
        resultado_otm=$($SIMULADOR OTM "$ENTRADAS_DIR/${arquivo}.log" 82 2>/dev/null)
        
        read lru_pf lru_pe <<< $(extrair_resultado "$resultado_lru")
        read nru_pf nru_pe <<< $(extrair_resultado "$resultado_nru")
        read otm_pf otm_pe <<< $(extrair_resultado "$resultado_otm")
        
        printf "%-15s | %-12s | %-12s | %-12s |\n" "${arquivo}.log" "$lru_pf" "$nru_pf" "$otm_pf"
    done
    echo ""
    echo ""

    # Estatísticas gerais
    echo "================================================================================"
    echo "                              OBSERVAÇÕES"
    echo "================================================================================"
    echo ""
    echo "1. O algoritmo ÓTIMO (OTM) sempre apresenta o menor número de page faults,"
    echo "   pois conhece todos os acessos futuros (algoritmo de Bélády)."
    echo ""
    echo "2. O algoritmo LRU geralmente apresenta desempenho intermediário,"
    echo "   aproximando-se do ótimo em cargas de trabalho com boa localidade temporal."
    echo ""
    echo "3. O algoritmo NRU é mais simples mas menos preciso, resultando em"
    echo "   mais page faults na maioria dos casos."
    echo ""
    echo "4. Aumentar a memória física reduz page faults para todos os algoritmos."
    echo ""
    echo "5. Páginas maiores podem aumentar page faults devido à fragmentação interna"
    echo "   e menor número de quadros disponíveis."
    echo ""
    echo "================================================================================"
    echo "                           FIM DO RELATÓRIO"
    echo "================================================================================"

} > "$OUTPUT_FILE"

echo "Relatório gerado com sucesso: $OUTPUT_FILE"
echo ""
echo "Para visualizar: cat $OUTPUT_FILE"

