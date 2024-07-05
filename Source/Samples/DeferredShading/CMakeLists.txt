cmake_minimum_required(VERSION 3.10)

project(DeferredShading)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Add an executable
add_executable(DeferredShading deferredshading.cpp)
add_dependencies(DeferredShading Vulk GenSchemaFiles BuildAssetsTarget)

# Link the Vulk library to your executable
target_link_libraries(DeferredShading PRIVATE Vulk)
target_link_libraries(DeferredShading PRIVATE glfw)
target_link_libraries(DeferredShading PRIVATE imgui::imgui)
build_project(DeferredShading "deferredshading.proj")
