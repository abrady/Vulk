cmake_minimum_required(VERSION 3.10)

project(PipelineBuilder)

include_directories("${CMAKE_SOURCE_DIR}/Source")

add_executable(PipelineBuilder PipelineBuilder.cpp)
add_subdirectory(Tests)