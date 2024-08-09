#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 incamacroplastics_soil_dll.cpp
g++ -o incamacroplastics_soil.so -m64 -shared incamacroplastics_soil_dll.o