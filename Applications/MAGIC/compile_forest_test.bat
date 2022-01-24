@echo off

g++ forest_test.cpp -std=c++11 -O2 -Werror=return-type -o forest_test.exe -fmax-errors=5 -luuid -lole32 -loleaut32

REM g++ test.cpp -std=c++11 -ggdb -Werror=return-type -o test.exe -fmax-errors=5