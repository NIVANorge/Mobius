@echo off

g++ -c -m64 -std=c++11 -O2 incatox_dll_lake.cpp -fexceptions -fmax-errors=5
g++ -o incatox_lake.dll -static -static-libgcc -static-libstdc++ -s -shared incatox_dll_lake.o -Wl,--subsystem,windows