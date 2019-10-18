@echo off

g++ -c -m64 -std=c++11 -O2 magic_dll.cpp -fexceptions -fmax-errors=5
g++ -o magic.dll -s -static -static-libgcc -static-libstdc++ -shared magic_dll.o -Wl,--subsystem,windows