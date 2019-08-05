@echo off
g++ simplyQ_test.cpp -O2 -std=c++11 -fno-exceptions -o SimplyQ_test.exe -fmax-errors=5
REM g++ simplyQ_test.cpp -O0 -ggdb -std=c++11 -fno-exceptions -o SimplyQ_test.exe -fmax-errors=5