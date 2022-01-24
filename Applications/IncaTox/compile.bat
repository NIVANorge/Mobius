@echo off

g++ -c -m64 -std=c++11 -O2 incatox_dll.cpp -fexceptions -fmax-errors=5
g++ -o incatox.dll -static -static-libgcc -static-libstdc++ -s -shared incatox_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32