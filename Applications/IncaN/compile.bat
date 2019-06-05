@echo off

REM g++ -O2 -I../../Calibration/MCMC/mcmc/include/ -I../../Calibration/MCMC/mcmc/src/ incan.cpp -m64 -std=c++11 -fopenmp -o incan.exe ../../sqlite3/sqlite3.o -L../../Calibration/MCMC/libs/ -llapack_win64_MT -lblas_win64_MT -DINCAVIEW_INCLUDE_OPTIMIZER=1 -DINCAVIEW_INCLUDE_GLUE=1 -DINCAVIEW_INCLUDE_MCMC=1

REM g++ IncaN.cpp -O2 -m64 -std=c++11 -fno-exceptions -Werror=return-type -o IncaN.exe ../../sqlite3/sqlite3.o -fmax-errors=5

REM g++ IncaN.cpp -Og -m64 -std=c++11 -fno-exceptions -Werror=return-type -o IncaN.exe ../../sqlite3/sqlite3.o -fmax-errors=5

g++ -c -m64 -std=c++11 -O2 incan_dll.cpp -fexceptions -fmax-errors=5
g++ -o incan.dll -s -shared incan_dll.o -Wl,--subsystem,windows