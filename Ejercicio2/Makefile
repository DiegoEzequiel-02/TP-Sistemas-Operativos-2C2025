CC=gcc
CFLAGS=-Wall -g

SRV_SRC=servidor.c funcion.c
CLT_SRC=main.c funcion.c
CLI_SRC=cliente.c

SRV_BIN=servidor
CLT_BIN=cliente_simple
CLI_BIN=cliente

all: $(SRV_BIN) $(CLI_BIN)

$(SRV_BIN): $(SRV_SRC)
	$(CC) $(CFLAGS) $(SRV_SRC) -o $(SRV_BIN) -lpthread

$(CLT_BIN): $(CLT_SRC)
	$(CC) $(CFLAGS) $(CLT_SRC) -o $(CLT_BIN)

$(CLI_BIN): $(CLI_SRC)
	$(CC) $(CFLAGS) $(CLI_SRC) -o $(CLI_BIN)

clean:
	rm -f $(SRV_BIN) $(CLT_BIN) $(CLI_BIN) *.o

.PHONY: all clean
