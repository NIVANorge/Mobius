@echo off

g++ -c -m64 -std=c++11 -O2 sedflex_dll.cpp -fexceptions -fmax-errors=5
g++ -o sedflex.dll -s -static -static-libgcc -static-libstdc++ -shared sedflex_dll.o -Wl,--subsystem,windows