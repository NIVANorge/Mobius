#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 sedimentoxygen_dll.cpp
g++ -o sedimentoxygen.so -m64 -shared sedimentoxygen_dll.o