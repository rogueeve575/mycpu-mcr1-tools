<DEFAULT
COMPILE=g++ -g -std=c++11 -c %SRCFILE% -Wno-unused-result -Wno-multichar -Wunused-variable -o %MODULE%.%OBJ_EXT%
LPREFIX=g++ -o %OUTPUT%
LSUFFIX=-lkaty -lncurses
OBJ_EXT=o
OUTPUT=emulator
NO_STATERR=1

>>
main.cpp
emulator.cpp
scrollwindow.cpp
asmscrollwindow.cpp
curses.cpp
cstat.cpp

../common/alu.cpp
../common/scanner.cpp
../common/tokenizer.cpp
../common/controls.cpp
../common/rpnparser.cpp
../common/asmcore.cpp
../common/hash.cpp
../common/exception.cpp
<<
