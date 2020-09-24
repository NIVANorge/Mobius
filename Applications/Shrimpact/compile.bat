@echo off

g++ -c -m64 -std=c++11 -O2 shrimpact_dll.cpp -fexceptions -fmax-errors=5
g++ -o shrimpact.dll -static -static-libgcc -static-libstdc++ -s -shared shrimpact_dll.o -Wl,--subsystem,windows