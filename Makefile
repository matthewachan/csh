CC = gcc
CFLAGS = -Wall -g -I.

EXE = w4118_sh
DEPS = queue.h
OBJ = $(EXE).o queue.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o $(EXE) queue *.dSYM
