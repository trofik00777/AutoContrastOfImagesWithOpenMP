cmake_minimum_required(VERSION 3.21)
project(AutoContrast)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "-fopenmp -O3 -mfma")

add_executable(AutoContrast main.cpp)
