CC=gcc

.SUFFIXES: .c .o .h .h.gch

.c.o:
	strace -f -o trace.out $(CC) $(CFLAGS) -c $*.c -o $@
	ls -l trace.out

.h.h.gch:
	$(CC) $(CFLAGS) -c $*.h -o $@

all: main

main: includes.h.gch main.o 
	$(CC) -o main main.o

clean:
	rm -f *.gch *.o trace.out *~ main
