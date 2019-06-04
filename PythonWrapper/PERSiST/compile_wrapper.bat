@echo off

g++ -c -m64 -std=c++11 -O2 persist_wrapper.cpp -fexceptions -fmax-errors=5
g++ -o persist.dll -s -shared persist_wrapper.o -Wl,--subsystem,windows