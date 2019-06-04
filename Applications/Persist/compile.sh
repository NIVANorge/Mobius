#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 persist_wrapper.cpp
g++ -o persist.so -m64 -shared persist_wrapper.o