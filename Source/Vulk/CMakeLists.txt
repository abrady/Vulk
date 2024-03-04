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
find_package(flatbuffers CONFIG REQUIRED)

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
target_link_libraries(Vulk PRIVATE flatbuffers::flatbuffers)

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


######
## Flatbuffer generation
######

set(FLATBUFFERS_FLATC_EXECUTABLE $<TARGET_FILE:flatbuffers::flatc>)
set(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated/Vulk)
file(MAKE_DIRECTORY ${GENERATED_HEADERS_DIR})

# List of schema files
file(GLOB FLATBUF_SCHEMA_FILES "${CMAKE_SOURCE_DIR}/Source/FlatbufSchemas/*.fbs")
#message("schema files: ${FLATBUF_SCHEMA_FILES}")

# Generate commands for each schema file
foreach(SCHEMA_FILE IN LISTS FLATBUF_SCHEMA_FILES)
    get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
    set(GENERATED_HEADER ${GENERATED_HEADERS_DIR}/${SCHEMA_NAME}_generated.h)
    add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp -o ${GENERATED_HEADERS_DIR} ${SCHEMA_FILE}
        DEPENDS ${SCHEMA_FILE}
        COMMENT "Compiling FlatBuffers schema: ${SCHEMA_FILE}"
    )
    list(APPEND GENERATED_HEADERS ${GENERATED_HEADER})
endforeach()

add_custom_target(GenerateFlatBuffersSchemas ALL DEPENDS ${GENERATED_HEADERS})


# Ensure your main target depends on the generated FlatBuffers schemas
add_dependencies(Vulk GenerateFlatBuffersSchemas)

# Include the generated headers in your project
# NOTE: these are 'public' so anyone using this library will have access to these too
target_include_directories(Vulk PUBLIC ${GENERATED_HEADERS_DIR})
