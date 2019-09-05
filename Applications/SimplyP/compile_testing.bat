@echo off
REM g++ simplyp_testing.cpp -O2 -std=c++11 -ffast-math -funsafe-math-optimizations -msse4.2 -o SimplyP_testing.exe -fmax-errors=5
g++ simplyp_testing.cpp -O2 -std=c++11 -fno-exceptions -o SimplyP_testing.exe -fmax-errors=5