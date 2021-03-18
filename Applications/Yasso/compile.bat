@echo off

g++ -c -m64 -std=c++11 -O2 yasso_dll.cpp -fexceptions -fmax-errors=5
g++ -o yasso.dll -s -static -static-libgcc -static-libstdc++ -shared yasso_dll.o -Wl,--subsystem,windows