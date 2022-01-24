@echo off

g++ -c -m64 -std=c++11 -O2 sedimentoxygen_dll.cpp -fexceptions -fmax-errors=5
g++ -o sedimentoxygen.dll -s -static -static-libgcc -static-libstdc++ -shared sedimentoxygen_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32