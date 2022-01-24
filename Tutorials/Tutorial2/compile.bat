@echo off
g++ tutorial2.cpp -O2 -std=c++11 -fno-exceptions -o tutorial2.exe -fmax-errors=5 -luuid -lole32 -loleaut32