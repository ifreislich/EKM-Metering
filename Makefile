CFLAGS= -Wall -g -I/usr/local/include
LDFLAGS= -L/usr/local/lib

all: ekm

ekm.o: ekm.c ekm.h ekmprivate.h
	cc ${CFLAGS} -c ekm.c

libekm.o: libekm.c ekm.h ekmprivate.h
	cc ${CFLAGS} -c libekm.c

ekm: ekm.o libekm.o
	cc ${LDFLAGS} -o ekm libekm.o ekm.o

clean:
	rm -f ekm *.o *.core
