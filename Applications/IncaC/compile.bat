@echo off

REM g++ -O2 -I../../Calibration/MCMC/mcmc/include/ -I../../Calibration/MCMC/mcmc/src/ incac.cpp -m64 -std=c++11 -fopenmp -o incaviewincac.exe ../../sqlite3/sqlite3.o -L../../Calibration/MCMC/libs/ -llapack_win64_MT -lblas_win64_MT -DINCAVIEW_INCLUDE_OPTIMIZER=1 -DINCAVIEW_INCLUDE_GLUE=1 -DINCAVIEW_INCLUDE_MCMC=1
REM g++ incac.cpp -std=c++11 -O2 -Werror=return-type -o incac.exe ../../sqlite3/sqlite3.o -fmax-errors=5

g++ -c -m64 -std=c++11 -O2 incac_dll.cpp -fexceptions -fmax-errors=5
g++ -o incac.dll -s -shared incac_dll.o -Wl,--subsystem,windows