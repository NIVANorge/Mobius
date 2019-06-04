@echo off
g++ -c -m64 -std=c++11 -O2 tutorial3_dll.cpp -fexceptions -fmax-errors=5
g++ -o tutorial3.dll -s -shared tutorial3_dll.o -Wl,--subsystem,windows