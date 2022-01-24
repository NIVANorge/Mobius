@echo off
g++ tutorial1.cpp -O2 -std=c++11 -fno-exceptions -o tutorial1.exe -fmax-errors=5 -luuid -lole32 -loleaut32