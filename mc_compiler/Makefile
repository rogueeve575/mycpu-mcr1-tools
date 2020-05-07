
all:	mc_compiler

mc_compiler:  main.o ../common/pp.o compiler.o ../common/scanner.o \
	 ../common/tokenizer.o ../common/controls.o ../common/rpnparser.o ../common/alu.o ../common/hash.o \
	 ../common/exception.o
	g++ -o mc_compiler \
	 main.o ../common/pp.o compiler.o ../common/scanner.o \
	 ../common/tokenizer.o ../common/controls.o ../common/rpnparser.o ../common/alu.o ../common/hash.o \
	 ../common/exception.o \
	 -lkaty

main.o:	main.cpp main.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h compiler.h
	g++ -g -std=c++11 -c main.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o main.o

../common/pp.o:	../common/pp.cpp ../common/pp.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/pp.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/pp.o

compiler.o:	compiler.cpp compiler.fdh main.h ../common/common.h \
		../common/hash.h ../common/scanner.h ../common/tokenizer.h \
		../common/controls.h ../common/alu.h ../common/pp.h \
		../common/rpnparser.h compiler.h
	g++ -g -std=c++11 -c compiler.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o compiler.o

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

../common/alu.o:	../common/alu.cpp ../common/alu.fdh ../common/common.h ../common/hash.h \
		../common/scanner.h ../common/tokenizer.h ../common/controls.h \
		../common/alu.h ../common/pp.h ../common/rpnparser.h
	g++ -g -std=c++11 -c ../common/alu.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/alu.o

../common/hash.o:	../common/hash.cpp ../common/hash.fdh ../common/hash.h
	g++ -g -std=c++11 -c ../common/hash.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/hash.o

../common/exception.o:	../common/exception.cpp ../common/exception.fdh ../common/exception.h
	g++ -g -std=c++11 -c ../common/exception.cpp -Wno-unused-result -Wno-multichar -Wunused-variable -o ../common/exception.o

.FORCE:

clean:
	rm -f main.o
	rm -f ../common/pp.o
	rm -f compiler.o
	rm -f ../common/scanner.o
	rm -f ../common/tokenizer.o
	rm -f ../common/controls.o
	rm -f ../common/rpnparser.o
	rm -f ../common/alu.o
	rm -f ../common/hash.o
	rm -f ../common/exception.o
	rm -f mc_compiler

cleanfdh:
	rm -f main.fdh
	rm -f ../common/pp.fdh
	rm -f compiler.fdh
	rm -f ../common/scanner.fdh
	rm -f ../common/tokenizer.fdh
	rm -f ../common/controls.fdh
	rm -f ../common/rpnparser.fdh
	rm -f ../common/alu.fdh
	rm -f ../common/hash.fdh
	rm -f ../common/exception.fdh

cleanall: clean cleanfdh

