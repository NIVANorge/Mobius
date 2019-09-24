#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 persist_dll.cpp
g++ -o persist.so -m64 -shared persist_dll.o