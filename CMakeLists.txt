# The only prerequisite here is that cmake is installed and in your path to run this
# it also uses vcpkg to manage dependencies, but tries to bootstrap this from within the repo
cmake_minimum_required (VERSION 3.13)

project ("VulkanRoot" CXX)

######
## project setup
######

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_CXX_STANDARD 23)
    add_compile_options(/W4 /MP /WX)
    add_compile_options(/EHa) # enable exception handling - this will allow SEH to be caught by catch(...) blocks
else()
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_OSX_ARCHITECTURES arm64)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_definitions(LIBCXX_ENABLE_INCOMPLETE_FEATURES)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED) 


######
## global includes
######

#include_directories("${CMAKE_SOURCE_DIR}/Source")
#include_directories("${CMAKE_SOURCE_DIR}/Source/ThirdParty/include")
#include_directories("$ENV{VK_SDK_PATH}/include")

######
## global defines
######

# TODO: configure so it will work for release builds too

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Resources") 
set(GENERATED_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/generated") 

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Release") 
else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/Shaders")
set(SHADER_DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Source/Shaders") 

######
## configure directory structure, config files, etc
######

# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})

# make a configuration file for so the resource directory BUILT_RESOURCES_TARGET_DIR is known at runtime
function(vulk_add_config_json target_dir)
    message("Copying config.json to ${target_dir}")
    configure_file(${CMAKE_SOURCE_DIR}/config.json.in ${target_dir}/config.json)
endfunction()
vulk_add_config_json("${CMAKE_BINARY_DIR}/Source/Samples")

# File to write - mostly for debug. this is used in Assets/ and Source/Shaders atm
set(ASSETS_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets.txt")

# Ensure the file is empty before we start writing
file(WRITE ${ASSETS_OUTPUT_FILE} "")

######
## testing
######

# Add a custom target that builds only the tests, so we can build them all at once in our github workflow
# NOTE: used by subprojects to add their tests to the custom target
add_custom_target(build_tests)

# this has to be before we include any subdirectories
enable_testing()


# Define a function to add tests and accumulate them into a custom target so we can
# build them all at once in our github workflow
function(vulk_add_and_remember_test target)
  # Register the test with CTest
  add_test(NAME ${target} COMMAND ${target})
  # Add the test to the custom target
  add_dependencies(build_tests ${target})
endfunction()

######
## subdirectory CMakeLists.txt
######

add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Schemas")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Vulk")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples/imguiVulkan")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples/PBR")
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/BuildTool)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Shaders)
add_subdirectory(${CMAKE_SOURCE_DIR}/Assets)

add_dependencies(Vulk GenSchemaFiles)
add_dependencies(BuildTool GenSchemaFiles)
#add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Editor)




