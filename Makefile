CC=gcc
CFLAGS=-Wall -g

EXE=w4118_sh

$(EXE): $(EXE).c
	$(CC) $(EXE).c -o $(EXE) $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o w4118_sh *.dSYM
