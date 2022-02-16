cmake_minimum_required(VERSION 3.19)
project(gzstream)

set(CMAKE_CXX_STANDARD 14)

add_executable(sample sample.cpp)
target_link_libraries(sample z bz2)

add_executable(dc dc.cpp)
target_link_libraries(dc z bz2)