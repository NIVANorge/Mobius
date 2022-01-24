@echo off

g++ -c -m64 -std=c++11 -O2 simplycnp_with_lake_dll.cpp -fexceptions -fmax-errors=5
g++ -o simplycnp_with_lake.dll -static -static-libgcc -static-libstdc++ -s -shared simplycnp_with_lake_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32