# The only prerequisite here is that cmake is installed and in your path to run this
# it also uses vcpkg to manage dependencies, but tries to bootstrap this from within the repo
cmake_minimum_required (VERSION 3.13)
project ("VulkanRoot" CXX)

# Check if vcpkg is bootstrapped
find_program(VCPKG_EXECUTABLE NAMES vcpkg PATHS "${CMAKE_SOURCE_DIR}/vcpkg" NO_DEFAULT_PATH)
if(NOT VCPKG_EXECUTABLE)
  message(STATUS "Bootstrapping vcpkg...")
  execute_process(COMMAND git submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)
  # Find vcpkg executable again after bootstrapping
  find_program(VCPKG_EXECUTABLE NAMES vcpkg PATHS "${CMAKE_SOURCE_DIR}/vcpkg" NO_DEFAULT_PATH)
endif()
if(VCPKG_EXECUTABLE)
  execute_process(COMMAND ${VCPKG_EXECUTABLE} integrate install
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)
else()
  message(FATAL_ERROR "Could not find vcpkg executable")
endif()

# Set the toolchain file
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")


find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)


######
## project setup
######

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_STANDARD 23)
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
## global defines
######

# TODO: configure so it will work for release builds too

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
set(GLSLC_FLAGS "-g" "--target-env=vulkan1.3" "-I${CMAKE_SOURCE_DIR}/Source/Shaders/Common") # build debug symbols and turn off optimizations
set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Resources") 
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GLSLC_FLAGS "-O0")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # set(BUILT_RESOURCES_TARGET_DIR "${CMAKE_BINARY_DIR}/Release") 
else()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()


######
## subdirectories
######
# Add a custom target that builds only the tests, so we can build them all at once in our github workflow
# NOTE: used by subprojects to add their tests to the custom target
add_custom_target(build_tests)

# this has to be before we include any subdirectories
enable_testing()

function(vulk_add_config_json target_dir)
    message("Copying config.json to ${target_dir}")
    configure_file(${CMAKE_SOURCE_DIR}/config.json.in ${target_dir}/config.json)
endfunction()

# Define a function to add tests and accumulate them into a custom target so we can
# build them all at once in our github workflow
function(vulk_add_and_remember_test target)
  # Register the test with CTest
  add_test(NAME ${target} COMMAND ${target})
  # Add the test to the custom target
  add_dependencies(build_tests ${target})
endfunction()

add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Vulk")
add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples")
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/PipelineBuilder)

##############################################################################
## Shaders
##############################################################################

# TODO: glslc has a -M option that will print a makefile dependency, so we could use that instead of globbing
file(GLOB SHADER_COMMON_FILES "${CMAKE_SOURCE_DIR}/Source/Shaders/Common/*")

# Set the source and destination directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/Shaders")
set(SHADER_DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Source/Shaders") 

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
        COMMENT "building vertex shader ${SHADER_OUTPUT} from ${SHADER} flags ${GLSLC_FLAGS}"
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
        COMMENT "building fragment shader ${SHADER_OUTPUT} from ${SHADER} flags ${GLSLC_FLAGS}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Compile Geometry shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Geom)
file(GLOB_RECURSE GEOMETRY_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Geom/*")
foreach(SHADER ${GEOMETRY_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Geom/${SHADER_NAME}.geomspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building geometry shader ${SHADER} from ${SHADER} flags ${GLSLC_FLAGS}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    compile_shaders 
    DEPENDS ${COMPILED_SHADERS}
)


##############################################################################
## Assets
##############################################################################

function(copy_assets DIR_NAME GLOBAL_ASSETS_VAR)
    set(ASSETS_TO_BUILD ${${GLOBAL_ASSETS_VAR}}) # get the list of assets from parent scope
    set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/${DIR_NAME}")
    set(DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/${DIR_NAME}") 

    file(MAKE_DIRECTORY ${DEST_DIR})
    file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "${SOURCE_DIR}/*")
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
        list(APPEND ASSETS_TO_BUILD "${DEST}")
    endforeach()
    set(${GLOBAL_ASSETS_VAR} "${ASSETS_TO_BUILD}" PARENT_SCOPE) # set the list of assets in parent scope (equivalent to return value)
endfunction()

# accumulate the assets to build in this
set(GLOBAL_ASSETS_TO_BUILD "")

# these assets don't need any processing, but we need everything in the build
# directory to be usable by the executable
set(STRAIGHT_COPY_DIRECTORIES Scenes Shaders Models Textures Materials)
foreach(DIR ${STRAIGHT_COPY_DIRECTORIES})
    copy_assets(${DIR} GLOBAL_ASSETS_TO_BUILD)
endforeach()

add_custom_target(
    build_assets
    ALL    
    DEPENDS ${GLOBAL_ASSETS_TO_BUILD} compile_shaders
)

##
# build the pipelines
##
# Set the source and destination directories
set(PIPELINES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/Pipelines")
set(PIPELINES_DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/Pipelines") 
# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${PIPELINES_DEST_DIR})
file(GLOB_RECURSE PIPELINE_SRC_FILES CONFIGURE_DEPENDS "${PIPELINES_SOURCE_DIR}/*")
# Create a custom command for each pipeline file

foreach(FILE ${PIPELINE_SRC_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(OUTPUT_FILE "${PIPELINES_DEST_DIR}/${FILE_NAME}")
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND $<TARGET_FILE:PipelineBuilder> ${SHADER_DEST_DIR} ${PIPELINES_DEST_DIR} ${FILE}
        DEPENDS PipelineBuilder ${FILE}
        COMMENT "building ${OUTPUT_FILE} from ${FILE}"
    )
    list(APPEND PIPELINEBUILDER_OUTPUT_FILES ${OUTPUT_FILE})
endforeach()
# Create a custom target that depends on the output of the previous commands
add_custom_target(
    RunPipelineBuilder ALL
    DEPENDS ${PIPELINEBUILDER_OUTPUT_FILES}
)
# Make your main target depend on the RunPipelineBuilder target
message("Adding dependency on RunPipelineBuilder")
add_dependencies(VulkSamples RunPipelineBuilder)
