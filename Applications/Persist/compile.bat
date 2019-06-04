@echo off

g++ -c -m64 -std=c++11 -O2 persist_dll.cpp -fexceptions -fmax-errors=5
g++ -o persist.dll -s -shared persist_dll.o -Wl,--subsystem,windows