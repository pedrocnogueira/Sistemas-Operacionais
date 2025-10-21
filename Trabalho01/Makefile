CC=gcc
CFLAGS=-Wall -O2 -I./include

BIN_DIR=bin
OUTPUT_DIR=output
BIN=launcher kernelsim interctl app
BIN_PATHS=$(addprefix $(BIN_DIR)/,$(BIN))

all: $(BIN_PATHS)

# Cria diretórios necessários
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BIN_DIR)/launcher: src/launcher.c include/common.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/launcher.c

$(BIN_DIR)/kernelsim: src/kernelsim.c include/common.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/kernelsim.c

$(BIN_DIR)/interctl: src/interctl.c include/common.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/interctl.c

$(BIN_DIR)/app: src/app.c include/common.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/app.c

clean:
	rm -rf $(BIN_DIR) $(OUTPUT_DIR)

# Targets para execução dos testes
test1: $(BIN_PATHS) | $(OUTPUT_DIR)
	@echo "=== Executando Teste 1: A1, A2, A3 (sem I/O) ==="
	cd $(BIN_DIR) && ./launcher --test1 2>&1 | tee ../$(OUTPUT_DIR)/test1_output.txt
	@echo "Resultado salvo em $(OUTPUT_DIR)/test1_output.txt"

test2: $(BIN_PATHS) | $(OUTPUT_DIR)
	@echo "=== Executando Teste 2: A4, A5, A6 (com I/O) ==="
	cd $(BIN_DIR) && ./launcher --test2 2>&1 | tee ../$(OUTPUT_DIR)/test2_output.txt
	@echo "Resultado salvo em $(OUTPUT_DIR)/test2_output.txt"

test3: $(BIN_PATHS) | $(OUTPUT_DIR)
	@echo "=== Executando Teste 3: Todas as 6 tarefas (A1-A6) ==="
	cd $(BIN_DIR) && ./launcher --test3 2>&1 | tee ../$(OUTPUT_DIR)/test3_output.txt
	@echo "Resultado salvo em $(OUTPUT_DIR)/test3_output.txt"

test-all: test1 test2 test3
	@echo "=== Todos os testes executados ==="

clean-logs:
	rm -f $(OUTPUT_DIR)/test1_output.txt $(OUTPUT_DIR)/test2_output.txt $(OUTPUT_DIR)/test3_output.txt
