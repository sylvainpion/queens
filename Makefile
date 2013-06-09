#### Makefile automagique ####

CC = gcc
CFLAGS =  -DRG=21 -DRG_MASTER=5 -UDEBUG -DDBG_LVL=3 -O6 -Wall -pedantic -mpentium
LDFLAGS = 

###########################################

all:	apart master slave

master:	master.c machines.h Makefile rezo.h 
	$(CC) $(CFLAGS) $(LDFLAGS) -o master master.c

slave:	slave.o algo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o slave slave.o algo.o

apart:	apart.o algo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o apart apart.o algo.o

algo.o: algo.c Makefile algo.h
	$(CC) $(CFLAGS) -o algo.o -c algo.c 

apart.o: apart.c Makefile algo.h
	$(CC) $(CFLAGS) -o apart.o -c apart.c

slave.o: slave.c Makefile algo.h rezo.h
	$(CC) $(CFLAGS) -o slave.o -c slave.c

clean:
	rm -f *.o master slave apart core
