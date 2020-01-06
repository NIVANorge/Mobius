@echo off

g++ -c -m64 -std=c++11 -O2 SimplyC_structure_6.cpp -fexceptions -fmax-errors=5
g++ -o SimplyC_structure_6.dll -static -static-libgcc -static-libstdc++ -s -shared SimplyC_structure_6.o -Wl,--subsystem,windows