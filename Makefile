target: decideMPI decideMP decide
CC = gcc
MCC = mpicc
CFLAGS=-fopenmp -O3 -std=c99

mainMP.o:mainMP.c projet.h
	$(CC) $(CFLAGS) -c mainMP.c 
mainMPI.o:mainMPI.c projet.h
	$(MCC) -O3 -std=c99 -c mainMPI.c
main.o:mainMP.c projet.h
	$(CC) -O3 -std=c99 -c main.c
aux.o: aux.c projet.h
	$(CC) $(CFLAGS) -c aux.c



decideMP: mainMP.o aux.o
	$(CC) $(CFLAGS) $^ -o $@
decideMPI: mainMPI.o aux.o
	$(MCC) -O3 -std=c99 $^ -o $@
decide: main.o aux.o
	$(CC) -O3 -std=c99 $^ -o $@



.PHONY: clean

clean:
	rm -f *.o decideMPI decideMP decide

