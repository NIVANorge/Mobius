@echo off

g++ -c -m64 -std=c++11 -O2 edna.cpp -fexceptions -fmax-errors=5
g++ -o edna.dll -static -static-libgcc -static-libstdc++ -s -shared edna.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32