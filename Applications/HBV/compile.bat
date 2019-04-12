@echo off

REM C:\Mingw\bin\g++ main.cpp -O2 -fno-exceptions  -ffast-math -Werror=return-type -o simplyp.exe -fmax-errors=5
g++ incaviewHBV.cpp -std=c++11 -O2 -Werror=return-type -o hbv.exe ../../sqlite3/sqlite3.o -fmax-errors=5