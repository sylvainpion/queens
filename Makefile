###########################################
# Makefile pour le probleme des N reines. #
###########################################

CC	= gcc
CFLAGS	= -DRG=17 -DRG_MASTER=2 -UDEBUG -O6 -Wall -pedantic

###########################################

all:	apart master slave

master:	master.c
	$(CC) $(CFLAGS) -o master master.c

slave:	slave.o algo.o
	$(CC) $(CFLAGS) -o slave slave.o algo.o

apart:	apart.o algo.o
	$(CC) $(CFLAGS) -o apart apart.o algo.o

algo.o: algo.c
	$(CC) $(CFLAGS) -o algo.o -c algo.c 

apart.o: apart.c
	$(CC) $(CFLAGS) -o apart.o -c apart.c

slave.o: slave.c
	$(CC) $(CFLAGS) -o slave.o -c slave.c

clean:
	rm -f *.o master slave apart core
