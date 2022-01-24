@echo off

g++ -c -m64 -std=c++11 -O2 easylake_dll.cpp -fexceptions -fmax-errors=5
g++ -o easylake.dll -s -static -static-libgcc -static-libstdc++ -shared easylake_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32