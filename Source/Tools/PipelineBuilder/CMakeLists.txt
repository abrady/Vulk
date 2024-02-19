cmake_minimum_required(VERSION 3.10)

project(PipelineBuilder)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)

find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Vulkan REQUIRED)


include_directories("${CMAKE_SOURCE_DIR}/Source")



add_executable(PipelineBuilder 
    PipelineBuilder.cpp
    "${CMAKE_SOURCE_DIR}/Source/Vulk/Vulk.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkActor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkCamera.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkDescriptorSetLayoutBuilder.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkDescriptorSetUpdater.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkGeo.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkMesh.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkPipelineBuilder.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkResourceMetadata.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkResources.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkTextureView.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Vulk/VulkUtil.cpp" 

)
target_link_libraries(PipelineBuilder PRIVATE Catch2::Catch2 Catch2::Catch2WithMain spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util nlohmann_json::nlohmann_json Vulkan::Vulkan)

target_link_libraries(PipelineBuilder PRIVATE glfw)
target_link_libraries(PipelineBuilder PRIVATE Vulkan::Vulkan)
target_link_libraries(PipelineBuilder PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(PipelineBuilder PRIVATE glm::glm-header-only)
target_link_libraries(PipelineBuilder PRIVATE assimp::assimp)


add_subdirectory(Tests)