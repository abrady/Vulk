# this uses vcpkg to manage dependencies so don't forget to run vcpkg integrate install to get the path and build with it
# e.g. mkdir Build; cd Build; cmake -DCMAKE_TOOLCHAIN_FILE=<path_to>/buildsystems/vcpkg.cmake ..
# cmake -DCMAKE_BUILD_TYPE=Debug - for Debug Build

cmake_minimum_required (VERSION 3.8)
project ("VulkanRenderer")

# Check if vcpkg is bootstrapped
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
  message(STATUS "Bootstrapping vcpkg...")
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)
endif()

# Set the toolchain file
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

######
## project setup
######

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-W3)

######
## global includes
######

include_directories("${CMAKE_SOURCE_DIR}/Source")
include_directories("${CMAKE_SOURCE_DIR}/Source/ThirdParty/include")
include_directories("$ENV{VK_SDK_PATH}/include")

add_compile_options(-W3)

######
## subdirectories
######

add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Vulk")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples")

# Add a custom target that builds only the tests, so we can build them all at once in our github workflow
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


# add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/vertfrag)
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/PipelineBuilder)

# TODO: configure so it will work for release builds too
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TARGET_DIR "${CMAKE_BINARY_DIR}/Debug") 
    set(GLSLC_FLAGS "-g" "-O0" "-I${CMAKE_SOURCE_DIR}/Source/Shaders/Common") # build debug symbols and turn off optimizations
endif()

##############################################################################
## Assets
##############################################################################

function(copy_assets DIR_NAME)
    set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/${DIR_NAME}")
    set(DEST_DIR "${TARGET_DIR}/Assets/${DIR_NAME}") 

    file(MAKE_DIRECTORY ${DEST_DIR})
    file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "${SOURCE_DIR}/*")
    set(COMPILED_ASSETS_${DIR_NAME} "") # Clear the list before use
    foreach(ASSET_FILE ${ASSETS})
        file(RELATIVE_PATH REL_PATH "${SOURCE_DIR}" "${ASSET_FILE}")
        set(DEST "${DEST_DIR}/${REL_PATH}")
        get_filename_component(DEST_FILE_DIR "${DEST}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_FILE_DIR}")
        add_custom_command(
            OUTPUT "${DEST}"
            COMMAND ${CMAKE_COMMAND} -E copy "${ASSET_FILE}" "${DEST}"
            DEPENDS "${ASSET_FILE}"
            COMMENT "Copying ${DIR_NAME} ${ASSET_FILE} to ${DEST}"
        )
        list(APPEND COMPILED_ASSETS_${DIR_NAME} "${DEST}")
    endforeach()
    add_custom_target(
        compile_${DIR_NAME}
        ALL
        
        DEPENDS ${COMPILED_ASSETS_${DIR_NAME}}
    )
endfunction()

# these assets don't need any processing, but we need everything in the build
# directory to be usable by the executable
set(STRAIGHT_COPY_DIRECTORIES Pipelines Scenes Shaders Models Textures Materials)
foreach(DIR ${STRAIGHT_COPY_DIRECTORIES})
    copy_assets(${DIR})
endforeach()

##############################################################################
## Shaders
##############################################################################

# TODO: glslc has a -M option that will print a makefile dependency, so we could use that instead of globbing
file(GLOB SHADER_COMMON_FILES "${CMAKE_SOURCE_DIR}/Source/Shaders/Common/*")

# Set the source and destination directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/Shaders")
set(SHADER_DEST_DIR "${TARGET_DIR}/Source/Shaders") 

# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})

# Compile vertex shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Vert)
file(GLOB_RECURSE VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Vert/*")
foreach(SHADER ${VERTEX_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Vert/${SHADER_NAME}.vertspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building vertex shader ${SHADER_OUTPUT} from ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()
# Compile fragment shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Frag)
file(GLOB_RECURSE FRAGMENT_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Frag/*")
foreach(SHADER ${FRAGMENT_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Frag/${SHADER_NAME}.fragspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building fragment shader ${SHADER_OUTPUT} from ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Compile Geometry shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Tess)
file(GLOB_RECURSE GEOMETRY_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Geom/*")
foreach(SHADER ${GEOMETRY_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Geom/${SHADER_NAME}.geomspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building geometry shader ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    compile_shaders 
    DEPENDS ${COMPILED_SHADERS}
)
