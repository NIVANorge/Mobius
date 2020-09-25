@echo off

g++ -c -m64 -std=c++11 -O2 simplycnp_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplycnp.dll -static -static-libgcc -static-libstdc++ -s -shared simplycnp_dll.o -Wl,--subsystem,windows