@echo off

REM C:\Mingw\bin\g++ main.cpp -O2 -fno-exceptions  -ffast-math -Werror=return-type -o simplyp.exe -fmax-errors=5
REM g++ incaviewHBV.cpp -std=c++11 -O2 -Werror=return-type -o hbv.exe ../../sqlite3/sqlite3.o -fmax-errors=5

g++ -c -m64 -std=c++11 -O2 hbv_dll.cpp -fexceptions -fmax-errors=5
g++ -o hbv.dll -static -static-libgcc -static-libstdc++ -s -shared hbv_dll.o -Wl,--subsystem,windows