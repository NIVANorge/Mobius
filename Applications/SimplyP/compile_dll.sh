#!/bin/bash
g++ -fpic -m64 -std=c++11 -c -O2 simplyp_wrapper.cpp
g++ -o simplyp.so -m64 -shared simplyp_wrapper.o