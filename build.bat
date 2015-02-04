mkdir output
g++ rps.cpp -g -o output/rps.exe -D_WIN32_WINNT=0x0601 -I C:/local/boost_1_56_0/ -L C:/local/boost_1_56_0/bin.v2/libs/system/build/gcc-mingw-4.8.1/release/link-static/threading-multi/ -lboost_system-mgw48-mt-1_56 -L C:/local/boost_1_56_0/bin.v2/libs/program_options/build/gcc-mingw-4.8.1/release/link-static/threading-multi/ -lboost_program_options-mgw48-mt-1_56 -lws2_32 -static -lpdcurses -static-libgcc -static-libstdc++

