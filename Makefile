INCLUDE_DIR = include
SRC_DIR = src

CC = gcc
CFLAGS = -Wall -g -Iinclude

EXE = w4118_sh
DEPS = $(INCLUDE_DIR)/queue.h
OBJ = $(EXE).o queue.o

all: $(EXE)

debug: CFLAGS += -DDEBUG
debug: $(EXE)

%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o $(EXE) queue *.dSYM
