@echo off

g++ -c -m64 -std=c++11 -O2 simplyp_v04_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplyp_v04.dll -static -static-libgcc -static-libstdc++ -s -shared simplyp_v04_dll.o -Wl,--subsystem,windows