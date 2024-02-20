cmake_minimum_required(VERSION 3.10)

project(PipelineBuilder)


add_executable(PipelineBuilder PipelineBuilder.cpp)
add_subdirectory(Tests)
target_link_libraries(PipelineBuilder Vulk)
