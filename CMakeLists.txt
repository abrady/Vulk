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
    add_compile_options(-W3)
else()
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_OSX_ARCHITECTURES arm64)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_definitions(LIBCXX_ENABLE_INCOMPLETE_FEATURES)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Check if vcpkg is bootstrapped
# find_program(VCPKG_EXECUTABLE NAMES vcpkg PATHS "${CMAKE_SOURCE_DIR}/vcpkg" NO_DEFAULT_PATH)
# if(NOT VCPKG_EXECUTABLE)
#   message(STATUS "Bootstrapping vcpkg...")
#   execute_process(COMMAND git submodule update --init --recursive
#                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#   execute_process(COMMAND ${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat
#                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)
#   # Find vcpkg executable again after bootstrapping
#   find_program(VCPKG_EXECUTABLE NAMES vcpkg PATHS "${CMAKE_SOURCE_DIR}/vcpkg" NO_DEFAULT_PATH)
# endif()
# if(VCPKG_EXECUTABLE)
#   execute_process(COMMAND ${VCPKG_EXECUTABLE} integrate install
#                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg)
# else()
#   message(FATAL_ERROR "Could not find vcpkg executable")
# endif()

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
## global includes
######

include_directories("${CMAKE_SOURCE_DIR}/Source")
include_directories("${CMAKE_SOURCE_DIR}/Source/ThirdParty/include")
include_directories("$ENV{VK_SDK_PATH}/include")

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
add_subdirectory(${CMAKE_SOURCE_DIR}/Source/Tools/BuildTool)

##############################################################################
## Assets
##############################################################################

# File to write
set(ASSETS_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets.txt")

# Ensure the file is empty before we start writing
file(WRITE ${ASSETS_OUTPUT_FILE} "")

###############################################
## Shader Related
###############################################

######
## Generate Shader Headers
######

# we do this first so the most up to date includes are available to the shader compilation
set(SHADER_GENERATED_HEADER_DIR "${GENERATED_RESOURCES_TARGET_DIR}/Shaders")
set(SHADER_GENERATED_HEADER_FILE "${SHADER_GENERATED_HEADER_DIR}/VulkShaderEnums_generated.glsl")
file(MAKE_DIRECTORY ${SHADER_GENERATED_HEADER_DIR})
add_custom_command(
    OUTPUT ${SHADER_GENERATED_HEADER_FILE}
    COMMAND $<TARGET_FILE:BuildTool> GenVulkShaderEnums ${SHADER_GENERATED_HEADER_FILE}
    DEPENDS BuildTool
    COMMENT "Generating shader includes header"
)

######
## Shaders
######

set(GLSLC_FLAGS "-g" "--target-env=vulkan1.3" "-I${SHADER_GENERATED_HEADER_DIR}" "-I${CMAKE_SOURCE_DIR}/Source/Shaders/Common") # build debug symbols and turn off optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GLSLC_FLAGS "-O0")
endif()

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
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES} ${SHADER_GENERATED_HEADER_FILE}
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
    CompileShadersTarget 
    DEPENDS ${COMPILED_SHADERS}
)

# Append each file name to the file
foreach(SOURCE_FILE ${COMPILED_SHADERS})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()


######
## Copied assets
######

# define a function to copy assets from the source directory to the build directory
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
        message("Copying ${ASSET_FILE} to ${DEST}")
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
set(ASSETS_TO_COPY "")

# these assets don't need any processing, but we need everything in the build
# directory to be usable by the executable
set(STRAIGHT_COPY_DIRECTORIES Scenes Models Textures Materials)
foreach(DIR ${STRAIGHT_COPY_DIRECTORIES})
    copy_assets(${DIR} ASSETS_TO_COPY)
endforeach()


# Append each file name to the file
foreach(SOURCE_FILE ${ASSETS_TO_COPY})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()


##
# build buildable assets
##

function(build_assets DIR_NAME GLOBAL_ASSETS_VAR)
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
            COMMAND $<TARGET_FILE:BuildTool> ${DIR_NAME} "${ASSET_FILE}" "${DEST}"
            DEPENDS BuildTool "${ASSET_FILE}"
            COMMENT "Building ${DIR_NAME} ${ASSET_FILE} to ${DEST}"
        )
        list(APPEND ASSETS_TO_BUILD "${DEST}")
    endforeach()
    set(${GLOBAL_ASSETS_VAR} "${ASSETS_TO_BUILD}" PARENT_SCOPE) # set the list of assets in parent scope (equivalent to return value)
endfunction()

set(ASSETS_TO_BUILD "")
set(BUILDTOOL_RESOURCE_DIRS )
foreach(DIR ${BUILDTOOL_RESOURCE_DIRS})
    build_assets(${DIR} ASSETS_TO_BUILD)
endforeach()

# Append each file name to the file
foreach(SOURCE_FILE ${ASSETS_TO_BUILD})
file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()


add_custom_target(
    BuildAssetsTarget
    ALL    
    DEPENDS ${ASSETS_TO_COPY} ${ASSETS_TO_BUILD} CompileShadersTarget
)

add_dependencies(VulkSamples BuildAssetsTarget)

##
# build the pipelines
##
# Set the source and destination directories
set(PIPELINES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/Pipelines")
set(PIPELINES_DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/Pipelines") 
# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${PIPELINES_DEST_DIR})
file(GLOB_RECURSE PIPELINE_SRC_FILES CONFIGURE_DEPENDS "${PIPELINES_SOURCE_DIR}/*.pipeline")
# Create a custom command for each pipeline file

foreach(FILE ${PIPELINE_SRC_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(ASSETS_OUTPUT_FILE "${PIPELINES_DEST_DIR}/${FILE_NAME}.bin")
    add_custom_command(
        OUTPUT ${ASSETS_OUTPUT_FILE}
        COMMAND $<TARGET_FILE:BuildTool> pipeline ${SHADER_DEST_DIR} ${ASSETS_OUTPUT_FILE} ${FILE}
        DEPENDS BuildTool ${FILE}
        # COMMENT "building ${ASSETS_OUTPUT_FILE} from ${FILE}"
    )
    list(APPEND PIPELINEBUILDER_ASSETS_OUTPUT_FILES ${ASSETS_OUTPUT_FILE})
endforeach()
# Create a custom target that depends on the output of the previous commands
add_custom_target(
    BuildPipelines ALL
    DEPENDS ${PIPELINEBUILDER_ASSETS_OUTPUT_FILES}
)

# Append each file name to the file
foreach(SOURCE_FILE ${PIPELINEBUILDER_ASSETS_OUTPUT_FILES})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()

# Make your main target depend on the BuildPipelines target
message("Adding dependency on BuildPipelines")
add_dependencies(VulkSamples BuildPipelines)
