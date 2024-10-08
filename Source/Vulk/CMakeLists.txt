cmake_minimum_required(VERSION 3.10)

######
## vcpkg
######

# Set the project name
project(Vulk)

# packages
find_package(assimp CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Vulkan REQUIRED)

# Add all the source files to a library
file(GLOB SOURCES "src/*.cpp")
file(GLOB MODULES "modules/*.ixx") # getting compiler errors doing this for now.
#message(STATUS "MODULES: ${MODULES}")

add_library(Vulk ${SOURCES} ${MODULES})
add_dependencies(Vulk GenSchemaFiles)

target_include_directories(Vulk PUBLIC include) # Make the include directory public
target_include_directories(Vulk PRIVATE private)

# Specify the include directories
target_include_directories(Vulk PRIVATE ${Stb_INCLUDE_DIR})

target_include_directories(Vulk PUBLIC ${GENSCHEMAFILES_INCLUDE_DIRS})
# target_precompile_headers(Vulk PUBLIC include/Vulk/VulkPCH.h)

target_link_libraries(Vulk PRIVATE glfw)
target_link_libraries(Vulk PRIVATE Vulkan::Vulkan)
target_link_libraries(Vulk PRIVATE glm::glm-header-only)
target_link_libraries(Vulk PRIVATE assimp::assimp)
target_link_libraries(Vulk PRIVATE flatbuffers::flatbuffers)
#target_link_libraries(Vulk PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
target_link_libraries(Vulk PRIVATE fmt::fmt)
target_link_libraries(Vulk PUBLIC spdlog::spdlog) # Make spdlog public so that anyone using this library can use it too due to VulkLogger.h
target_link_libraries(Vulk PUBLIC GenSchemaFiles)

target_include_directories(Vulk PUBLIC ${THRIFT_INCLUDE_DIR})
target_sources(Vulk PUBLIC ${THRIFT_SOURCES})

# Include the generated headers in your project
# NOTE: these are 'public' so anyone using this library will have access to these too
target_include_directories(Vulk PUBLIC ${GENERATED_HEADERS_DIR})

add_subdirectory(Tests)

