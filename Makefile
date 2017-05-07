target: decide decideMPIv1 decideMPIv2 decideMP decideMPv2 decideMPv3
CC = gcc
MCC = mpicc
CFLAGS=-fopenmp -O3 -std=c99

mainMP.o:mainMP.c projet.h
	$(CC) $(CFLAGS) -c mainMP.c
mainMPv2.o:mainMPv2.c projet.h
	$(CC) $(CFLAGS) -c mainMPv2.c
mainMPv3.o:mainMPv3.c projet.h
	$(CC) $(CFLAGS) -c mainMPv3.c 
mainMPIv1.o:mainMPIv1.c projetv2.h
	$(MCC) -O3 -std=c99 -c mainMPIv1.c -o mainMPIv1.o
mainMPIv2.o:mainMPIv2.c projetv2.h
	$(MCC) -O3 -std=c99 -c mainMPIv2.c -o mainMPIv2.o	
main.o:mainMP.c projet.h
	$(CC) -O3 -std=c99 -c main.c
aux.o: aux.c projet.h
	$(CC) $(CFLAGS) -c aux.c



decideMP: mainMP.o aux.o
	$(CC) $(CFLAGS) $^ -o $@
decideMPv2: mainMPv2.o aux.o
	$(CC) $(CFLAGS) $^ -o $@
decideMPv3: mainMPv3.o aux.o
	$(CC) $(CFLAGS) $^ -o $@
decideMPIv1: mainMPIv1.o aux.o
	$(MCC) -O3 -std=c99 $^ -o $@
decideMPIv2: mainMPIv2.o aux.o
	$(MCC) -O3 -std=c99 $^ -o $@
decide: main.o aux.o
	$(CC) -O3 -std=c99 $^ -o $@



.PHONY: clean

clean:
	rm -f *.o decideMPIv1 decideMPIv2 decideMP decide decideMPv2 decideMPv3

