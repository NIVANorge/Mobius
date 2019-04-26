@echo off

REM g++ -O2 -I../../Calibration/MCMC/mcmc/include/ -I../../Calibration/MCMC/mcmc/src/ incan.cpp -m64 -std=c++11 -fopenmp -o incan.exe ../../sqlite3/sqlite3.o -L../../Calibration/MCMC/libs/ -llapack_win64_MT -lblas_win64_MT -DINCAVIEW_INCLUDE_OPTIMIZER=1 -DINCAVIEW_INCLUDE_GLUE=1 -DINCAVIEW_INCLUDE_MCMC=1

g++ IncaP.cpp -O2 -m64 -std=c++11 -Werror=return-type -o IncaP.exe ../../sqlite3/sqlite3.o -fmax-errors=5