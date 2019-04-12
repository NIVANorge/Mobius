@echo off

g++ -c -m64 -std=c++11 -O2 hbv_wrapper.cpp -fexceptions -fmax-errors=5
g++ -o hbv.dll -s -shared hbv_wrapper.o -Wl,--subsystem,windows