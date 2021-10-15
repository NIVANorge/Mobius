#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 incamicroplastics_dll.cpp
g++ -o incamicroplastics.so -m64 -shared incamicroplastics_dll.o