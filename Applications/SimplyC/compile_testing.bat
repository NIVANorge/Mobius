@echo off
REM g++ SimplyC_testing.cpp -Og -ggdb -std=c++11 -fno-exceptions -o SimplyC_testing.exe -fmax-errors=5
g++ SimplyC_testing.cpp -O2 -std=c++11 -fno-exceptions -o SimplyC_testing.exe -fmax-errors=5