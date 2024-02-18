# Note: if you put these above the FetchContent_MakeAvailable calls, it will not work
include_directories(${spirv-cross-core_SOURCE_DIR}/include)


# Add an executable for the tests
add_executable(PipelineBuilderTests PipelineBuilderTests.cpp)

find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(PipelineBuilderTests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain spirv-cross-core spirv-cross-glsl)

enable_testing()
add_test(NAME PipelineBuilderTests COMMAND PipelineBuilderTests)

