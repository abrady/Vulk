# Add an executable for the tests
add_executable(PipelineBuilderTests PipelineBuilderTests.cpp)

find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# find_path(SPIRV_HEADERS_INCLUDE_DIRS "spirv/1.0/GLSL.std.450.h")
# target_include_directories(PipelineBuilderTests PRIVATE ${SPIRV_HEADERS_INCLUDE_DIRS})

target_link_libraries(PipelineBuilderTests PRIVATE Vulk)
target_link_libraries(PipelineBuilderTests PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(PipelineBuilderTests PRIVATE spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util)
target_link_libraries(PipelineBuilderTests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util nlohmann_json::nlohmann_json)

enable_testing()
vulk_add_and_remember_test(PipelineBuilderTests)
