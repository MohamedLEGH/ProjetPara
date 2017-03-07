target: decideMP

CFLAGS=-fopenmp -O3 -std=c99

main.o: mainMP.c projet.h
aux.o: aux.c projet.h


decideMP: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o decideMP

