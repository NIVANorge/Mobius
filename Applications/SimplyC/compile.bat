@echo off

g++ -c -m64 -std=c++11 -O2 SimplyC_dll.cpp -fexceptions -fmax-errors=5
g++ -o SimplyC.dll -s -shared SimplyC_dll.o -Wl,--subsystem,windows