cmake_minimum_required(VERSION 3.19)
project(gzstream)

set(CMAKE_CXX_STANDARD 14)

add_executable(gzstream main.cpp gzstream.cpp)
target_link_libraries(gzstream z)

add_executable(zip zip.cpp gzstream.cpp)
target_link_libraries(zip z)

add_executable(unzip unzip.cpp gzstream.cpp)
target_link_libraries(unzip z)