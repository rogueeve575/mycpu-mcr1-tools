
all:	emulator

emulator:  main.o emulator.o scrollwindow.o asmscrollwindow.o \
	 curses.o cstat.o ../common/alu.o ../common/scanner.o ../common/tokenizer.o \
	 ../common/controls.o ../common/rpnparser.o ../common/asmcore.o ../common/hash.o ../common/exception.o
	g++ -o emulator \
	 main.o emulator.o scrollwindow.o asmscrollwindow.o \
	 curses.o cstat.o ../common/alu.o ../common/scanner.o ../common/tokenizer.o \
	 ../common/controls.o ../common/rpnparser.o ../common/asmcore.o ../common/hash.o ../common/exception.o \
	 -lkaty -lncurses

main.o:	main.cpp main.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h curses.h \
		asmscrollwindow.h emulator.h scrollwindow.h
	g++ -g -std=c++11 -c main.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o main.o

emulator.o:	emulator.cpp emulator.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h curses.h \
		asmscrollwindow.h emulator.h scrollwindow.h
	g++ -g -std=c++11 -c emulator.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o emulator.o

scrollwindow.o:	scrollwindow.cpp scrollwindow.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h curses.h \
		asmscrollwindow.h emulator.h scrollwindow.h
	g++ -g -std=c++11 -c scrollwindow.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o scrollwindow.o

asmscrollwindow.o:	asmscrollwindow.cpp asmscrollwindow.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h curses.h \
		asmscrollwindow.h emulator.h scrollwindow.h
	g++ -g -std=c++11 -c asmscrollwindow.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o asmscrollwindow.o

curses.o:	curses.cpp curses.fdh curses.h
	g++ -g -std=c++11 -c curses.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o curses.o

cstat.o:	cstat.cpp cstat.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h curses.h \
		asmscrollwindow.h emulator.h scrollwindow.h
	g++ -g -std=c++11 -c cstat.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o cstat.o

../common/alu.o:	../common/alu.cpp ../common/alu.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/alu.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/alu.o

../common/scanner.o:	../common/scanner.cpp ../common/scanner.fdh ../common/scanner.h
	g++ -g -std=c++11 -c ../common/scanner.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/scanner.o

../common/tokenizer.o:	../common/tokenizer.cpp ../common/tokenizer.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/tokenizer.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/tokenizer.o

../common/controls.o:	../common/controls.cpp ../common/controls.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/controls.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/controls.o

../common/rpnparser.o:	../common/rpnparser.cpp ../common/rpnparser.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h \
		../common/exception.h
	g++ -g -std=c++11 -c ../common/rpnparser.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/rpnparser.o

../common/asmcore.o:	../common/asmcore.cpp ../common/asmcore.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h \
		../common/asmcore.h
	g++ -g -std=c++11 -c ../common/asmcore.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/asmcore.o

../common/hash.o:	../common/hash.cpp ../common/hash.fdh ../common/hash.h
	g++ -g -std=c++11 -c ../common/hash.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/hash.o

../common/exception.o:	../common/exception.cpp ../common/exception.fdh ../common/exception.h
	g++ -g -std=c++11 -c ../common/exception.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/exception.o

.FORCE:

clean:
	rm -f main.o
	rm -f emulator.o
	rm -f scrollwindow.o
	rm -f asmscrollwindow.o
	rm -f curses.o
	rm -f cstat.o
	rm -f ../common/alu.o
	rm -f ../common/scanner.o
	rm -f ../common/tokenizer.o
	rm -f ../common/controls.o
	rm -f ../common/rpnparser.o
	rm -f ../common/asmcore.o
	rm -f ../common/hash.o
	rm -f ../common/exception.o
	rm -f emulator

cleanfdh:
	rm -f main.fdh
	rm -f emulator.fdh
	rm -f scrollwindow.fdh
	rm -f asmscrollwindow.fdh
	rm -f curses.fdh
	rm -f cstat.fdh
	rm -f ../common/alu.fdh
	rm -f ../common/scanner.fdh
	rm -f ../common/tokenizer.fdh
	rm -f ../common/controls.fdh
	rm -f ../common/rpnparser.fdh
	rm -f ../common/asmcore.fdh
	rm -f ../common/hash.fdh
	rm -f ../common/exception.fdh

cleanall: clean cleanfdh

