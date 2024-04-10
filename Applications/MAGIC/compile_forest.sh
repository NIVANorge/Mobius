#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 magic_forest_dll.cpp
g++ -o magic_forest.so -m64 -shared magic_forest.o