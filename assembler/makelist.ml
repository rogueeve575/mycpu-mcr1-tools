<DEFAULT
COMPILE=g++ -g -std=c++11 -c %SRCFILE% -Wno-unused-result -Wno-multichar -Wunused-variable -o %MODULE%.%OBJ_EXT%
LPREFIX=g++ -o %OUTPUT%
LSUFFIX=-lkaty
OBJ_EXT=o
OUTPUT=assembler
NO_STATERR=1

>>
main.cpp
assembler.cpp

../common/asmcore.cpp
../common/scanner.cpp
../common/tokenizer.cpp
../common/rpnparser.cpp
../common/exception.cpp
../common/hash.cpp
../common/pp.cpp
<<