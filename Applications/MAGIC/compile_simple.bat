@echo off

g++ -c -m64 -std=c++11 -O2 magic_simple_dll.cpp -fexceptions -fmax-errors=5
g++ -o magic_simple.dll -s -static -static-libgcc -static-libstdc++ -shared magic_simple_dll.o -Wl,--subsystem,windows