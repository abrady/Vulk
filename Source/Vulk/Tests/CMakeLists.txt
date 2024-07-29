# Add an executable for the tests
add_executable(VulkTestsExe VulkTests.cpp)

find_package(Catch2 CONFIG REQUIRED)

target_link_libraries(VulkTestsExe PRIVATE Vulk)
target_link_libraries(VulkTestsExe PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)

enable_testing()
add_test(NAME VulkTests COMMAND VulkTestsExe)

