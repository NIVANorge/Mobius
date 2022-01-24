@echo off

g++ -c -m64 -std=c++11 -O2 incan_dll.cpp -fexceptions -fmax-errors=5
g++ -o incan.dll -s -static -static-libgcc -static-libstdc++ -shared incan_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32