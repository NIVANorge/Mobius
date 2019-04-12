@echo off
REM g++ -O2 -I../../Calibration/MCMC/mcmc/include/ -I../../Calibration/MCMC/mcmc/src/ incaviewsimplyp.cpp -m64 -std=c++11 -fopenmp -o incaviewsimplyp.exe ../../sqlite3/sqlite3.o -L../../Calibration/MCMC/libs/ -llapack_win64_MT -lblas_win64_MT
REM g++ incaviewsimplyp.cpp -O2 -std=c++11 -o incaviewsimplyp.exe ../../sqlite3/sqlite3.o -fmax-errors=5 -fopenmp
g++ incaviewSimplyC.cpp -O2 -std=c++11 -o incaview_SimplyC.exe ../../sqlite3/sqlite3.o -fmax-errors=5