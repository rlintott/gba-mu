cmake_minimum_required (VERSION 3.10)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(GBA)
INCLUDE(CTest)
add_subdirectory(src)
add_subdirectory(test)