# Add an executable for the tests
add_executable(BuildToolTests BuildPipelineTests.cpp BuildToolTests.cpp BuildProjectTests.cpp ../BuildProject.cpp)

find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

target_link_libraries(BuildToolTests PRIVATE Vulk)
target_link_libraries(BuildToolTests PRIVATE spirv-cross-core spirv-cross-glsl spirv-cross-reflect spirv-cross-util)
target_link_libraries(BuildToolTests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)

enable_testing()
# add_test(NAME BuildToolTests COMMAND BuildToolTests)
add_test(NAME MyBuildToolTest COMMAND $<TARGET_FILE:BuildToolTests>)

