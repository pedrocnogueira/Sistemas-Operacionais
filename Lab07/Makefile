# Compila os dois programas na mesma pasta: questão 1 (assíncrona) e questão 2 (síncrona)

CC = gcc
CFLAGS = -Wall -O2

# Alvos
ALL = produtor_consumidor troca_sincrona

all: $(ALL)

produtor_consumidor: produtor_consumidor.c
	$(CC) $(CFLAGS) produtor_consumidor.c -o produtor_consumidor

troca_sincrona: troca_sincrona.c
	$(CC) $(CFLAGS) troca_sincrona.c -o troca_sincrona

# Execuções rápidas
run-q1: produtor_consumidor
	./produtor_consumidor

run-q2: troca_sincrona
	./troca_sincrona

clean:
	rm -f $(ALL)
