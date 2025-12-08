# Simulador de Memória Virtual - sim-virtual
# INF1316 - Sistemas Operacionais
# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -O2
SRCDIR = src
OBJDIR = obj
TARGET = sim-virtual

# Arquivos fonte
SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/simulator.c \
       $(SRCDIR)/page_table.c \
       $(SRCDIR)/algorithms.c

# Arquivos objeto
OBJS = $(OBJDIR)/main.o \
       $(OBJDIR)/simulator.o \
       $(OBJDIR)/page_table.o \
       $(OBJDIR)/algorithms.o

# Regra principal
all: $(OBJDIR) $(TARGET)

# Cria diretório de objetos
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Linkagem
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilação dos arquivos .c
$(OBJDIR)/main.o: $(SRCDIR)/main.c $(SRCDIR)/simulator.h $(SRCDIR)/types.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/simulator.o: $(SRCDIR)/simulator.c $(SRCDIR)/simulator.h $(SRCDIR)/types.h $(SRCDIR)/page_table.h $(SRCDIR)/algorithms.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/page_table.o: $(SRCDIR)/page_table.c $(SRCDIR)/page_table.h $(SRCDIR)/types.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/algorithms.o: $(SRCDIR)/algorithms.c $(SRCDIR)/algorithms.h $(SRCDIR)/types.h $(SRCDIR)/page_table.h
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa arquivos compilados
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Recompila tudo
rebuild: clean all

# Arquivos de entrada
LOGS = compilador matriz compressor simulador

# Testes rápidos (um arquivo, um algoritmo)
test-lru:
	./$(TARGET) LRU Entradas/compilador.log 8 2

test-nru:
	./$(TARGET) NRU Entradas/compilador.log 8 2

test-otm:
	./$(TARGET) OTM Entradas/compilador.log 8 2

# Testes completos com todos os 4 arquivos
test-all:
	@echo "==============================================="
	@echo "=== Testes com compilador.log (8KB, 2MB) ==="
	@echo "==============================================="
	@./$(TARGET) LRU Entradas/compilador.log 8 2
	@echo ""
	@./$(TARGET) NRU Entradas/compilador.log 8 2
	@echo ""
	@./$(TARGET) OTM Entradas/compilador.log 8 2
	@echo ""
	@echo "==============================================="
	@echo "=== Testes com matriz.log (8KB, 2MB) ==="
	@echo "==============================================="
	@./$(TARGET) LRU Entradas/matriz.log 8 2
	@echo ""
	@./$(TARGET) NRU Entradas/matriz.log 8 2
	@echo ""
	@./$(TARGET) OTM Entradas/matriz.log 8 2
	@echo ""
	@echo "==============================================="
	@echo "=== Testes com compressor.log (8KB, 2MB) ==="
	@echo "==============================================="
	@./$(TARGET) LRU Entradas/compressor.log 8 2
	@echo ""
	@./$(TARGET) NRU Entradas/compressor.log 8 2
	@echo ""
	@./$(TARGET) OTM Entradas/compressor.log 8 2
	@echo ""
	@echo "==============================================="
	@echo "=== Testes com simulador.log (8KB, 2MB) ==="
	@echo "==============================================="
	@./$(TARGET) LRU Entradas/simulador.log 8 2
	@echo ""
	@./$(TARGET) NRU Entradas/simulador.log 8 2
	@echo ""
	@./$(TARGET) OTM Entradas/simulador.log 8 2

# Testes por arquivo específico
test-compilador:
	@echo "=== compilador.log (8KB, 2MB) ==="
	@./$(TARGET) LRU Entradas/compilador.log 8 2
	@./$(TARGET) NRU Entradas/compilador.log 8 2
	@./$(TARGET) OTM Entradas/compilador.log 8 2

test-matriz:
	@echo "=== matriz.log (8KB, 2MB) ==="
	@./$(TARGET) LRU Entradas/matriz.log 8 2
	@./$(TARGET) NRU Entradas/matriz.log 8 2
	@./$(TARGET) OTM Entradas/matriz.log 8 2

test-compressor:
	@echo "=== compressor.log (8KB, 2MB) ==="
	@./$(TARGET) LRU Entradas/compressor.log 8 2
	@./$(TARGET) NRU Entradas/compressor.log 8 2
	@./$(TARGET) OTM Entradas/compressor.log 8 2

test-simulador:
	@echo "=== simulador.log (8KB, 2MB) ==="
	@./$(TARGET) LRU Entradas/simulador.log 8 2
	@./$(TARGET) NRU Entradas/simulador.log 8 2
	@./$(TARGET) OTM Entradas/simulador.log 8 2

# Teste completo com todas as combinações de tamanho
test-full:
	@echo "=============================================="
	@echo "  TESTE COMPLETO - Todas as combinações"
	@echo "=============================================="
	@for file in compilador matriz compressor simulador; do \
		echo ""; \
		echo "=== $$file.log ==="; \
		for pag in 8 16 32; do \
			for mem in 1 2 4; do \
				echo "--- Pagina: $${pag}KB, Memoria: $${mem}MB ---"; \
				./$(TARGET) LRU Entradas/$$file.log $${pag} $${mem}; \
				./$(TARGET) NRU Entradas/$$file.log $${pag} $${mem}; \
				./$(TARGET) OTM Entradas/$$file.log $${pag} $${mem}; \
				echo ""; \
			done; \
		done; \
	done

.PHONY: all clean rebuild test-lru test-nru test-otm test-all test-compilador test-matriz test-compressor test-simulador test-full

