@echo off

g++ test.cpp -std=c++11 -O2 -Werror=return-type -o test.exe -fmax-errors=5

REM g++ test.cpp -std=c++11 -ggdb -Werror=return-type -o test.exe -fmax-errors=5