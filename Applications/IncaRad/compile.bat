@echo off

g++ -c -m64 -std=c++11 -O2 incarad_dll.cpp -fexceptions -fmax-errors=5
g++ -o incarad.dll -static -static-libgcc -static-libstdc++ -s -shared incarad_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32