cmake_minimum_required(VERSION 3.10)

project(BuildTool)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Add an executable
file(GLOB SOURCES "*.cpp")
add_executable(BuildTool 
    ${SOURCES}
)
# add_dependencies(BuildTool CompileShadersTarget)

target_include_directories(BuildTool PRIVATE ${GENSCHEMAFILES_INCLUDE_DIRS})

target_link_libraries(BuildTool PRIVATE Vulk)
target_link_libraries(BuildTool PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(BuildTool PRIVATE spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util)
target_link_libraries(BuildTool PRIVATE CLI11::CLI11)
target_link_libraries(BuildTool PRIVATE spdlog::spdlog)

add_subdirectory(Tests)
