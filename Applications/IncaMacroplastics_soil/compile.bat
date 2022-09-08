@echo off

g++ -c -m64 -std=c++11 -O2 incamacroplastics_soil_dll.cpp -fexceptions -fmax-errors=5
g++ -o incamacroplastics_soil.dll -static -static-libgcc -static-libstdc++ -s -shared incamacroplastics_soil_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32
