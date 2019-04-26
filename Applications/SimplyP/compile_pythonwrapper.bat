@echo off

g++ -c -m64 -std=c++11 -O2 simplyp_wrapper.cpp -fexceptions -fmax-errors=5
g++ -o simplyp.dll -s -shared simplyp_wrapper.o -Wl,--subsystem,windows