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

# Testes rápidos
test-lru:
	./$(TARGET) LRU Entradas/compilador.log 8 2

test-nru:
	./$(TARGET) NRU Entradas/compilador.log 8 2

test-otm:
	./$(TARGET) OTM Entradas/compilador.log 8 2

# Testes completos com todos os arquivos
test-all:
	@echo "=== Testes com compilador.log ==="
	./$(TARGET) LRU Entradas/compilador.log 8 2
	./$(TARGET) NRU Entradas/compilador.log 8 2
	./$(TARGET) OTM Entradas/compilador.log 8 2
	@echo ""
	@echo "=== Testes com matriz.log ==="
	./$(TARGET) LRU Entradas/matriz.log 8 2
	./$(TARGET) NRU Entradas/matriz.log 8 2
	./$(TARGET) OTM Entradas/matriz.log 8 2

.PHONY: all clean rebuild test-lru test-nru test-otm test-all

