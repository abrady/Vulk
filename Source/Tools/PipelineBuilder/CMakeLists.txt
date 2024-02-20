cmake_minimum_required(VERSION 3.10)

project(PipelineBuilder)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Add an executable
file(GLOB SOURCES "*.cpp")
add_executable(PipelineBuilder ${SOURCES})
add_dependencies(PipelineBuilder compile_shaders build_assets)

target_link_libraries(PipelineBuilder PRIVATE Vulk)
target_link_libraries(PipelineBuilder PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(PipelineBuilder PRIVATE spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util)

add_subdirectory(Tests)
