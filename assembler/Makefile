
all:	assembler

assembler:  main.o assembler.o ../common/asmcore.o ../common/scanner.o \
	 ../common/tokenizer.o ../common/rpnparser.o ../common/exception.o ../common/hash.o ../common/pp.o
	g++ -o assembler \
	 main.o assembler.o ../common/asmcore.o ../common/scanner.o \
	 ../common/tokenizer.o ../common/rpnparser.o ../common/exception.o ../common/hash.o ../common/pp.o \
	 -lkaty

main.o:	main.cpp main.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h assembler.h
	g++ -g -std=c++11 -c main.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o main.o

assembler.o:	assembler.cpp assembler.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h ../common/asmcore.h assembler.h
	g++ -g -std=c++11 -c assembler.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o assembler.o

../common/asmcore.o:	../common/asmcore.cpp ../common/asmcore.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h \
		../common/asmcore.h
	g++ -g -std=c++11 -c ../common/asmcore.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/asmcore.o

../common/scanner.o:	../common/scanner.cpp ../common/scanner.fdh ../common/scanner.h
	g++ -g -std=c++11 -c ../common/scanner.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/scanner.o

../common/tokenizer.o:	../common/tokenizer.cpp ../common/tokenizer.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/tokenizer.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/tokenizer.o

../common/rpnparser.o:	../common/rpnparser.cpp ../common/rpnparser.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h \
		../common/exception.h
	g++ -g -std=c++11 -c ../common/rpnparser.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/rpnparser.o

../common/exception.o:	../common/exception.cpp ../common/exception.fdh ../common/exception.h
	g++ -g -std=c++11 -c ../common/exception.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/exception.o

../common/hash.o:	../common/hash.cpp ../common/hash.fdh ../common/hash.h
	g++ -g -std=c++11 -c ../common/hash.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/hash.o

../common/pp.o:	../common/pp.cpp ../common/pp.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/pp.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/pp.o

.FORCE:

clean:
	rm -f main.o
	rm -f assembler.o
	rm -f ../common/asmcore.o
	rm -f ../common/scanner.o
	rm -f ../common/tokenizer.o
	rm -f ../common/rpnparser.o
	rm -f ../common/exception.o
	rm -f ../common/hash.o
	rm -f ../common/pp.o
	rm -f assembler

cleanfdh:
	rm -f main.fdh
	rm -f assembler.fdh
	rm -f ../common/asmcore.fdh
	rm -f ../common/scanner.fdh
	rm -f ../common/tokenizer.fdh
	rm -f ../common/rpnparser.fdh
	rm -f ../common/exception.fdh
	rm -f ../common/hash.fdh
	rm -f ../common/pp.fdh

cleanall: clean cleanfdh

