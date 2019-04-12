@echo off

g++ -c -m64 -std=c++11 -O2 SimplyC_wrapper.cpp -fexceptions -fmax-errors=5
g++ -o simplyc.dll -s -shared simplyc_wrapper.o -Wl,--subsystem,windows