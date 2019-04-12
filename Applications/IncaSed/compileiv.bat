@echo off

REM g++ -O2 -I../../Calibration/MCMC/mcmc/include/ -I../../Calibration/MCMC/mcmc/src/ incased.cpp -m64 -std=c++11 -fopenmp -o incaviewincased.exe ../../sqlite3/sqlite3.o -L../../Calibration/MCMC/libs/ -llapack_win64_MT -lblas_win64_MT -DINCAVIEW_INCLUDE_OPTIMIZER=1 -DINCAVIEW_INCLUDE_GLUE=1 -DINCAVIEW_INCLUDE_MCMC=1
g++ incased.cpp -std=c++11 -O2 -Werror=return-type -o incaviewincased.exe ../../sqlite3/sqlite3.o -fmax-errors=5