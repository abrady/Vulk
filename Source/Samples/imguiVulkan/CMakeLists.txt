cmake_minimum_required(VERSION 3.10)

project(ImGUIVulkan)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Add an executable
file(GLOB SOURCES "*.cpp")
add_executable(ImGUIVulkan ${SOURCES})
# Link the Vulk library to your executable
target_link_libraries(ImGUIVulkan PRIVATE Vulk)
target_link_libraries(ImGUIVulkan PRIVATE glfw)
target_link_libraries(ImGUIVulkan PRIVATE imgui::imgui)


vulk_add_config_json("${CMAKE_BINARY_DIR}/Source/Samples")
