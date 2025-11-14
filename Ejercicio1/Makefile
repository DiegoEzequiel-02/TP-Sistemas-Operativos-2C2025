CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = alumnos
SRC = main.c

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
