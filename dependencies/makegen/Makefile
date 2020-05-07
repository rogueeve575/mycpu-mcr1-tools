CC=gcc
CFLAGS=-c -g -O2 -Wall -fsigned-char -Wno-pointer-sign -Wno-unused-result
LFLAGS=
HFILES=Makefile makegen.h types.h constants.h token.h string.h quicksearch.h fdh.h cache.h summary.h filetime.h dependency.h header.h getline.c

all: makegen

clean:
	rm -f *.o
	rm -f makegen

makegen: makegen.o fdh.o quicksearch.o string.o token.o summary.o cache.o filetime.o dependency.o header.o
	  $(CC) $(LFLAGS) -o makegen \
          makegen.o fdh.o quicksearch.o string.o token.o summary.o cache.o filetime.o dependency.o header.o \

makegen.o:   makegen.c $(HFILES)
	$(CC) $(CFLAGS) makegen.c -o makegen.o

quicksearch.o:  quicksearch.c $(HFILES)
	$(CC) $(CFLAGS) quicksearch.c -o quicksearch.o

string.o:  string.c $(HFILES)
	$(CC) $(CFLAGS) string.c -o string.o

token.o:  token.c $(HFILES)
	$(CC) $(CFLAGS) token.c -o token.o

fdh.o:  fdh.c $(HFILES)
	$(CC) $(CFLAGS) fdh.c -o fdh.o

cache.o:	cache.c $(HFILES)
	$(CC) $(CFLAGS) cache.c -o cache.o

filetime.o:	filetime.c $(HFILES)
	$(CC) $(CFLAGS) filetime.c -o filetime.o

summary.o:	summary.c $(HFILES)
	$(CC) $(CFLAGS) summary.c -o summary.o

dependency.o:	dependency.c $(HFILES)
	$(CC) $(CFLAGS) dependency.c -o dependency.o

header.o:	header.c $(HFILES)
	$(CC) $(CFLAGS) header.c -o header.o
