cmake_minimum_required(VERSION 3.10)

project(Cubemap)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Add an executable
add_executable(Cubemap cubemap.cpp)
# Link the Vulk library to your executable
target_link_libraries(Cubemap PRIVATE Vulk)
target_link_libraries(Cubemap PRIVATE glfw)
target_link_libraries(Cubemap PRIVATE imgui::imgui)
build_project(Cubemap "cubemap.proj")
