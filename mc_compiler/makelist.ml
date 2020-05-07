<DEFAULT
COMPILE=g++ -g -std=c++11 -c %SRCFILE% -Wno-unused-result -Wno-multichar -Wunused-variable -o %MODULE%.%OBJ_EXT%
LPREFIX=g++ -o %OUTPUT%
LSUFFIX=-lkaty
OBJ_EXT=o
OUTPUT=mc_compiler
NO_STATERR=1
#ARCH_AWARE=1

>>
main.cpp
../common/pp.cpp
compiler.cpp

../common/scanner.cpp
../common/tokenizer.cpp
../common/controls.cpp
../common/rpnparser.cpp
../common/alu.cpp
../common/hash.cpp
../common/exception.cpp
<<
