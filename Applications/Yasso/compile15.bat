@echo off

REM g++ -c -m64 -std=c++11 -O2 yasso15_dll.cpp -fexceptions -fmax-errors=5
REM g++ -o yasso15.dll -s -static -static-libgcc -static-libstdc++ -shared yasso15_dll.o -Wl,--subsystem,windows -luuid -lole32 -loleaut32


g++ yasso15_dll.cpp -m64 -o2 -DARMA_DONT_USE_WRAPPER -o yasso15.dll -L"lib_win64" -llibopenblas -s -static -static-libgcc -static-libstdc++ -shared -Wl,--subsystem,windows -luuid -lole32 -loleaut32