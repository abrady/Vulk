cmake_minimum_required(VERSION 3.10)

set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(vertfrag)

add_executable(vertfrag vertfrag.cpp)