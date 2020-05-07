
all:	a.out

a.out:  stat.o misc.o BList.o StringList.o \
	 qstree.o atree.o tcpstuff.o udpstuff.o DBuffer.o \
	 DString.o RingBuffer.o timer.o ini.o buildstamp.o \
	 email.o locker.o semaphore.o beep.o
	g++ \
	 stat.o misc.o BList.o StringList.o \
	 qstree.o atree.o tcpstuff.o udpstuff.o DBuffer.o \
	 DString.o RingBuffer.o timer.o ini.o buildstamp.o \
	 email.o locker.o semaphore.o beep.o \
	 -shared -Wl,-soname,libkaty.so.1 -o libkaty.so.1.0.0 -lrt -lpthread

stat.o:	stat.cpp stat.fdh stat.h timer.h \
		misc.h DString.h DBuffer.h
	g++ -fPIC -std=gnu++14 -g -O2 -c stat.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o stat.o

misc.o:	misc.cpp misc.fdh DString.h DBuffer.h \
		StringList.h BList.h stat.h \
		misc.h timer.h
	g++ -fPIC -std=gnu++14 -g -O2 -c misc.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o misc.o

BList.o:	BList.cpp BList.fdh BList.h
	g++ -fPIC -std=gnu++14 -g -O2 -c BList.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o BList.o

StringList.o:	StringList.cpp StringList.fdh StringList.h BList.h
	g++ -fPIC -std=gnu++14 -g -O2 -c StringList.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o StringList.o

qstree.o:	qstree.cpp qstree.fdh stat.h qstree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c qstree.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o qstree.o

atree.o:	atree.cpp atree.fdh atree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c atree.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o atree.o

tcpstuff.o:	tcpstuff.cpp tcpstuff.fdh timer.h stat.h \
		tcpstuff.h
	g++ -fPIC -std=gnu++14 -g -O2 -c tcpstuff.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o tcpstuff.o

udpstuff.o:	udpstuff.cpp udpstuff.fdh stat.h tcpstuff.h \
		udpstuff.h
	g++ -fPIC -std=gnu++14 -g -O2 -c udpstuff.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o udpstuff.o

DBuffer.o:	DBuffer.cpp DBuffer.fdh DBuffer.h stat.h
	g++ -fPIC -std=gnu++14 -g -O2 -c DBuffer.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o DBuffer.o

DString.o:	DString.cpp DString.fdh katy.h misc.h \
		stat.h timer.h atree.h \
		qstree.h BList.h StringList.h \
		DBuffer.h DString.h RingBuffer.h \
		tcpstuff.h udpstuff.h ini.h \
		email.h llist.h locker.h \
		semaphore.h fifo.h autofree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c DString.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o DString.o

RingBuffer.o:	RingBuffer.cpp RingBuffer.fdh RingBuffer.h
	g++ -fPIC -std=gnu++14 -g -O2 -c RingBuffer.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o RingBuffer.o

timer.o:	timer.cpp timer.fdh timer.h misc.h \
		DString.h DBuffer.h
	g++ -fPIC -std=gnu++14 -g -O2 -c timer.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o timer.o

ini.o:	ini.cpp ini.fdh katy.h misc.h \
		stat.h timer.h atree.h \
		qstree.h BList.h StringList.h \
		DBuffer.h DString.h RingBuffer.h \
		tcpstuff.h udpstuff.h ini.h \
		email.h llist.h locker.h \
		semaphore.h fifo.h autofree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c ini.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o ini.o

buildstamp.o:	buildstamp.cpp buildstamp.fdh
	g++ -fPIC -std=gnu++14 -g -O2 -c buildstamp.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o buildstamp.o

email.o:	email.cpp email.fdh katy.h misc.h \
		stat.h timer.h atree.h \
		qstree.h BList.h StringList.h \
		DBuffer.h DString.h RingBuffer.h \
		tcpstuff.h udpstuff.h ini.h \
		email.h llist.h locker.h \
		semaphore.h fifo.h autofree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c email.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o email.o

locker.o:	locker.cpp locker.fdh locker.h
	g++ -fPIC -std=gnu++14 -g -O2 -c locker.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o locker.o

semaphore.o:	semaphore.cpp semaphore.fdh katy.h misc.h \
		stat.h timer.h atree.h \
		qstree.h BList.h StringList.h \
		DBuffer.h DString.h RingBuffer.h \
		tcpstuff.h udpstuff.h ini.h \
		email.h llist.h locker.h \
		semaphore.h fifo.h autofree.h
	g++ -fPIC -std=gnu++14 -g -O2 -c semaphore.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o semaphore.o

beep.o:	beep.cpp beep.fdh locker.h misc.h \
		beepdata.inc
	g++ -fPIC -std=gnu++14 -g -O2 -c beep.cpp -Wno-multichar -Wunused-variable -Wno-unused-result -o beep.o

.FORCE:

clean:
	rm -f stat.o
	rm -f misc.o
	rm -f BList.o
	rm -f StringList.o
	rm -f qstree.o
	rm -f atree.o
	rm -f tcpstuff.o
	rm -f udpstuff.o
	rm -f DBuffer.o
	rm -f DString.o
	rm -f RingBuffer.o
	rm -f timer.o
	rm -f ini.o
	rm -f buildstamp.o
	rm -f email.o
	rm -f locker.o
	rm -f semaphore.o
	rm -f beep.o
	rm -f a.out

cleanfdh:
	rm -f stat.fdh
	rm -f misc.fdh
	rm -f BList.fdh
	rm -f StringList.fdh
	rm -f qstree.fdh
	rm -f atree.fdh
	rm -f tcpstuff.fdh
	rm -f udpstuff.fdh
	rm -f DBuffer.fdh
	rm -f DString.fdh
	rm -f RingBuffer.fdh
	rm -f timer.fdh
	rm -f ini.fdh
	rm -f buildstamp.fdh
	rm -f email.fdh
	rm -f locker.fdh
	rm -f semaphore.fdh
	rm -f beep.fdh

cleanall: clean cleanfdh

