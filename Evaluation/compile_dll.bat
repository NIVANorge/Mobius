@echo off

g++ -c -m64 -std=c++11 -O2 simplyq_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplyq.dll -static -static-libgcc -static-libstdc++ -s -shared simplyq_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32