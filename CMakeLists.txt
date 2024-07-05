cmake_minimum_required (VERSION 3.13)
cmake_policy(SET CMP0091 NEW) # enable MSVC_RUNTIME_LIBRARY

project ("VulkanRoot" CXX)

######
## project setup
######

# note: don't forget the kits for this come from C:\Users\abrady\AppData\Local\CMakeTools\cmake-tools-kits.json

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# performance optimization, but can mask some errors
# see https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html
# didn't actually save time, so keeping it off
# set(CMAKE_UNITY_BUILD ON)

# this basically did nothing to build times, but keeping it
if(MSVC)
    message("Using MSVC")
    #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # /MDd or /MD
    #message("CMAKE_MSVC_RUNTIME_LIBRARY: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
    #et(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL")
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL")
    #set(CMAKE_GENERATOR_PLATFORM x64)
    add_compile_options(/W4 /MP /WX /sdl)
    add_compile_options(/EHa) # enable exception handling - this will allow SEH to be caught by catch(...) blocks
    add_definitions(-DGLOG_USE_GLOG_EXPORT) # glog errors without this, but some sub vcproject is using it so blarg.
    # we can't do any of this as it causes linker problems :(
    # add_compile_options(/fsanitize=address)
    # add_compile_options(/fsanitize=fuzzer)
    # add_compile_options(/analyze) # Static analysis
    # add_compile_options(/EHa) # enable exception handling - this will allow SEH to be caught by catch(...) blocks

    # # for release builds: 
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Zi /Zo")
else()
    message("Using Clang")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")

    # https://nullprogram.com/blog/2023/04/29/ - flags from here
    add_compile_options(
        -Wall -Wextra # all doesn't actually do all, 
        -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-private-field
        -Wdouble-promotion
        -Wconversion -Wno-sign-conversion # warns about implicit conversions that may result in data loss. Sign conversions do not have data loss, the implicit conversions are useful, and in my experience they’re not a source of defects
        -Werror

        #-fsanitize-trap=undefined # just halt on crash, no runtime checker
        #-fsanitize=address
        #-fsanitize=undefined

        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2

        # assume debug mode for now
        -O0
        -g3
        -DDEBUG

        # somehow some define for min and max is being set, we don't want it
        -DNOMINMAX
    )
endif()

# message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
# message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

######
## global defines
######

set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Resources") 
set(GENERATED_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/generated") 

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
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
# Ensure the file is empty before we start writing
set(ASSETS_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets.txt")
file(WRITE ${ASSETS_OUTPUT_FILE} "")

set(SAMPLES_BUILD_DIR_ROOT "${CMAKE_CURRENT_BINARY_DIR}/Source/Samples")

# Define a function that takes a target name and calls an external build tool.
function(build_project target_name project_filename)
  # Ensure the target exists.
  if(NOT TARGET ${target_name})
    message(FATAL_ERROR "Specified target does not exist: ${target_name}")
  endif()

  # Use add_custom_command to invoke the external tool during the build phase.
  # the TARGET ... POST_BUILD is necessary because things like $<TARGET_FILE_DIR:${target_name}> don't work 
  # until the target is built
  add_custom_command(
    TARGET ${target_name} POST_BUILD
    COMMAND $<TARGET_FILE:BuildTool> project $<TARGET_PROPERTY:${target_name},SOURCE_DIR>/${project_filename} $<TARGET_FILE_DIR:${target_name}> 
    COMMENT "build_project: $<TARGET_FILE:BuildTool> project $<TARGET_PROPERTY:${target_name},SOURCE_DIR>/${project_filename} $<TARGET_FILE_DIR:${target_name}> "
  )
endfunction()


######
## testing
######

include(CTest)

# this has to be before we include any subdirectories
enable_testing()

######
## subdirectory CMakeLists.txt
######

add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Schemas)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Vulk)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Shaders)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/BuildTool)
add_subdirectory(${CMAKE_SOURCE_DIR}/Assets)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Samples)

add_dependencies(Vulk GenSchemaFiles)
add_dependencies(BuildTool GenSchemaFiles)
#add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Editor)




