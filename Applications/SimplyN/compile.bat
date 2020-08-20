@echo off

g++ -c -m64 -std=c++11 -O2 simplyn_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplyn.dll -static -static-libgcc -static-libstdc++ -s -shared simplyn_dll.o -Wl,--subsystem,windows