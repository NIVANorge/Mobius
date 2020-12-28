@echo off

g++ -c -m64 -std=c++11 -O2 incamacroplastics_dll.cpp -fexceptions -fmax-errors=5
g++ -o incamacroplastics.dll -static -static-libgcc -static-libstdc++ -s -shared incamacroplastics_dll.o -Wl,--subsystem,windows