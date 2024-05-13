cmake_minimum_required(VERSION 3.10)

project(VulkSamples)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Add an executable
file(GLOB SOURCES "*.cpp")
add_executable(VulkSamples ${SOURCES})
# Link the Vulk library to your executable
target_link_libraries(VulkSamples PRIVATE Vulk)
target_link_libraries(VulkSamples PRIVATE glfw)
target_link_libraries(VulkSamples PRIVATE imgui::imgui)

# Make your main target depend on the BuildPipelines target
add_dependencies(VulkSamples BuildPipelines)

vulk_add_config_json("${CMAKE_BINARY_DIR}/Source/Samples")
