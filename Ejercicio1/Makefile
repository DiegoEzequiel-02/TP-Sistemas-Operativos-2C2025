# filepath: /home/diego/Documentos/TP-Sistemas-Operativos-2C2025-1/Ejercicio1/Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = alumnos
SRC = main.c

# valores por defecto para run (se pueden sobrescribir: make run GEN=3 TOTAL=30)
GEN ?= 5
TOTAL ?= 50

all: $(TARGET)

$(TARGET): $(SRC)
    $(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: all
    ./$(TARGET) $(GEN) $(TOTAL)

clean:
    rm -f $(TARGET) alumnos.csv run_output.txt trace.txt

.PHONY: all run clean
