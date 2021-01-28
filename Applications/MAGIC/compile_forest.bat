@echo off

g++ -c -m64 -std=c++11 -O2 magic_forest_dll.cpp -fexceptions -fmax-errors=5
g++ -o magic_forest.dll -s -static -static-libgcc -static-libstdc++ -shared magic_forest_dll.o -Wl,--subsystem,windows