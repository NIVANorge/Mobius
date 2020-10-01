@echo off

g++ -c -m64 -std=c++11 -O2 easylake_simplyq_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplyq_easylake.dll -s -static -static-libgcc -static-libstdc++ -shared easylake_simplyq_dll.o -Wl,--subsystem,windows