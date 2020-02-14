@echo off

g++ -c -m64 -std=c++11 -O2 shrimptox_dll.cpp -fexceptions -fmax-errors=5
g++ -o shrimptox.dll -static -static-libgcc -static-libstdc++ -s -shared shrimptox_dll.o -Wl,--subsystem,windows