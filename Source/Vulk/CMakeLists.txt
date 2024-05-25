cmake_minimum_required(VERSION 3.10)

######
## vcpkg
######

# Set the project name
project(Vulk)

# packages
find_package(assimp CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Vulkan REQUIRED)

# Add all the source files to a library
file(GLOB SOURCES "src/*.cpp")
add_library(Vulk 
  ${SOURCES} 
)
target_include_directories(Vulk PUBLIC include) # Make the include directory public

target_include_directories(Vulk PRIVATE private)

# Specify the include directories
target_include_directories(Vulk PRIVATE ${Stb_INCLUDE_DIR})

target_include_directories(Vulk PUBLIC ${GENSCHEMAFILES_INCLUDE_DIRS})

target_link_libraries(Vulk PRIVATE glfw)
target_link_libraries(Vulk PRIVATE Vulkan::Vulkan)
target_link_libraries(Vulk PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Vulk PRIVATE glm::glm-header-only)
target_link_libraries(Vulk PRIVATE assimp::assimp)
target_link_libraries(Vulk PRIVATE flatbuffers::flatbuffers)
#target_link_libraries(Vulk PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
target_link_libraries(Vulk PRIVATE cereal::cereal)
target_link_libraries(Vulk PRIVATE spdlog::spdlog)
target_link_libraries(Vulk PRIVATE fmt::fmt)
target_link_libraries(Vulk PUBLIC GenSchemaFiles)

target_include_directories(Vulk PUBLIC ${THRIFT_INCLUDE_DIR})
target_sources(Vulk PUBLIC ${THRIFT_SOURCES})

# Include the generated headers in your project
# NOTE: these are 'public' so anyone using this library will have access to these too
target_include_directories(Vulk PUBLIC ${GENERATED_HEADERS_DIR})



