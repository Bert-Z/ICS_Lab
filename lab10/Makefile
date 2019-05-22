CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

all: proxy hostinfo echoclient echoserveri

proxy.o: proxy.c csapp.h

csapp.o: csapp.c csapp.h

hostinfo.o: hostinfo.c csapp.h

hostinfo: hostinfo.o csapp.o

proxy: proxy.o csapp.o

echoclient.o: echoclient.c csapp.h

echoserveri.o: echoserveri.c csapp.h

echoclient:echoclient.o csapp.o

echoserveri:echoserveri.o csapp.o

clean:
	rm -f *~ *.o proxy proxy.log
