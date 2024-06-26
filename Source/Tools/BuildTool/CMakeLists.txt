cmake_minimum_required(VERSION 3.10)

project(BuildTool)

find_package(Boost REQUIRED COMPONENTS process)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_c CONFIG REQUIRED)

find_package(Catch2 CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Add an executable
message("BuildTool: CMAKE_MSVC_RUNTIME_LIBRARY is ${CMAKE_MSVC_RUNTIME_LIBRARY}")
add_executable(BuildTool BuildTool.cpp BuildProject.cpp)

target_include_directories(BuildTool PRIVATE ${GENSCHEMAFILES_INCLUDE_DIRS})

target_link_libraries(BuildTool PRIVATE Boost::process)
target_link_libraries(BuildTool PRIVATE Vulk)
target_link_libraries(BuildTool PRIVATE)
target_link_libraries(BuildTool PRIVATE CLI11::CLI11)
target_link_libraries(BuildTool PRIVATE spdlog::spdlog)

target_link_libraries(BuildTool PRIVATE spirv-cross-c)
target_link_libraries(BuildTool PRIVATE spirv-cross-core)
target_link_libraries(BuildTool PRIVATE spirv-cross-cpp)
target_link_libraries(BuildTool PRIVATE spirv-cross-reflect)
target_link_libraries(BuildTool PRIVATE spirv-cross-util)
target_link_libraries(BuildTool PRIVATE spirv-cross-glsl)


add_subdirectory(Tests)
