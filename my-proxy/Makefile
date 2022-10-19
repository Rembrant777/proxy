CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o cache.o

all: proxy tiny

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c


proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

proxy: proxy.o cache.o csapp.o sbuf.o
	$(CC) $(CFLAGS) proxy.o cache.o csapp.o sbuf.o -o proxy $(LDFLAGS)

tiny:
	(cd tiny; make clean; make)
	(cd tiny/cgi-bin; make clean; make)

clean:
	rm -f *~ *.o proxy core 
	(cd tiny; make clean)
	(cd tiny/cgi-bin; make clean)
