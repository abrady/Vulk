cmake_minimum_required(VERSION 3.10)

project(PipelineBuilder)

find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)


add_executable(PipelineBuilder PipelineBuilder.cpp)
target_link_libraries(PipelineBuilder Vulk)

add_subdirectory(Tests)
