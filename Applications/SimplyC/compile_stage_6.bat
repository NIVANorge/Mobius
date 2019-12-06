@echo off

g++ -c -m64 -std=c++11 -O2 SimplyC_stage_6.cpp -fexceptions -fmax-errors=5
g++ -o SimplyC_stage_6.dll -static -static-libgcc -static-libstdc++ -s -shared SimplyC_stage_6.o -Wl,--subsystem,windows