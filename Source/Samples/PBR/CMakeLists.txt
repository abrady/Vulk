cmake_minimum_required(VERSION 3.10)

project(PBRSample)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Add an executable
add_executable(PBRSample pbr.cpp)
# Link the Vulk library to your executable
target_link_libraries(PBRSample PRIVATE Vulk)
target_link_libraries(PBRSample PRIVATE glfw)
target_link_libraries(PBRSample PRIVATE imgui::imgui)

add_dependencies(PBRSample BuildAssetsTarget)