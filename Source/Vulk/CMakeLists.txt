cmake_minimum_required(VERSION 3.10)

######
## vcpkg
######

# Set the project name
project(Vulk)

# packages
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Add all the source files to a library
file(GLOB SOURCES "src/*.cpp")
add_library(Vulk ${SOURCES})
target_include_directories(Vulk PUBLIC include) # Make the include directory public

# Specify the include directories
target_include_directories(Vulk PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(Vulk PRIVATE glfw)
target_link_libraries(Vulk PRIVATE Vulkan::Vulkan)
target_link_libraries(Vulk PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Vulk PRIVATE glm::glm-header-only)
target_link_libraries(Vulk PRIVATE assimp::assimp)

if(MSVC)
  file(GLOB MY_PROJECT_NATVIS_FILES "${CMAKE_SOURCE_DIR}/Source/Tools/MSVC/Natvis/*.natvis")
  message("adding natvis files to project ${MY_PROJECT_NATVIS_FILES}")
  target_sources(Vulk PRIVATE ${MY_PROJECT_NATVIS_FILES})
  # /analyze - /analyze:warninglevel 4
  # /fsanitize=address /fsanitize=fuzzer
  # /sdl - Enables recommended Security Development Lifecycle (SDL) checks
  target_compile_options(Vulk PRIVATE /W4 /WX /sdl)
  add_compile_options(/fsanitize=address)
else()
  target_compile_options(Vulk PRIVATE -Wall -Wextra -pedantic -Werror)
  add_compile_options(-sanitize=address)
endif()
