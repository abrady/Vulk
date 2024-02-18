# Add an executable for the tests
add_executable(VertFragTests vertfrag_tests.cpp)

target_link_libraries(VertFragTests Catch2::Catch2)

enable_testing()
add_test(NAME VertFragTests COMMAND VertFragTests)
