@echo off

g++ IncaNClassic.cpp -O2 -std=c++11 -fno-exceptions -Werror=return-type -o IncaNClassic.exe ../../sqlite3/sqlite3.o -fmax-errors=5