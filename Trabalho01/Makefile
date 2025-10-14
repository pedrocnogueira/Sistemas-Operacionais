CC=gcc
CFLAGS=-Wall -O2 -I./include

BIN=launcher kernelsim interctl app

all: $(BIN)

launcher: src/launcher.c include/common.h
	$(CC) $(CFLAGS) -o $@ src/launcher.c

kernelsim: src/kernelsim.c include/common.h
	$(CC) $(CFLAGS) -o $@ src/kernelsim.c

interctl: src/interctl.c include/common.h
	$(CC) $(CFLAGS) -o $@ src/interctl.c

app: src/app.c include/common.h
	$(CC) $(CFLAGS) -o $@ src/app.c

clean:
	rm -f $(BIN)
