<<<<<<< HEAD
target: decideMPI decideMPI_def decideMP decide decideMPv2 decideMPv3 decideMPI_openMP
=======
target: decide decidev2 decideMPIv1 decideMPIv2 decideMP decideMPv2 decideMPv3
>>>>>>> 5f2b5fc24a76ce2db0d5a62ba2725cdbb238698c
CC = gcc
MCC = mpicc
CFLAGS=-fopenmp -O3 -std=c99

mainMP.o:mainMP.c projet.h
	$(CC) $(CFLAGS) -c mainMP.c
mainMPv2.o:mainMPv2.c projet.h
	$(CC) $(CFLAGS) -c mainMPv2.c
mainMPv3.o:mainMPv3.c projet.h
	$(CC) $(CFLAGS) -c mainMPv3.c 
<<<<<<< HEAD
mainMPI.o:mainMPI.c projet.h
	$(MCC) -O3 -std=c99 -c mainMPI.c -o mainMPI.o
mainMPI_def.o:mainMPI_def.c projet.h
	$(MCC) -O3 -std=c99 -c mainMPI_def.c -o mainMPI_def.o	
mainMPI_openMP.o:mainMPI_openMP.c projet.h
	$(MCC) -openmp -O3 -std=c99 -c mainMPI_openMP.c -o mainMPI_openMP.o
main.o:mainMP.c projet.h
=======
mainMPIv1.o:mainMPIv1.c projet.h
	$(MCC) -O3 -std=c99 -c mainMPIv1.c -o mainMPIv1.o
mainMPIv2.o:mainMPIv2.c projetv2.h
	$(MCC) -O3 -std=c99 -c mainMPIv2.c -o mainMPIv2.o	
main.o:main.c projet.h
>>>>>>> 5f2b5fc24a76ce2db0d5a62ba2725cdbb238698c
	$(CC) -O3 -std=c99 -c main.c
mainv2.o:main.c projetv2.h
	$(CC) -O3 -std=c99 -c mainv2.c	
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
decideMPI_openMP: mainMPI_openMP.o aux.o
	$(MCC) -openMP -O3 -std=c99 $^ -o $@
decide: main.o aux.o
	$(CC) -O3 -std=c99 $^ -o $@
decidev2: mainv2.o aux.o
	$(CC) -O3 -std=c99 $^ -o $@



.PHONY: clean

clean:
<<<<<<< HEAD
	rm -f *.o decideMPI decideMPI_def decideMP decide decideMPv2 decideMPv3 decideMPI_openMP
=======
	rm -f *.o decideMPIv1 decideMPIv2 decideMP decide decidev2 decideMPv2 decideMPv3
>>>>>>> 5f2b5fc24a76ce2db0d5a62ba2725cdbb238698c

