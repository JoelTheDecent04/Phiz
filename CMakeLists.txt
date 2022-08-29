cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_FLAGS "-O2")
set(CMAKE_EXE_LINKER_FLAGS "-s USE_SDL=2 -s USE_SDL_TTF=2 -s ALLOW_MEMORY_GROWTH=1 --preload-file resources/")
set(CMAKE_EXECUTABLE_SUFFIX .html)

add_executable(Physics main.cpp)

