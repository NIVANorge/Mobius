@echo off

REM g++ -c -m64 -o2 sedflex_dll.cpp -DARMA_DONT_USE_WRAPPER
REM g++ -o sedflex.dll -L"lib_win64" -llibopenblas -s -static -static-libgcc -static-libstdc++ -shared sedflex_dll.o -Wl,--subsystem,windows

g++ sedflex_dll.cpp -m64 -o2 -DARMA_DONT_USE_WRAPPER -o sedflex.dll -L"lib_win64" -llibopenblas -s -static -static-libgcc -static-libstdc++ -shared -Wl,--subsystem,windows -luuid -lole32 -loleaut32